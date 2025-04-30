#define LOG_TAG "device"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/mount.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "common_c.h"
#include "device.h"
#include "fileutil.h"
#include "sha2.h"
#include "config.h"
#include "crypto/secp256k1.h"
#include "secure_util.h"
#include "secure_api.h"
#include "loading_win.h"

#define NVM_HEADER_LEN  4
#define PRIVATE_TAG_LEN 4
#define PRIVATE_SN_SIZEOF 24

#define DEVICE_CPUID_BUFFSIZE (DEVICE_CPUID_LEN+4)

typedef struct {
	char check[PRIVATE_TAG_LEN];
	int16_t version;
	uint16_t len;
	char sn[PRIVATE_SN_SIZEOF];
	char id[24];
	char hostkey[20];
	unsigned char host_privkey[32];
	unsigned char se_type;
	unsigned char se_hostkey[16];
	unsigned char se_pubkey[33];
	char inited;
	unsigned char sign_index;
	unsigned char sign_data[64];
} PrivateDataInfo;

typedef struct {
	uint64_t id;
	unsigned char _r[16];
} seed_info_t;

static char *g_CpuId = NULL;
static PrivateDataInfo *gPrivateData = NULL;
static int gPrivDataCached = 0;
static int gActiveTime = -1;

static const char *getPrivatePath() {
	return "/path/to/private";
}

static int clearNvmData(int addr, int size, int block) {
	unsigned char tmp[256];
	if (size < 1 || size > 0xFF) {
		db_serr("%d invalid input param", __LINE__);
		return -1;
	}
	if (size < NVM_HEADER_LEN) size = NVM_HEADER_LEN;

	int fd = open(getPrivatePath(), O_WRONLY | O_NONBLOCK);
	if (fd <= 0) {
		db_serr("open mtd block failed");
		return -1;
	}
	int offset = addr + block * PRIVATE_DATA_BAKUP_OFFSET;
	if (offset > 0 && lseek(fd, offset, SEEK_SET) != offset) {
		close(fd);
		db_serr("lseek false");
		return -2;
	}
	memset(tmp, 0xFF, size);
	int datalen = write(fd, tmp, size);
	if (datalen != size) {
		db_serr("write nvm addr:0x%x block:%d header false ret:%d", addr, block, datalen);
		close(fd);
		return -3;
	}
	fsync(fd);
	close(fd);
	if (block == 0) { //backup
		clearNvmData(addr, size, 1);
	}
	return size;
}

static int writeNvmData(int addr, const void *data, int size, int block, int backup) {
	unsigned char tmp[NVM_HEADER_LEN];
	if (NULL == data || size < 1 || size > 0xFF) {
		db_serr("%d invalid input param", __LINE__);
		return -1;
	}
	int fd = open(getPrivatePath(), O_WRONLY | O_NONBLOCK);
	if (fd <= 0) {
		db_serr("open mtd block failed");
		return -1;
	}
	int offset = addr + block * PRIVATE_DATA_BAKUP_OFFSET;
	if (offset > 0 && lseek(fd, offset, SEEK_SET) != offset) {
		close(fd);
		db_serr("lseek false");
		return -2;
	}
	tmp[0] = '';
	tmp[1] = '';
	tmp[2] = '';
	tmp[3] = size & 0xFF;
	int datalen = write(fd, tmp, NVM_HEADER_LEN);
	if (datalen != NVM_HEADER_LEN) {
		db_serr("write nvm addr:0x%x block:%d header false ret:%d", addr, block, datalen);
		close(fd);
		return -3;
	}
	datalen = write(fd, data, size);
	db_secure("write nvm data addr:0x%x block:%d len=%d", addr, block, datalen);
	fsync(fd);
	close(fd);
	if (datalen != size) {
		return -4;
	}
	if (block == 0 && backup) { //backup
		writeNvmData(addr, data, size, 1, 0);
	}
	return datalen;
}

