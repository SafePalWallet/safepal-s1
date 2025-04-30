#ifndef WALLET_WALLETMANAGER_H
#define WALLET_WALLETMANAGER_H

#include "wallet_proto.h"
#include "wallet_util.h"
#include "storage_manager.h"
#include "secure_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int wallet_init0();

int wallet_init(unsigned char is_init_se);

int wallet_isInited();

const sec_base_info *wallet_getBaseInfo();

int wallet_getPasswdAllowErrorTimes();

uint64_t wallet_getAccountId(int refresh);

uint64_t wallet_AccountId();

int wallet_getAccountSuffix(char suffix[4]);

int wallet_queryPubHDNode(uint16_t curv, const char *path, const unsigned char *passwd, PubHDNode *node);

int wallet_getPubHDNode(uint16_t curv, const char *path, const unsigned char *passwd, PubHDNode *node);

int wallet_getCoinPubHDNode(int type, const char *uname, const unsigned char *passwd, PubHDNode *node);

int wallet_genPathPubHDNode(const unsigned char *passwd, int curv, const char *path);

int wallet_genDefaultPubHDNode(const unsigned char *passwd, int type, const char *uname);

int wallet_gen_exists_hdnode(const unsigned char *passwd, uint64_t old_account_id);

int wallet_initDefaultCoin(const unsigned char *passwd);

int wallet_storeSeed(unsigned char *seed, int seedlen, const unsigned char *passwd);

int wallet_verify_mnemonic(const unsigned char *mnemonic, int len, const unsigned char *passwd);

int wallet_store_passphrase(const unsigned char *passphrase, int len, const unsigned char *passwd);

int wallet_destorySeed(int type);

int wallet_getHDNode(int type, const char *uname, HDNode *hdnode);

int wallet_genAddress(char *address, int size, HDNode *hdnode, int type, const char *uname, uint32_t index, int testnet);

int wallet_verify_seed_xpub(const unsigned char *seed, int seed_len);

#ifdef __cplusplus
}
#endif
#endif
