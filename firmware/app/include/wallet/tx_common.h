#ifndef WALLET_TX_COMMON_H
#define WALLET_TX_COMMON_H

#include "wallet_proto.h"
#include "storage_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int GetExtHeaderLen(const ProtoClientMessage *msg);

int TxGetVerifyCode(const ProtoClientMessage *msg);

int tx_save_history(const ProtoClientMessage *msg, DBTxCoinInfo *db);

#ifdef __cplusplus
}
#endif
#endif