static int readNvmData(int addr, void *data, int size, int block) {
	unsigned char tmp[NVM_HEADER_LEN] = {0};
	if (NULL == data || size < 1) {
		db_serr("invalid input param");
		return -1;
	}
	int fd = open(getPrivatePath(), O_RDONLY | O_NONBLOCK);
	if (fd <= 0) {
		db_serr("open nvm block failed fd:%d err:%d", fd, errno);
		return -1;
	}
	int offset = addr + block * PRIVATE_DATA_BAKUP_OFFSET;
	if (offset > 0 && lseek(fd, offset, SEEK_SET) != offset) {
		close(fd);
		db_serr("lseek false");
		return -2;
	}

	int datalen = read(fd, tmp, NVM_HEADER_LEN);
	if (datalen != NVM_HEADER_LEN || tmp[0] != 0xca || tmp[1] != 0x48 || tmp[2] != 0) {
		db_serr("invalid nvm:%d header block:%d", addr, block);
		close(fd);
		if (block == 0) {
			return readNvmData(addr, data, size, 1);
		} else {
			return -3;
		}
	}
	int readlen = (size > tmp[3]) ? tmp[3] : size;
	datalen = read(fd, data, readlen);
	close(fd);
	if (datalen != readlen) {
		db_secure("read data len:%d != %d ", datalen, readlen);
		return -4;
	}
	return readlen;
}

static int getPrivateAESKey(unsigned char *key) {
	//...
	return 32;
}

static int writeEncryptNvmData(int addr, const void *data, int size, int block, int backup) {
	unsigned char buff[256];
	if (size < 1 || size > 250) {
		db_error("invalid size:%d", size);
		return -1;
	}
	unsigned char key[32];
	sha256_Raw(data, size, key);
	memcpy(buff, data, size);
	memcpy(buff + size, key, 4);
	int total_len = size + 4;
	if (getPrivateAESKey(key) != 32) {
		db_serr("get AES key false");
		return -1;
	}
	int ret = aes256_encrypt((const unsigned char *) buff, buff, total_len, key);
	memset(key, 0, sizeof(key));
	if (ret != 0) {
		db_serr("encrypt data false addr:%d ret:%d need:%d", addr, ret, total_len);
		return -1;
	}
	ret = writeNvmData(addr, buff, total_len, block, backup);
	if (ret != total_len) {
		db_serr("save data false addr:%d len:%d ret:%d", addr, total_len, ret);
		return -1;
	}
	return size;
}

static int readEncryptNvmData(int addr, void *data, int size, int block) {
	unsigned char buff[256] = {0};
	int len = readNvmData(addr, buff, sizeof(buff), block);
	if (len < 0) {
		return len;
	}
	if (len <= 4) {
		db_error("invalid read addr:%d ret:%d", addr, len);
		return 0;
	}
	unsigned char key[32];
	if (getPrivateAESKey(key) != 32) {
		db_serr("get AES key false");
		return -1;
	}
	int ret = aes256_decrypt(buff, buff, len, key);
	memset(key, 0, sizeof(key));
	if (ret != 0) {
		db_serr("decrypt data false addr:%d ret:%d", addr, ret);
		return -1;
	}
	len -= 4;
	sha256_Raw(buff, len, key);
	if (memcmp(buff + len, key, 4) != 0) {
		db_serr("decrypt data verify false addr:%d", addr);
		return -1;
	}
	if (size > len) size = len;
	memcpy(data, buff, size);
	return size;
}

static int readPrivateData(PrivateDataInfo *privdata, int block) {
	//..
	return 0;
}

static PrivateDataInfo *getPrivateDataInfo() {
	int ret;
	if (gPrivateData) {
		if (!gPrivDataCached) {
			ret = readPrivateData(gPrivateData, 0);
			gPrivDataCached = 1;
			if (ret != 0) {
				gPrivateData->version = 0;
				db_serr("readPrivateData false,ret:%d", ret);
			}
		}
		return gPrivateData->version ? gPrivateData : NULL;
	} else {
		return NULL;
	}
}

int device_init(unsigned char *mask) {
	gPrivateData = (PrivateDataInfo *) calloc(1, sizeof(PrivateDataInfo));
	if (gPrivateData == NULL) {
		return -1;
	}

	g_CpuId = (char *) calloc(1, DEVICE_CPUID_BUFFSIZE);
	if (g_CpuId == NULL) {
		return -11;
	}
}

int device_get_cpuid(char *buf, int len) {
	if (NULL == buf || len <= DEVICE_CPUID_LEN) {
		db_serr("invalid param");
		return -1;
	}
	int retlen;
	const char *cpu = device_get_cpuid_p();
	if (!cpu) {
		return -1;
	}
	memset(buf, 0, len);
	retlen = strlen(cpu);
	if (retlen != DEVICE_CPUID_LEN) {
		db_serr("invalid cpuid");
		return -1;
	}
	strncpy(buf, cpu, retlen);
	buf[retlen] = '\0';
	return retlen;
}

