#define LOG_TAG "upgrade"

#include <stdio.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>
#include <aes/aes.h>
#include "common.h"
#include "upgrade_helper.h"
#include "fileutil.h"
#include "sha2.h"
#include "loading_win.h"
#include "resource.h"
#include "device_manager.h"

#define BLOCK_IS_RAMDISK 1

static char mMountPath[16] ="/path/mount";
static char mBlockPath[20] = "/dev/ram0";

enum {
	USB_STATE_NONE = 0,
	USB_STATE_MASS_STORAGE = 1,
};

static int upgrade_cleanDisk();

static int upgrade_isShaing();

static int upgrade_isMounted();

static int upgrade_doMount();

static int upgrade_doUnmount();

static int upgrade_verifyFirmware(HWND hwnd, const char *path, int upgrade);

static char storage_lun_path[64] = "/path/storage";

static int is_path_mounted(const char *checkPath) {
	const char *path = "/proc/mounts";

	FILE *fp;
	char line[255];
	if (!checkPath || checkPath[0] != '/') {
		db_error("invalid path:%s", checkPath);
		return -1;
	}
	int len = strlen(checkPath);
	if (!(fp = fopen(path, "r"))) {
		db_error("fopen fail=%s", path);
		return 0;
	}
	memset(line, 0, sizeof(line));
	while (fgets(line, sizeof(line) - 1, fp)) {
		//db_debug("line:%s", line);
		if (line[0] == '/' && !strncmp(line, checkPath, len) && line[len] == ' ') {
			db_debug("path:%s mounted", checkPath);
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	db_debug("path:%s not mount", checkPath);
	return 0;
}

static int mount_path(const char *dev, const char *path) {
	char cmd[64] = "/bin/mount -t vfat ";
	int prefix_len = 19;

	snprintf(cmd + prefix_len, sizeof(cmd) - prefix_len, "%s %s", dev, path);
	int ret = system(cmd);
	db_secure("system:%s ret:%d", cmd, ret);
	if (ret != 0) {
		return ret;
	}
	ret = is_path_mounted(dev);
	if (ret != 1) {
		db_error("not real mount?");
		return -1;
	}
	return 0;
}

static int umount_path(const char *dev, const char *path) {
	db_msg("umount dev:%s path:%s", dev, path);
	if (is_path_mounted(dev) != 1) {
		return 0;
	}
	int ret = umount(path);
	if (ret != 0) {
		db_error("call umout:%s ret:%d", path, ret);
		return ret;
	}
	db_msg("call umout:%s OK", path);
	if (is_path_mounted(dev) == 1) {
		db_error("keep mount??");
		return -1;
	}
	return 0;
}

static int get_usb_state() {
	char value[128] = {0};
	device_get_usb_config(value, sizeof(value));
	db_secure("usb state:%s", value);
	int state = USB_STATE_NONE;
	if (value[0] == 0 || strcmp(value, "none") == 0) {
		return state;
	}
	if (strstr(value, "mass_storage")) {
		state |= USB_STATE_MASS_STORAGE;
	}
	return state;
}

int set_usb_state(int state) {
	int c = get_usb_state();
	db_secure("current state:%d new state:%d", c, state);
	if (c == state) {
		return 0;
	}
	if (state == (state & c)) {
		db_secure("skip not need state:%d new state:%d", c, state);
		return 0;
	}
	int ret = 0;
	if (state) {
		ret = device_set_usb_config("mass_storage");
	} else {
		ret = device_set_usb_config("none");
	}
	return ret;
}

static int share_to_pc(const char *dev) {
	int fd = -1;
	db_debug("shareVol: %s; lun: %s", dev, storage_lun_path);
	set_usb_state(USB_STATE_MASS_STORAGE);
	if ((fd = open(storage_lun_path, O_WRONLY)) < 0) {
		db_error("Unable to open ums lunfile (%s)", strerror(errno));
		return -1;
	}
	if (write(fd, dev, strlen(dev)) < 0) {
		db_error("Unable to write to ums lunfile (%s)", strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static int unshare_to_pc() {
	int fd = -1;
	char ch = 0;
	int wait_i = 0;
	db_secure("unshare start");
#ifdef BUILD_FOR_RELEASE
	set_usb_state(USB_STATE_NONE);
#else
	if (Global_FactoryTesting || !app_run_in_dev_mode()) {
		set_usb_state(USB_STATE_NONE);
	}
#endif
	if ((fd = open(storage_lun_path, O_WRONLY)) < 0) {
		db_error("Unable to open ums lunfile (%s)", strerror(errno));
		return -1;
	}
	while (wait_i < 30) {
		if (write(fd, &ch, 1) >= 0) {
			break;
		}
		wait_i++;
		usleep(10000);
	}
	close(fd);
	if (wait_i == 30) {
		db_error("Unable to write to ums lunfile (%s)", strerror(errno));
		return -1;
	}
	return 0;
}

static int set_usb_switch(int state) {
	db_secure("state:%d", state);
	//...
	return 0;
}

static int clean_ramdisk(const char *dev) {
#if BLOCK_IS_RAMDISK
	int f;
	f = open(dev, O_RDWR);
	if (f < 1) {
		db_error("open:%s false", dev);
		return -1;
	}
	ioctl(f, BLKFLSBUF);
	close(f);
#endif
	return 0;
}

static int check_is_sharing(const char *dev) {
	char path[64] = {0};
	int ret = file_get_line(storage_lun_path, path, 63);
	db_msg("ret:%d sharing path:%s", ret, path);
	if (ret > 4) {
		if (is_not_empty_string(dev)) {
			ret--;
			if (path[ret] == '\n' || path[ret] == '\r' || path[ret] == ' ') {
				path[ret] = 0;
			}
			return strcmp(path, dev) == 0 ? 1 : 0;
		} else {
			return 1;
		}
	}
	return 0;
}

int upgrade_init() {
	return 0;
}

int upgrade_getState() {
	int state = 0;
	if (upgrade_isMounted() == 1) {
		db_msg("dev:%s mounted", mBlockPath);
		state |= OTA_DISK_STATE_MOUNT;
	}
	if (upgrade_isShaing() == 1) {
		db_msg("dev:%s sharing", mBlockPath);
		state |= OTA_DISK_STATE_SHARE;
	}
	return state;
}

int upgrade_shareToPC() {
	char cmd[64] = "newfs_msdos -O SafePal -L wallet ";
	int prefix_len = 33;//strlen(cmd);

	upgrade_doUnmount();
	memcpy(cmd + prefix_len, mBlockPath, sizeof(cmd) - prefix_len);
	int ret = system(cmd);
	db_secure("system:%s ret:%d", cmd, ret);
	if (ret != 0) {
		return ret;
	}
	ret = share_to_pc(mBlockPath);
	db_msg("share_to_pc ret:%d", ret);
	if (ret != 0) {
		return ret;
	}
	ret = check_is_sharing(mBlockPath);
	db_msg("check_is_sharing ret:%d", ret);
	if (ret != 1) {
		return -1;
	}
	if (set_usb_switch(1) != 0) {
		db_error("open usb sw false");
		return -1;
	}
	return 0;
}

int upgrade_unshareFromPC() {
	int ret;
	ret = set_usb_switch(0);
	if (ret != 0) {
		db_error("close usb sw false ret:%d", ret);
	}
	ret = upgrade_isShaing();
	if (ret == 1) {
		db_msg("block:%s sharing", mBlockPath);
		ret = unshare_to_pc();
		if (ret != 0) {
			db_error("unshare_to_pc False ret:%d", ret);
			return -1;
		} else {
			db_msg("unshare_to_pc OK");
		}
	} else {
		db_msg("dev:%s not sharing", mBlockPath);
	}
	return 0;
}

int upgrade_umount() {
	return upgrade_doUnmount();
}

int upgrade_clean() {
	int ret;
	ret = upgrade_doUnmount();
	if (ret != 0) {
		db_msg("mount false ret:%d", ret);
		return ret;
	}
	ret = upgrade_unshareFromPC();
	if (ret != 0) {
		db_msg("unshare false ret:%d", ret);
		return ret;
	}
	//clean data
	upgrade_cleanDisk();
	return ret;
}

int upgrade_verifyFirmware(HWND hwnd, const char *path, int upgrade) {
	db_secure("start path:%s", path);
	int fd = open(path, O_RDONLY);
	if (fd <= 0) {
		db_serr("open %s false", path);
		return -1;
	}
	int code = -1;
	do {
		//...
		db_secure("verify firmware OK");
		code = 0;
	} while (0);
	close(fd);

	if (IS_VALID_HWND(hwnd) && loading_win_processing && (upgrade || code != UPGRADE_FW_VERIFY_DIGEST_FAILED)) {
		loading_win_stop();
	}

	return code;
}

int upgrade_prepare(HWND hwnd, int upgrade) {
	char path[64];
	db_secure("start");
	const char *fn = "/upgrade.bin";
	int ret;
	int code = -1;
	ret = upgrade_doMount();
	if (ret) {
		db_error("mount false");
		return -51;
	}
	do {
		memcpy(path, mMountPath, sizeof(mMountPath));
		strcat(path, fn);
		if (access(path, F_OK) != 0) {
			db_error("ota file:%s not exist", path);
			code = -101;
			break;
		}
		ret = upgrade_verifyFirmware(hwnd, path, upgrade);
		if (ret != 0) {
			db_error("verifyFirmware false ret:%d", ret);
			if (ret == UPGRADE_FW_VERIFY_DIGEST_FAILED) {
				code = ret;
			} else {
				code = ret - 1000;
			}
			break;
		}
		code = 0;
	} while (0);
	memset(path, 0, sizeof(path));
	if (code) {
		db_error("err code:%d", code);
		upgrade_doUnmount();
	}
	db_secure("end");
	return code;
}

int upgrade_do_upgrade(HWND hwnd) {
	int ret = upgrade_prepare(hwnd, 1);
	if (loading_win_processing) {
		loading_win_stop();
	}
	if (ret != 0) {
		return ret;
	}
	ret = upgrade_unshareFromPC();
	db_serr("unshareFromPC ret:%d", ret);
	return 0;
}

int upgrade_cleanDisk() {
	db_msg("clean data");
#if BLOCK_IS_RAMDISK
	int ret = clean_ramdisk(mBlockPath);
	db_msg("clean_ramdisk ret:%d", ret);
#endif
	return 0;
}

int upgrade_isShaing() {
	return check_is_sharing(mBlockPath);
}

int upgrade_isMounted() {
	return is_path_mounted(mBlockPath);
}

int upgrade_doMount() {
	if (upgrade_isMounted() != 1) { //mount
		if (mount_path(mBlockPath, mMountPath) != 0) {
			db_error("false mount:%s -> %s", mBlockPath, mMountPath);
			return -1;
		}
	}
	return 0;
}

int upgrade_doUnmount() {
	int ret = umount_path(mBlockPath, mMountPath);
	if (ret != 0) {
		db_error("unmount:%s -> %s false", mBlockPath, mMountPath);
	} else {
		db_msg("unmount:%s -> %s OK", mBlockPath, mMountPath);
	}
	return ret;
}