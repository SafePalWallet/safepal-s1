#ifndef _STORAGE_MANAGER_H
#define _STORAGE_MANAGER_H

#include "coin_util.h"
#include "cstr.h"

#define DB_PATH DATA_POINT"/wallet.db"
#define DB_JOURNAL_PATH DATA_POINT"/wallet.db-journal"

#define TABLE_CLIENTS "clients"
#define TABLE_XPUBS "xpubs"
#define TABLE_TXS "txs"
#define TABLE_COINS "coins"
#define TABLE_CONFIG "config"

#define SM_ERROR_TOO_MUCH_CLIENT (-201)
#define CLIENT_SECKEY_SIZE    32
#define CLIENT_NAME_MAX_LEN   63
#define CLIENT_UNIQID_MAX_LEN  20

#define TX_LIST_COIN_TYPE_TRANS (-1)
#define TX_LIST_COIN_TYPE_TRADE (-2)
#define TX_LIST_COIN_TYPE_AUTH (-3)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int client_id;
	unsigned int bind_time;
	unsigned char seckey[CLIENT_SECKEY_SIZE];
	char unique_id[CLIENT_UNIQID_MAX_LEN + 1];
	char client_name[CLIENT_NAME_MAX_LEN + 1];
} ClientInfo;

typedef struct {
    uint16_t type;
    uint8_t decimals;
    uint8_t curv;
    char uname[COIN_UNAME_BUFFSIZE];
    char name[COIN_NAME_BUFFSIZE];
    char symbol[COIN_SYMBOL_BUFFSIZE];
    int32_t flag; //0:hide 1:show 0x100:universal evm
} DBCoinInfo;

typedef struct {
	int id;
	int msg_type;
	int time;
	int time_zone;
	int client_id;
	int flag;
	int tx_type;
	int coin_type;
	char coin_uname[COIN_UNAME_BUFFSIZE];
	char coin_name[COIN_NAME_BUFFSIZE];
	char coin_symbol[COIN_SYMBOL_BUFFSIZE];
	char send_value[40];
	char currency_value[40];
	char currency_symbol[8];
	cstring *data;
} DBTxInfo;

typedef struct {
	int coin_type;
	const char *coin_uname;
	int tx_type;
} DBTxFilter;

int storage_trySync2Disk();

int storage_syncData2Disk();

int storage_cleanAllData();

int storage_upgrade(uint64_t account_id);

int storage_save_config(const char *key, char *data, int len);

int storage_get_config(const char *key, char *data, int len);

int storage_save_config_int(const char *key, int value);

int storage_get_config_int(const char *key, int default_val);

int storage_get_version();

int storage_queryClientId(const char *unique_id);

int storage_queryClientUniqueId(int client_id, char *unique_id);

int storage_getClientInfo(int client_id, ClientInfo *client);

int storage_getClientSeckey(int client_id, unsigned char *key);

int storage_saveClientInfo(ClientInfo *client);

int storage_getXpubInfo(uint64_t account_id, uint16_t curv, const char *path, uint8_t *xpub_bin, int xpub_size);

int storage_check_xpub_exist(uint64_t account_id, uint16_t curv, const char *path);

int storage_get_xpub_exists_paths(uint64_t account_id, cstring *data);

int storage_saveXpubInfo(uint64_t account_id, uint16_t curv, const char *path, uint8_t *xpub_bin, int xpub_size);

int storage_cleanXpubs();

int storage_saveTxsInfo(DBTxInfo *info);

int storage_cleanTxs();

int storage_getTxsInfo(int id, DBTxInfo *info);

int storage_getTxsCount(const DBTxFilter *filter);

int storage_getTxMaxId(const DBTxFilter *filter);

int storage_queryTxsInfo(DBTxInfo *info, int size, int offset, const DBTxFilter *filter);

int storage_save_coin(int type, const char *uname);

int storage_save_coin_info(const CoinConfig *config);

int storage_save_coin_dbinfo(const DBCoinInfo *info);

int storage_deleteCoinInfo(int type, const char *uname);

int storage_cleanCoinInfo();

int storage_isCoinExist(int type, const char *uname);

int storage_getCoinInfo(int type, const char *uname, DBCoinInfo *info);

int storage_queryCoinInfo(DBCoinInfo *info, int size, int offset, int not_hide);

int storage_getCoinsCount(int not_hide);

int storage_set_account_name(uint64_t account_id, char *name, int len);

int storage_get_account_name(uint64_t account_id, char *name, int len);

int storage_get_coin_max_index(uint64_t account_id, int coin_id);

int storage_set_coin_max_index(uint64_t account_id, int coin_id, int new_index);

int storage_set_coin_flag(uint8_t type, const char *uname, int value);

#ifdef __cplusplus
}
#endif

#endif