const char *device_get_cpuid_p() {
	if (g_CpuId == NULL) {
		db_serr("invalid param");
		return NULL;
	}
	int retlen;
	if (g_CpuId[0] == 0) {
		retlen = device_imp_get_cpuid(g_CpuId, DEVICE_CPUID_BUFFSIZE);
		if (retlen != DEVICE_CPUID_LEN) {
			memset(g_CpuId, 0, DEVICE_CPUID_BUFFSIZE);
			db_serr("getCpuId false ret:%d", retlen);
			return NULL;
		}
	}
	return g_CpuId;
}

int device_get_diviceid(char *buf, int len) {
	return device_get_cpuid(buf, len);
}

const char *device_get_diviceid_p() {
	return device_get_cpuid_p();
}

int device_get_id(char *buf, int len) {
	if (NULL == buf || len < 24) {
		db_serr("invalid param");
		return -1;
	}
	PrivateDataInfo *privdata = getPrivateDataInfo();
	if (privdata == NULL) {
		return -2;
	}
	int idlen = strlen(privdata->id);
	if (!idlen) {
		return -4;
	}
	if (idlen >= len) {
		idlen = len - 1;
	}
	memcpy(buf, privdata->id, idlen);
	buf[idlen] = '\0';
	return idlen;
}

int device_is_inited() {
	PrivateDataInfo *privdata = getPrivateDataInfo();
	if (privdata == NULL) {
		db_serr("getPrivateDataInfo false");
		return 0;
	}

	return (privdata->sn[0] && privdata->inited && privdata->sign_index) ? 1 : 0;
}

int device_check_sn(const char *sn) {
	//...
	return 0;
}

int device_get_sn(char *buf, int len) {
	if (NULL == buf || len < 24) {
		db_serr("invalid param");
		return -1;
	}
	PrivateDataInfo *privdata = getPrivateDataInfo();
	if (privdata == NULL) {
		return -2;
	}
	db_secure("get SN:[%s]", privdata->sn);
	int snlen = strlen(privdata->sn);
	if (device_check_sn(privdata->sn) != 0) {
		db_serr("check SN:%s false", privdata->sn);
		return 0;
	}

	if (snlen >= len) {
		snlen = len - 1;
	}
	memcpy(buf, privdata->sn, snlen);
	buf[snlen] = '\0';
	return snlen;
}

int device_check_host() {
	//...
	return 0;
}

int device_check_sechip() {
	//...
	return 0;
}

int device_check_se_shake(const unsigned char *host_random, const unsigned char *se_sign) {
	PrivateDataInfo *privdata = getPrivateDataInfo();
	if (privdata == NULL) {
		db_serr("getPrivateDataInfo false");
		return 0;
	}
	if (!buffer_is_zero(privdata->se_pubkey, 33) && ecdsa_verify_digest(&secp256k1, privdata->se_pubkey, se_sign, host_random) != 0) {
		db_serr("verify shake sign false pubkey:%s", debug_ubin_to_hex(privdata->se_pubkey, 33));
		db_serr("se_sign:%s", debug_ubin_to_hex(se_sign, 64));
		return -1;
	}
	return 0;
}

int device_sign_se_shake(const unsigned char *se_random, unsigned char *host_sign) {
	PrivateDataInfo *privdata = getPrivateDataInfo();
	if (privdata == NULL) {
		db_serr("getPrivateDataInfo false");
		return -1;
	}

	if (buffer_is_zero(privdata->host_privkey, 32)) {
		db_serr("emoty host_privkey");
		return -1;
	}

	if (ecdsa_sign_digest(&secp256k1, privdata->host_privkey, se_random, host_sign, NULL, NULL) != 0) {
		db_serr("sign se random false");
		return -1;
	}
	return 0;
}

static int checkUserActiveInfo(UserActiveInfo *info) {
	//...
	return 0;
}

