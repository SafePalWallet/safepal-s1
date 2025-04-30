#define LOG_TAG "storage"

#include <string.h>
#include <unistd.h>
#include "common_c.h"
#include "storage_manager.h"
#include "db_ctrl.h"
#include "rtc.h"
#include "base58.h"
#include "rand.h"
#include "device.h"
#include "secure_api.h"
#include "secure_util.h"
#include "coin_config.h"
#include "common_util.h"

#define DB_CKEY_VERSION "version"
#define CURR_STORAGE_VERSION 3

static int G_lock = 0;

static inline void
LOCK() {
	while (__sync_lock_test_and_set(&(G_lock), 1)) {}
}

static inline void
UNLOCK() {
	__sync_lock_release(&(G_lock));
}

typedef struct {
	unsigned char tag[4];
	uint32_t len;
	unsigned char digest[8];
} encode_tx_st;

#define ENCODE_TX_TAG "BTX0"
#define ENCODE_TX_ST_SIZE 16
#define ENCODE_TX_FLAG 0x40000000

static ClientInfo mClientCache;
static int mDBInited = 0;
static long mWriteClock = 0;

static void *storage_getDBC();

static int storage_init();

static int storage_resetAllState();

static int storage_dbOpen();

static int storage_dbInit();

static int storage_init_config();

static int storage_checkClientIdExist(int id);

static int storage_genNewClientId();

static int storage_fetchTxsInfo(DBTxInfo *info, int fetch_data);

static int storage_fetchCoinInfo(DBCoinInfo *info);

static void storage_setWriteClock();

int storage_init() {
	int ret = 0;
	int reinit = 0;
	mWriteClock = 0;
	mDBInited = 0;
	if (access(DB_PATH, F_OK) != 0) { //db not exist
		reinit = 1;
	}
	db_debug("reinit:%d", reinit);
	ret = storage_dbOpen();
	if (ret != 0) {
		db_debug("dbOpen ret:%d", ret);
		unlink(DB_PATH);
		unlink(DB_JOURNAL_PATH);
		reinit = 1;
		ret = storage_dbOpen();
		if (ret != 0) { //error again
			db_error("open db false");
			return -1;
		}
	}
	if (reinit) {
		storage_dbInit();
	}
	mDBInited = 1;
	return 0;
}

int storage_resetAllState() {
	Global_Have_New_DBTX = 1;
	Global_Have_New_DBTX2 = 1;
	Global_Have_New_DBCoin = 1;
	memset(&mClientCache, 0, sizeof(ClientInfo));
	return 0;
}

int storage_trySync2Disk() {
	if (!mWriteClock) {
		return 0;
	}
	long now = getClockTime();
	db_msg("try sync write clock:%ld curr:%ld", mWriteClock, now);
	if (mWriteClock + 3 > now) {
		return 0;
	}
	storage_syncData2Disk();
	return 0;
}

int storage_syncData2Disk() {
	db_msg("sync inited:%d write clock:%ld", mDBInited, mWriteClock);
	if (!mDBInited) {
		return 0;
	}
	LOCK();
	mDBInited = 0;
	mWriteClock = 0;
	dbc_close();
	storage_resetAllState();
	UNLOCK();
	return 0;
}

int storage_cleanAllData() {
	db_msg("clean ALL");
	LOCK();
	mDBInited = 0;
	mWriteClock = 0;
	dbc_close();
	unlink(DB_PATH);
	unlink(DB_JOURNAL_PATH);
	storage_resetAllState();
	UNLOCK();
	return 0;
}

void *storage_getDBC() {
	int ret;
	LOCK();
	if (!mDBInited) {
		ret = storage_init();
		if (ret != 0) {
			db_error("init false ret:%d", ret);
			UNLOCK();
			return NULL;
		}
	}
	UNLOCK();
	return &mDBInited;
}

int storage_dbOpen() {
	if (dbc_open(DB_PATH) != 0) {
		db_error("oepn db:%s false", DB_PATH);
		return -1;
	}
	db_msg("oepn db:%s OK", DB_PATH);
	int ret = dbc_executeSQL("PRAGMA synchronous = FULL;");
	if (ret != 0) {
		db_error("PRAGMA synchronous FULL false");
	} else {
		db_msg("PRAGMA synchronous FULL OK");
	}
	return 0;
}

int storage_init_config() {
	const char *table = "CREATE TABLE IF NOT EXISTS " TABLE_CONFIG" (" \
        "ckey VARCHAR(255),"\
        "cvalue TEXT,"\
        "primary key (ckey)"\
    ");";
	int ret = dbc_executeSQL(table);
	if (ret != 0) {
		db_error("create table %s false ret:%d", TABLE_CONFIG, ret);
		return -1;
	}
	db_msg("create table:%s true", TABLE_CONFIG);
	char sql[64];
	snprintf(sql, sizeof(sql), "insert into %s values(\"%s\",\"%d\")", TABLE_CONFIG, DB_CKEY_VERSION, CURR_STORAGE_VERSION);
	dbc_executeSQL(sql);
	return 0;
}

int storage_dbInit() {
	int ret;
	db_msg("init db");
	ret = storage_init_config();
	if (ret != 0) {
		db_error("init config false ret:%d", ret);
		return ret;
	}

	const char *table = "CREATE TABLE IF NOT EXISTS " TABLE_CLIENTS" (" \
        "client_id INTEGER PRIMARY KEY,"\
        "unique_id VARCHAR(32),"\
        "bind_time INTEGER,"\
        "seckey VARCHAR(64),"\
        "client_name VARCHAR(64)"\
    ");";
	ret = dbc_executeSQL(table);
	if (ret != 0) {
		db_error("create table %s false ret:%d", TABLE_CLIENTS, ret);
		return -1;
	}
	db_msg("create table:%s true", TABLE_CLIENTS);

	table = "CREATE TABLE IF NOT EXISTS " TABLE_XPUBS" ("\
         "account VARCHAR(32),"\
         "curv INTEGER,"\
         "path VARCHAR(32),"\
         "xpub VARCHAR(256)"\
         ");";
	ret = dbc_executeSQL(table);
	if (ret != 0) {
		db_error("create table %s false ret:%d", TABLE_XPUBS, ret);
		return -1;
	}
	db_msg("create table:%s true", TABLE_XPUBS);

	table = "CREATE TABLE IF NOT EXISTS " TABLE_TXS " ("\
         "id INTEGER PRIMARY KEY,"\
         "msg_type INTEGER,"\
         "time INTEGER,"\
         "time_zone INTEGER,"\
         "client_id INTEGER,"\
         "flag INTEGER,"\
         "coin_type INTEGER,"\
         "coin_uname VARCHAR(64),"\
         "coin_name VARCHAR(64),"\
         "coin_symbol VARCHAR(32),"\
         "send_value VARCHAR(40),"\
         "currency_value VARCHAR(40),"\
         "currency_symbol VARCHAR(12),"\
         "data BLOB," \
         "account VARCHAR(32),"\
         "tx_type INTEGER"\
        ");";

	ret = dbc_executeSQL(table);

	if (ret != 0) {
		db_error("create table %s false ret:%d", TABLE_TXS, ret);
		return -1;
	}
	db_msg("create table:%s true", TABLE_TXS);

	table = "CREATE TABLE IF NOT EXISTS " TABLE_COINS " ("\
         "type INTEGER,"\
         "curv INTEGER,"\
         "decimals INTEGER,"\
         "uname VARCHAR(64),"\
         "name VARCHAR(64),"\
         "symbol VARCHAR(32),"\
         "flag INTEGER,"\
         "primary key (type,uname)"\
         ");";

	ret = dbc_executeSQL(table);

	if (ret != 0) {
		db_error("create table %s false ret:%d", TABLE_COINS, ret);
		return -1;
	}
	db_msg("create table:%s true", TABLE_COINS);
	return 0;
}

static int _read_coin_idx_callback(void *user, const char *key, const char *val) {
	if (is_empty_string(val)) {
		return 0;
	}
	db_msg("key:%s val:%s ", key, val);
	if (!strncmp(key, SETTING_KEY_BTC_MAX_INDEX, 11)) {
		const char *p = key + 11;
		int coin_id = -1;
		if (*p == 0) {
			coin_id = 0;
		} else if (*p >= '1' && *p <= '9') {
			coin_id = atoi(p);
		}
		if (coin_id >= 0) {
			storage_set_coin_max_index(*((uint64_t *) user), coin_id, strtol(val, NULL, 10));
		}
		config_file_set(WALLET_SETTINGS_FILE, key, ""); //clear
	}

	return 0;
}

int storage_upgrade(uint64_t account_id) {
	char sql[128];
	int ret;
	if (access(DB_PATH, F_OK) != 0) {
		db_msg("db file not exist,skip");
		return 0;
	}
	int version = storage_get_version();
	db_msg("have db version:%d, need version:%d", version, CURR_STORAGE_VERSION);
	if (version >= CURR_STORAGE_VERSION) {
		return 0;
	}
	int init_table = 0;
	if (version < 3) {
		//...
	}

	if (!init_table) {
		ret = storage_save_config_int(DB_CKEY_VERSION, CURR_STORAGE_VERSION);
		db_msg("upgrade version:%d ret:%d", CURR_STORAGE_VERSION, ret);
	}
	return ret ? -1 : 0;
}

int storage_save_config(const char *key, char *data, int len) {
	char sql[160];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (!is_safe_string(key, 100)) {
		return -1;
	}
	db_msg("key:%s data len:%d value:%s", key, len, data);
	snprintf(sql, sizeof(sql), "replace into %s values('%s',?)", TABLE_CONFIG, key);
	if (dbc_prepareSql(sql) != 0) {
		db_error("prepare config:%s false", key);
		return -1;
	}
	dbc_bindTextL(1, data, len);
	if (dbc_execStmt() != 0) {
		db_error("save config:%s false", key);
		return -1;
	}
	storage_setWriteClock();
	return 0;
}

int storage_get_config(const char *key, char *data, int len) {
	char sql[160];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (!is_safe_string(key, 100)) {
		return -1;
	}
	snprintf(sql, sizeof(sql), "select cvalue from %s where ckey='%s'", TABLE_CONFIG, key);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("error exec sql:%s", sql);
		return -1;
	}
	int ret = 0;
	if (dbc_getResult() == 0) {
		const char *val = dbc_getColumnText(0);
		if (val) {
			ret = strlcpy(data, val, len);
		}
	}
	dbc_endSelectSQL();
	if (!ret) {
		db_msg("not config key:%s", key);
	}
	return ret;
}