int device_get_user_active_info(UserActiveInfo *info) {
	memset(info, 0, sizeof(UserActiveInfo));
	int ret = readNvmData(NVM_ADDR_USER_ACTIVE_OFFSET, info, sizeof(UserActiveInfo), 0);
	if (ret != sizeof(UserActiveInfo)) {
		return -1;
	}
	unsigned char key[32];
	if (getPrivateAESKey(key) != 32) {
		db_serr("get AES key false");
		return -1;
	}
	ret = aes256_decrypt((const unsigned char *) info, (unsigned char *) info, sizeof(UserActiveInfo), key);
	memset(key, 0, sizeof(key));
	if (ret < 0) {
		db_serr("decrypt uactive data false");
		return -1;
	}
	if (checkUserActiveInfo(info) != 0) {
		return -1;
	}
	return 0;
}

int device_get_active_time() {
	if (gActiveTime > 0) {
		return gActiveTime;
	}
	UserActiveInfo info;
	int ret = device_get_user_active_info(&info);
	if (ret != 0) {
		db_serr("not user active info");
		return 0;
	}
	db_secure("set uactive time:%d time_zone:%d", info.time, info.time_zone);
	gActiveTime = info.time + info.time_zone;
	return gActiveTime;
}

int device_user_active(UserActiveInfo *info, int do_active) {
	int ret;
	if (!info) {
		return -1;
	}
	if (!device_is_inited()) {
		db_serr("device not inited");
		return -1;
	}

	ret = checkUserActiveInfo(info);
	if (ret != 0) {
		db_serr("check info false ret:%d", ret);
		return ret;
	}
	db_secure("active info PASS");
	if (!do_active) {
		return 0;
	}
	unsigned char key[32];
	unsigned char eninfo[sizeof(UserActiveInfo)];

	if (getPrivateAESKey(key) != 32) {
		db_serr("get AES key false");
		return -1;
	}

	ret = aes256_encrypt((const unsigned char *) info, eninfo, sizeof(UserActiveInfo), key);
	memset(key, 0, sizeof(key));
	if (ret < 0) {
		db_serr("encrypt uactive data false");
		return -1;
	}
	if (writeNvmData(NVM_ADDR_USER_ACTIVE_OFFSET, eninfo, sizeof(UserActiveInfo), 0, 1) == sizeof(UserActiveInfo)) {
		db_secure("save uactive OK");
	} else {
		db_serr("save uactive failed");
		return -1;
	}
	gActiveTime = -1;
	return 0;
}

int device_get_hw_break_state(int trytime) {
	char path[] = "";
	int ret = 0;
	do {
		ret = file_get_int(path);
		if (ret == 0) {
			break;
		}
		db_serr("cover state:%d", ret);
		trytime--;
		if (trytime > 0) {
			usleep(10 * 1000);
		} else {
			break;
		}
	} while (1);
	memset(path, 0, sizeof(path));
	return ret;
}

int device_boot_is_otp() {
	return 1;
}

int device_set_boot_otp() {
	//...
	db_secure("set boot OTP OK");
	return 0;
}

uint64_t device_read_seed_account() {
	seed_info_t info;
	memset(&info, 0, sizeof(info));
	int ret = readEncryptNvmData(NVM_ADDR_SEED_INFO_OFFSET, &info, sizeof(info), 0);
	if (ret < 4) {
		return 0;
	}
	return info.id;
}

int device_save_seed_account(uint64_t id) {
	seed_info_t info;
	int ret;
	if (!id) {
		ret = clearNvmData(NVM_ADDR_SEED_INFO_OFFSET, 32, 0);
		db_secure("clean account ret:%d", ret);
		return ret;
	} else {
		memset(&info, 0, sizeof(info));
		info.id = id;
		ret = writeEncryptNvmData(NVM_ADDR_SEED_INFO_OFFSET, &info, sizeof(info), 0, 1);
		db_secure("save account id:%llx ret:%d", id, ret);
		return ret;
	}
}

int device_read_settings(unsigned char *data, int size) {
	return readEncryptNvmData(NVM_ADDR_SETTINGS_OFFSET, data, size, 0);
}

int device_save_settings(const unsigned char *data, int size) {
	if (size > 120) {
		db_error("invalid size:%d", size);
		return -1;
	}
	return writeEncryptNvmData(NVM_ADDR_SETTINGS_OFFSET, data, size, 0, 1);
}

int device_clean_all_info() {
	clearNvmData(NVM_ADDR_SETTINGS_OFFSET, 128, 0);
	device_save_seed_account(0);
	return 0;
}

int device_reformat_data_partition() {
	//...
	return 0;
}

long device_data_partition_size() {
	return file_size("/path");
}