int storage_save_config_int(const char *key, int value) {
	char buff[20];
	snprintf(buff, sizeof(buff), "%d", value);
	return storage_save_config(key, buff, strlen(buff));
}

int storage_get_config_int(const char *key, int default_val) {
	char buff[32] = {0};
	int ret = storage_get_config(key, buff, 32);
	if (ret <= 0) {
		return default_val;
	} else {
		return strtol(buff, NULL, 10);
	}
}

int storage_get_version() {
	return storage_get_config_int(DB_CKEY_VERSION, 0);
}

int storage_queryClientId(const char *unique_id) {
	char sql[128];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (!is_safe_string(unique_id, CLIENT_UNIQID_MAX_LEN)) {
		return -1;
	}
	snprintf(sql, sizeof(sql), "select client_id from %s where unique_id='%s'", TABLE_CLIENTS, unique_id);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	int client_id = 0;
	if (dbc_getResult() == 0) {
		client_id = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return client_id;
}

int storage_queryClientUniqueId(int client_id, char *unique_id) {
    char sql[128];
    const char *val;
    int ret = -10;

    void *dbc = storage_getDBC();
    if (!dbc) {
        db_error("get dbc false");
        return -1;
    }

    snprintf(sql, sizeof(sql), "select unique_id from %s where client_id=%d", TABLE_CLIENTS, client_id);
    if (dbc_execSelectSQL(sql) != 0) {
        db_error("exec sql:%s error", sql);
        return -2;
    }
    if (dbc_getResult() == 0) {
        val = dbc_getColumnText(0);
        if (val) {
            strlcpy(unique_id, val, CLIENT_UNIQID_MAX_LEN + 1);
            ret = 0;
        }
    }
    dbc_endSelectSQL();
    return ret;
}

int storage_checkClientIdExist(int id) {
	char sql[64];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (id <= 0 || id >= 0xFF) return -1;
	snprintf(sql, sizeof(sql), "select client_id from %s where client_id=%d", TABLE_CLIENTS, id);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	int client_id = 0;
	if (dbc_getResult() == 0) {
		client_id = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return client_id > 0 ? 1 : 0;
}

int storage_genNewClientId() {
	int t = 0;
	uint32_t n;

	do {
		n = random32();
		n &= 0xFF;
		if (storage_checkClientIdExist(n) == 0) {
			db_msg("check id:%d OK", n);
			return n;
		} else {
			db_msg("check id:%d used", n);
		}
	} while (t++ < 50);

	for (n = 1; n < 0xFF; n++) {
		if (storage_checkClientIdExist(n) == 0) {
			db_msg("force check id:%d OK", n);
			return n;
		} else {
			db_msg("force check id:%d used", n);
		}
	}
	return -1;
}

int storage_getClientInfo(int client_id, ClientInfo *client) {
	char sql[128];
	const char *val;
	int ret = -1;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (mClientCache.client_id == client_id) {
		memcpy(client, &mClientCache, sizeof(ClientInfo));
		return 0;
	}

	snprintf(sql, sizeof(sql), "select unique_id,bind_time,seckey,client_name from %s where client_id=%d", TABLE_CLIENTS, client_id);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	int secsize = 0;
	char *sekeybuf = sql;
	int sekeybufsize = sizeof(sql);
	if (dbc_getResult() == 0) {
		memset(client, 0, sizeof(ClientInfo));
		client->client_id = client_id;
		val = dbc_getColumnText(0);
		if (val) {
			strlcpy(client->unique_id, val, sizeof(client->unique_id));
		}
		client->bind_time = (uint32_t) dbc_getColumnInt(1);
		val = dbc_getColumnText(2);
		if (val) {
			memset(sekeybuf, 0, sekeybufsize);
			secsize = base58_decode_check(val, HASHER_SHA2, (uint8_t *) sekeybuf, sekeybufsize);
			if (secsize == CLIENT_SECKEY_SIZE) {
				ret = 0;
				memcpy(client->seckey, sekeybuf, CLIENT_SECKEY_SIZE);
			} else {
				db_error("invalid seckey size:%d", secsize);
			}
		} else {
			db_error("empty seckey");
		}

		val = dbc_getColumnText(3);
		if (val) {
			strlcpy(client->client_name, val, CLIENT_NAME_MAX_LEN);
		}
	}
	dbc_endSelectSQL();
	if (ret == 0) {
		memcpy(&mClientCache, client, sizeof(ClientInfo));
	}
	return ret;
}

int storage_getClientSeckey(int client_id, unsigned char *key) {
	ClientInfo client;
	if (storage_getClientInfo(client_id, &client) == 0) {
		memcpy(key, client.seckey, CLIENT_SECKEY_SIZE);
		return CLIENT_SECKEY_SIZE;
	}
	return -1;
}

int storage_saveClientInfo(ClientInfo *client) {
	char sql[128];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	int client_id = client->client_id;
	if (client_id <= 0) {
		client_id = storage_genNewClientId();
		if (client_id <= 0) {
			db_error("gen new client_id false");
			return -2;
		}
	}
	if (!is_safe_string(client->unique_id, CLIENT_UNIQID_MAX_LEN)) {
		db_error("invalid uniq id");
		return -3;
	}

	if (client_id > 0xFF) {
		db_error("invalid client_id:%d", client_id);
		return SM_ERROR_TOO_MUCH_CLIENT;
	}

	snprintf(sql, sizeof(sql), "insert into %s values(%d,'%s',%d,?,?)", TABLE_CLIENTS, client_id, client->unique_id, client->bind_time);
	if (dbc_prepareSql(sql) != 0) {
		db_error("save client info false");
		return -4;
	}
	//reuse sql
	memset(sql, 0, sizeof(sql));
	if (base58_encode_check(client->seckey, CLIENT_SECKEY_SIZE, HASHER_SHA2, sql, sizeof(sql)) > 0) {
		dbc_bindText(1, sql);
	} else {
		db_error("encode seckey false");
		return -5;
	}
	dbc_bindText(2, client->client_name);
	if (dbc_execStmt() != 0) {
		db_error("save client info false");
		return -6;
	}
	storage_setWriteClock();
	return client_id;
}

static int get_xpub_aeskey(uint64_t account_id, const char *path, unsigned char key[32]) {
	//...
    return 32;
}

static int storage_delete_xpub(uint64_t account_id, uint16_t curv, const char *path) {
	char sql[128];
	snprintf(sql, sizeof(sql), "delete from %s where account='%llu' and curv=%d and path='%s'", TABLE_XPUBS, account_id, curv, path);
	dbc_executeSQL(sql);
	return 0;
}

int storage_getXpubInfo(uint64_t account_id, uint16_t curv, const char *path, uint8_t *xpub_bin, int xpub_size) {
	char sql[256];
	unsigned char key[32];
	const char *val = NULL;
	int len = 0;
	int find = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (!is_safe_string(path, 128)) {
		db_error("invalid path");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select xpub from %s where account='%llu' and curv=%d and path='%s'", TABLE_XPUBS, account_id, curv, path);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	int need_del = 0;
	if (dbc_getResult() == 0) {
		val = dbc_getColumnText(0);
		if (val) {
			memset(sql, 0, sizeof(sql));
			len = base58_decode_check(val, HASHER_SHA2, (uint8_t *) sql, 128);
			if (len >= 110) {
				if (len <= xpub_size) {
					memcpy(xpub_bin, sql, len);
					find = 1;
					db_info("find cached xpub:%s", val);
				} else if (!xpub_size) { //just check exist
					find = 1;
					db_info("just check find cached xpub:%s", val);
				}
			} else {
				need_del = 1;
			}
			if (!find) {
				db_error("invalid xpub decode size:%d buffer size:%d", len, xpub_size);
			}
		} else {
			need_del = 1;
			db_error("empty xpub account_id:%llx curv:%d path:%s", account_id, curv, path);
		}
	}
	dbc_endSelectSQL();

	if (need_del) {
		storage_delete_xpub(account_id, curv, path);
	}
	if (!find) {
		return 0;
	}
	if (!xpub_size) { //just check exist
		return 1;
	}
	if (get_xpub_aeskey(account_id, path, key) != 32) {
		db_error("gen aeskey false");
		return -1;
	}
	if (aes256_decrypt(xpub_bin, xpub_bin, len, key) != 0) {
		db_error("decrypt false");
		memzero(key, sizeof(key));
		storage_delete_xpub(account_id, curv, path);
		return -1;
	}
	sha256_Raw(xpub_bin, len - 4, key);
	if (memcmp(key, xpub_bin + len - 4, 4) != 0) {
		db_error("check digest false");
		storage_delete_xpub(account_id, curv, path);
		return -1;
	}
	memset(xpub_bin + len - 4, 0, 4);
	return 1;
}

int storage_check_xpub_exist(uint64_t account_id, uint16_t curv, const char *path) {
	return storage_getXpubInfo(account_id, curv, path, NULL, 0);
}

int storage_get_xpub_exists_paths(uint64_t account_id, cstring *data) {
	char sql[128];
	int curv;
	int len;
	const char *val = NULL;
	int total = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select curv,path from %s where account='%llu'", TABLE_XPUBS, account_id);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	memset(sql, 0, sizeof(sql));
	char *p;
	while (dbc_getResult() == 0) {
		curv = dbc_getColumnInt(0);
		val = dbc_getColumnText(1);
		if (val && *val == 'm') {
			len = strlen(val);
			if (len > 10) {
				p = sql;
				*p++ = len + 2; // curv+path+00
				*p++ = curv;
				memcpy(p, val, len);
				p += len;
				*p = 0;
				db_msg("add curv:%d path:%s", curv, val);
				//db_msg("add len:%d buff:%s", len + 3, debug_bin_to_hex(sql, len + 3));
				cstr_append_buf(data, sql, len + 3);
				total++;
			}
		}
	}
	dbc_endSelectSQL();
	return total;
}

int storage_saveXpubInfo(uint64_t account_id, uint16_t curv, const char *path, uint8_t *xpub_bin, int xpub_size) {
	char sql[256];
	int ret;
	unsigned char key[32];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (xpub_size > 128) {
		db_error("invalid xpub_size:%d", xpub_size);
		return -1;
	}
	if (!is_safe_string(path, 128)) {
		db_error("invalid path");
		return -1;
	}
	if (get_xpub_aeskey(account_id, path, key) != 32) {
		db_error("gen aeskey false");
		return -1;
	}

	snprintf(sql, sizeof(sql), "insert into %s values('%llu',%d,?,?)", TABLE_XPUBS, account_id, curv);
	if (dbc_prepareSql(sql) != 0) {
		db_error("save client info false");
		return -1;
	}
	dbc_bindText(1, path);

	//encrypt
	memcpy(sql, xpub_bin, xpub_size);
	sha256_Raw(xpub_bin, xpub_size, (unsigned char *) (sql + xpub_size));
	xpub_size += 4; //have digest
	ret = aes256_encrypt((unsigned char *) sql, (unsigned char *) sql, xpub_size, key);
	memset(key, 0, sizeof(key));
	if (ret < 0) {
		db_error("encrypt info false");
		return -1;
	}
	//reuse sql
	if (base58_encode_check((unsigned char *) sql, xpub_size, HASHER_SHA2, sql, sizeof(sql)) > 0) {
		dbc_bindText(2, sql);
	} else {
		db_error("encode xpub false");
		return -1;
	}
	if (dbc_execStmt() != 0) {
		db_error("save xpub false");
		return -1;
	}
	storage_setWriteClock();
	return 0;
}

int storage_cleanXpubs() {
	char sql[32];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "delete from %s", TABLE_XPUBS);
	dbc_executeSQL(sql);
	return 0;
}

static unsigned char *encode_tx(const DBTxInfo *info, unsigned int *new_size) {
	if (!info->data || !info->data->str || info->data->len < 1) {
		db_error("invalid data");
		return NULL;
	}
	//db_debug("encode 0 data:%d %s", info->data->len, debug_bin_to_hex(info->data->str, info->data->len));
	unsigned char key[32] = {0};
	char path[64];
	snprintf(path, sizeof(path), "%d-%d-%d", info->msg_type, info->time, info->coin_type);
	if (get_xpub_aeskey(info->client_id, path, key) != 32) {
		db_error("gen aeskey false");
		return NULL;
	}
	unsigned char *buff = calloc(1, ENCODE_TX_ST_SIZE + info->data->len);
	if (!buff) {
		db_error("calloc false size:%d", info->data->len);
		return NULL;
	}
	encode_tx_st *p = (encode_tx_st *) buff;
	memcpy(p->tag, ENCODE_TX_TAG, 4);
	p->len = info->data->len;
	memset(path, 0, 32);
	sha256_Raw((const uint8_t *) info->data->str, info->data->len, (uint8_t *) path);
	memcpy(p->digest, path, sizeof(p->digest));
	int ret = aes256_encrypt((const uint8_t *) info->data->str, buff + ENCODE_TX_ST_SIZE, info->data->len, key);
	memset(key, 0, sizeof(key));
	if (ret < 0) {
		db_error("encrypt info false");
		free(buff);
		return NULL;
	}
	*new_size = p->len + ENCODE_TX_ST_SIZE;
	//db_debug("encode 1 data:%d %s", *new_size, debug_ubin_to_hex(buff, *new_size));
	return buff;
}

static int decode_tx(DBTxInfo *info, const char *buff, int size) {
	if (!(info->flag & ENCODE_TX_FLAG)) {
		info->data = cstr_new_buf(buff, size);
		return 0;
	}
	unsigned char key[32] = {0};
	char path[64];
	if (size <= ENCODE_TX_ST_SIZE) {
		db_error("invalid size:%d", size);
		return -1;
	}
	encode_tx_st *p = (encode_tx_st *) buff;
	if (memcmp(p->tag, ENCODE_TX_TAG, 4) != 0) {
		db_error("invalid tx tag");
		return -2;
	}
	if (!p->len) {
		db_error("invalid tx len:%d", p->len);
		return -3;
	}
	if (p->len + ENCODE_TX_ST_SIZE > (uint32_t) size) {
		db_error("invalid tx size:%d exp len:%d", size, p->len);
		return -4;
	}
	snprintf(path, sizeof(path), "%d-%d-%d", info->msg_type, info->time, info->coin_type);
	if (get_xpub_aeskey(info->client_id, path, key) != 32) {
		db_error("gen aeskey false");
		return -11;
	}
	cstring *s = cstr_new_sz(p->len);
	if (!s) {
		db_error("new cstring false size:%d", size);
		memset(key, 0, sizeof(key));
		return -12;
	}
	s->len = p->len;//hack use
	//db_debug("decode 0 data:%d %s", p->len, debug_bin_to_hex(buff, p->len + ENCODE_TX_ST_SIZE));
	if (aes256_decrypt((const unsigned char *) (buff + ENCODE_TX_ST_SIZE), (unsigned char *) s->str, p->len, key) != 0) {
		db_error("decrypt false");
		memzero(key, sizeof(key));
		cstr_free(s);
		return -13;
	}
	memset(key, 0, sizeof(key));
	sha256_Raw((const unsigned char *) s->str, p->len, key);
	if (memcmp(key, p->digest, sizeof(p->digest)) != 0) {
		db_error("check digest false");
		cstr_free(s);
		return -14;
	}
	//db_debug("data:%d %s", s->len, debug_bin_to_hex(s->str, s->len));
	info->data = s;
	return 0;
}

int storage_saveTxsInfo(DBTxInfo *info) {
	char sql[256];
	if (!info->data || !info->data->str || info->data->len < 1) {
		db_error("invalid data");
		return -1;
	}
	if (!gSeedAccountId) {
		db_error("invalid accountid");
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	if (info->flag & ENCODE_TX_FLAG) {
		db_error("invalid flag:%d", info->flag);
		return -1;
	}
	info->flag |= ENCODE_TX_FLAG;

	Global_Have_New_DBTX = 1;
	Global_Have_New_DBTX2 = 1;
	int tx_type = info->tx_type;
	if (tx_type == 0) {
		if (info->coin_type == COIN_TYPE_BNC && strcmp(info->coin_uname, "DEX") == 0) {
			tx_type = 1;
		}
	}
	snprintf(sql, sizeof(sql), "insert into %s values(null,%d,%d,%d,%d,%d,%d,?,?,?,?,?,?,?,\"%llx\",%d)", TABLE_TXS,
	         info->msg_type, info->time, info->time_zone, info->client_id, info->flag, info->coin_type, gSeedAccountId, tx_type);

	if (dbc_prepareSql(sql) != 0) {
		db_error("save txs info false");
		return -1;
	}

	dbc_bindText(1, info->coin_uname);
	dbc_bindText(2, info->coin_name);
	dbc_bindText(3, info->coin_symbol);
	dbc_bindText(4, info->send_value);
	dbc_bindText(5, info->currency_value);
	dbc_bindText(6, info->currency_symbol);

	unsigned int encode_len = 0;
	unsigned char *entx = encode_tx(info, &encode_len);
	if (!entx) {
		db_error("save tx,encode tx false");
		return -1;
	}
	dbc_bindBlob(7, entx, (int) encode_len);

	if (dbc_execStmt() != 0) {
		db_error("save client info false");
		return -1;
	}
	storage_setWriteClock();
	return 0;
}

int storage_cleanTxs() {
	char sql[32];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	Global_Have_New_DBTX = 1;
	Global_Have_New_DBTX2 = 1;
	snprintf(sql, sizeof(sql), "delete from %s", TABLE_TXS);
	dbc_executeSQL(sql);
	return 0;
}

int storage_fetchTxsInfo(DBTxInfo *info, int fetch_data) {
	const char *val;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	info->id = dbc_getColumnInt(0);
	info->msg_type = dbc_getColumnInt(1);
	info->time = dbc_getColumnInt(2);
	info->time_zone = dbc_getColumnInt(3);
	info->client_id = dbc_getColumnInt(4);
	info->flag = dbc_getColumnInt(5);
	info->coin_type = dbc_getColumnInt(6);
	if (fetch_data) { //select *
		info->tx_type = dbc_getColumnInt(15);
	} else { //select xxx,xxx
		info->tx_type = dbc_getColumnInt(13);
	}

	val = dbc_getColumnText(7);
	if (val) {
		strlcpy(info->coin_uname, val, sizeof(info->coin_uname));
	}
	val = dbc_getColumnText(8);
	if (val) {
		strlcpy(info->coin_name, val, sizeof(info->coin_name));
	}

	val = dbc_getColumnText(9);
	if (val) {
		strlcpy(info->coin_symbol, val, sizeof(info->coin_symbol));
	}
	val = dbc_getColumnText(10);
	if (val) {
		strlcpy(info->send_value, val, sizeof(info->send_value));
	}
	val = dbc_getColumnText(11);
	if (val) {
		strlcpy(info->currency_value, val, sizeof(info->currency_value));
	}
	val = dbc_getColumnText(12);
	if (val) {
		strlcpy(info->currency_symbol, val, sizeof(info->currency_symbol));
	}
	if (info->data) {
		cstr_free(info->data);
		info->data = NULL;
	}

	if (fetch_data) {
		int data_size = 0;
		val = (char *) dbc_getColumnBlob(13, &data_size);
		if (val && data_size > 0) {
			int ret = decode_tx(info, val, data_size);
			if (ret != 0) {
				db_error("decode txdata false,ret:%d", ret);
				return -1;
			}
		}
	}
	return 0;
}

int storage_getTxsInfo(int id, DBTxInfo *info) {
	char sql[64];
	int find = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select * from %s where id=%d", TABLE_TXS, id);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		if (storage_fetchTxsInfo(info, 1) == 0) {
			find = 1;
		}
	}
	dbc_endSelectSQL();
	return find;
}

static int getQueryTxsWhere(char *where, int size, const DBTxFilter *filter) {
	int l = snprintf(where, size, "account=\"%llx\" and ", gSeedAccountId);
	size -= l;
	if (size <= 0) {
		strcpy(where, "1=0");
		return 0;
	}
	where += l;

	if (filter) {
		if (filter->tx_type > 0) {
			if (filter->tx_type == 1001) {
				strcpy(where, "(tx_type>=2 and tx_type<=6)");
			} else {
				snprintf(where, size, "tx_type=%d", filter->tx_type);
			}
		} else if (filter->coin_type) {
			if (filter->coin_type == TX_LIST_COIN_TYPE_TRANS) {
				strcpy(where, "tx_type=0");
			} else if (is_safe_string(filter->coin_uname, COIN_UNAME_MAX_LEN)) {
				snprintf(where, size, "tx_type=0 and coin_type=%d and coin_uname='%s'", filter->coin_type, filter->coin_uname);
			} else {
				strcpy(where, "1=0");
			}
		}
	}
	return 0;
}

int storage_getTxsCount(const DBTxFilter *filter) {
	char sql[160];
	int n = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select count(id) from %s where ", TABLE_TXS);
	n = strlen(sql);
	getQueryTxsWhere(sql + n, sizeof(sql) - n, filter);

	n = 0;
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		n = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return n;
}

int storage_getTxMaxId(const DBTxFilter *filter) {
	char sql[160];
	int n = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select max(id) from %s where ", TABLE_TXS);
	n = strlen(sql);
	getQueryTxsWhere(sql + n, sizeof(sql) - n, filter);

	n = 0;
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		n = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return n;
}

int storage_queryTxsInfo(DBTxInfo *info, int size, int offset, const DBTxFilter *filter) {
	char sql[320];
	int n = 0;
	if (!info || size < 1) {
		db_error("invalid param");
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select id,msg_type,time,time_zone,client_id,flag,coin_type,coin_uname,coin_name,coin_symbol,send_value,currency_value,currency_symbol,tx_type from %s where ", TABLE_TXS);
	n = strlen(sql);
	getQueryTxsWhere(sql + n, sizeof(sql) - n, filter);
	strcat(sql, " order by id desc");
	n = strlen(sql);
	snprintf(sql + n, sizeof(sql) - n, " limit %d", size);
	if (offset > 0) {
		n = strlen(sql);
		snprintf(sql + n, sizeof(sql) - n, " offset %d", offset);
	}

	n = 0;
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	while (dbc_getResult() == 0 && n < size) {
		if (storage_fetchTxsInfo(info + n, 0) == 0) {
			n++;
		}
	}
	dbc_endSelectSQL();
	return n;
}

int storage_save_coin(int type, const char *uname) {
	return storage_save_coin_info(getCoinConfig(type, uname));
}

int storage_save_coin_info(const CoinConfig *config) {
	if (!config) {
		return -1;
	}
	if (*config->uname == '.') { //hide
		return 0;
	}
	if (storage_isCoinExist(config->type, config->uname) > 0) { //query not inserted
		return 0;
	}
	DBCoinInfo _dbinfo;
	DBCoinInfo *dbinfo = &_dbinfo;
	memset(dbinfo, 0, sizeof(DBCoinInfo));
	dbinfo->type = config->type;
	dbinfo->curv = config->curv;
	dbinfo->decimals = config->decimals;
	strncpy(dbinfo->uname, config->uname, COIN_UNAME_MAX_LEN);
	strncpy(dbinfo->name, config->name, COIN_NAME_MAX_LEN);
	strncpy(dbinfo->symbol, config->symbol, COIN_SYMBOL_MAX_LEN);
	return storage_save_coin_dbinfo(dbinfo);
}

int storage_save_coin_dbinfo(const DBCoinInfo *info) {
	char sql[128];
	if (!info) {
		return -1;
	}
	if (!is_safe_string(info->uname, COIN_UNAME_MAX_LEN)) {
		return -1;
	}
	if (!is_safe_string(info->name, COIN_NAME_MAX_LEN)) {
		return -1;
	}
	if (!is_safe_string(info->symbol, COIN_SYMBOL_MAX_LEN)) {
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	Global_Have_New_DBCoin = 1;

	snprintf(sql, sizeof(sql), "insert into %s values(%d,%d,%d,?,?,?,%d)", TABLE_COINS, info->type, info->curv, info->decimals, info->flag);
	if (dbc_prepareSql(sql) != 0) {
		db_error("save txs info false");
		return -1;
	}
	dbc_bindText(1, info->uname);
	dbc_bindText(2, info->name);
	dbc_bindText(3, info->symbol);
	if (dbc_execStmt() != 0) {
		db_error("save client info false");
		return -1;
	}
	storage_setWriteClock();
	return 0;
}

int storage_deleteCoinInfo(int type, const char *uname) {
	char sql[128];
	if (!is_safe_string(uname, COIN_UNAME_MAX_LEN)) {
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	Global_Have_New_DBCoin = 1;
	snprintf(sql, sizeof(sql), "delete from %s where type=%d and uname='%s'", TABLE_COINS, type, uname);
	return dbc_executeSQL(sql);
}

int storage_cleanCoinInfo() {
	char sql[32];
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	Global_Have_New_DBCoin = 1;
	snprintf(sql, sizeof(sql), "delete from %s", TABLE_COINS);
	return dbc_executeSQL(sql);
}

int storage_fetchCoinInfo(DBCoinInfo *info) {
	const char *val;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
    info->type = dbc_getColumnInt(0);
    info->curv = (uint16_t) dbc_getColumnInt(1);
    if (info->curv == 0) {
        info->type = info->type & 0xFF;
        info->curv = (info->type >> 8) & 0xFF;
        db_msg("old db info->type:%#x, info->curv:%d", info->type, info->curv);
    }
	info->decimals = (uint16_t) dbc_getColumnInt(2);

	val = dbc_getColumnText(3);
	if (val) {
		strlcpy(info->uname, val, sizeof(info->uname));
	}
	val = dbc_getColumnText(4);
	if (val) {
		strlcpy(info->name, val, sizeof(info->name));
	}
	val = dbc_getColumnText(5);
	if (val) {
		strlcpy(info->symbol, val, sizeof(info->symbol));
	}
	info->flag = dbc_getColumnInt(6);

	return 0;
}

int storage_isCoinExist(int type, const char *uname) {
	char sql[128];
	int find = -1;
	if (!is_safe_string(uname, COIN_UNAME_MAX_LEN)) {
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select count(type) from %s where type=%d and uname='%s'", TABLE_COINS, type, uname);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		find = dbc_getColumnInt(0);
	} else {
		db_error("get result false");
	}
	dbc_endSelectSQL();
	return find;
}

int storage_getCoinInfo(int type, const char *uname, DBCoinInfo *info) {
	char sql[128];
	int find = 0;
	if (!info || !is_safe_string(uname, COIN_UNAME_MAX_LEN)) {
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "select * from %s where type=%d and uname='%s'", TABLE_COINS, type, uname);
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		if (storage_fetchCoinInfo(info) == 0) {
			find = 1;
		}
	}
	dbc_endSelectSQL();
	return find;
}

int storage_queryCoinInfo(DBCoinInfo *info, int size, int offset, int not_hide) {
	char sql[128];
	int n = 0;
	if (!info || size < 1) {
		db_error("invalid param");
		return -1;
	}
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}

	if (not_hide) {
		snprintf(sql, sizeof(sql), "select * from %s where (flag&1)=0", TABLE_COINS);
	} else {
		snprintf(sql, sizeof(sql), "select * from %s", TABLE_COINS);
	}
	n = strlen(sql);
	snprintf(sql + n, sizeof(sql) - n, " limit %d", size);
	if (offset > 0) {
		n = strlen(sql);
		snprintf(sql + n, sizeof(sql) - n, " offset %d", offset);
	}

	n = 0;
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	while (dbc_getResult() == 0 && n < size) {
		if (storage_fetchCoinInfo(info + n) == 0) {
			n++;
		}
	}
	dbc_endSelectSQL();
	return n;
}

int storage_getCoinsCount(int not_hide) {
	char sql[64];
	int n = 0;
	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}

	if (not_hide) {
		snprintf(sql, sizeof(sql), "select count(type) from %s where (flag&1)=0", TABLE_COINS);
	} else {
		snprintf(sql, sizeof(sql), "select count(type) from %s", TABLE_COINS);
	}
	n = 0;
	if (dbc_execSelectSQL(sql) != 0) {
		db_error("exec sql:%s error", sql);
		return -1;
	}
	if (dbc_getResult() == 0) {
		n = dbc_getColumnInt(0);
	}
	dbc_endSelectSQL();
	return n;
}

void storage_setWriteClock() {
	mWriteClock = getClockTime();
}

int storage_set_account_name(uint64_t account_id, char *name, int len) {
	char key[32];
	snprintf(key, sizeof(key), "wn_%llx", account_id);
	return storage_save_config(key, name, len);
}

int storage_get_account_name(uint64_t account_id, char *name, int len) {
	char key[32];
	snprintf(key, sizeof(key), "wn_%llx", account_id);
	return storage_get_config(key, name, len);
}

int storage_get_coin_max_index(uint64_t account_id, int coin_id) {
	char key[48];
	if (!account_id) {
		return 0;
	}
	if (coin_id == COIN_ID_BCH2) { //hack use
		coin_id = COIN_ID_BCH;
	}
	snprintf(key, sizeof(key), "cidx_%llx_%u", account_id, coin_id);
	return storage_get_config_int(key, 0);
}

int storage_set_coin_max_index(uint64_t account_id, int coin_id, int new_index) {
	char key[48];
	if (!account_id || new_index < 0) {
		return -1;
	}
	snprintf(key, sizeof(key), "cidx_%llx_%u", account_id, coin_id);
	return storage_save_config_int(key, new_index);
}

int storage_set_coin_flag(uint8_t type, const char *uname, int value) {
	char sql[128];
	int ret;

	if (!is_safe_string(uname, COIN_UNAME_MAX_LEN)) {
		db_error("param err");
		return -1;
	}

	void *dbc = storage_getDBC();
	if (!dbc) {
		db_error("get dbc false");
		return -1;
	}
	snprintf(sql, sizeof(sql), "update %s set flag=%d where type=%d and uname='%s'", TABLE_COINS, value, type, uname);
	ret = dbc_executeSQL(sql);
	if (ret != 0) {
		db_error("update flag false ret:%d", ret);
	}

	return 0;
}


