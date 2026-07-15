#define LOG_TAG "tx_common"

#include "tx_common.h"
#include "wallet_proto_qr.h"
#include "qr_pack.h"
#include "protobuf_util.h"
#include "sha2.h"
#include "device.h"
#include "wallet_manager.h"
#include "common_util.h"

int GetExtHeaderLen(const ProtoClientMessage *msg) {
    size_t len = 0;
    if (msg && msg->data) {
        if (msg->flag & QR_FLAG_HAS_TIME) {
            len += 6;
            if (msg->data->len < len) {
                db_error("invalid data len:%d < time len:6", msg->data->len);
                return -1;
            }
        }
        if (msg->flag & QR_FLAG_EXT_HEADER) {
            if (msg->data->len < (len + 11)) {
                db_error("invalid data len:%d from len:%d", msg->data->len, len);
                return -2;
            }
            if (msg->data->str[len] != 0x7a) {
                return -3;
            }
            uint32_t low = 0;
            uint32_t hi = 0;
            len += 1;
            len += pb_decode((uint8_t *) (msg->data->str + len), &low, &hi);
            if (hi != 0 || low >= 0x4000) {
                db_error("invalid ext header var len:%d %d", low, hi);
                return -4;
            }
            len += low;
            if (msg->data->len < len) {
                db_error("invalid data len:%d < %d varlen:%d", msg->data->len, len, low);
                return -5;
            }
        }
    }
    return (int) len;
}

int TxGetVerifyCode(const ProtoClientMessage *msg) {
    SHA256_CTX context;
    char sn[24];
    int ret;
    char unique_id[CLIENT_UNIQID_MAX_LEN + 1];
    char str[32];
    uint8_t digest[32];

    sha256_Init(&context);

    memzero(sn, sizeof(sn));
    ret = device_get_sn(sn, 24);
    if (ret <= 0) {
        db_error("get SN error, ret:%d", ret);
        return -10;
    }
    sha256_Update(&context, (const unsigned char *) sn, ret);

    memzero(unique_id, sizeof(unique_id));
    ret = storage_queryClientUniqueId(msg->client_id, unique_id);
    if ((ret != 0) || (!is_safe_string(unique_id, CLIENT_UNIQID_MAX_LEN))) {
        db_error("get unique id error, ret:%d", ret);
        return -11;
    }
    sha256_Update(&context, (const unsigned char *) unique_id, strlen(unique_id));

    memzero(str, sizeof(str));
    snprintf(str, sizeof(str), "%d", msg->client_id);
    sha256_Update(&context, (const unsigned char *) str, strlen(str));

    uint64_t account_id = wallet_AccountId();
    if ((uint32_t) account_id != msg->account_id) {
        db_error("account mismatch local:%u msg:%u", (uint32_t) account_id, msg->account_id);
        return -12;
    }
    memzero(str, sizeof(str));
    snprintf(str, sizeof(str), "%u", msg->account_id);
    sha256_Update(&context, (const unsigned char *) str, strlen(str));

    int ext_header_len = GetExtHeaderLen(msg);
    if (ext_header_len < 0) {
        db_error("get ext header false ext_header_len:%d", ext_header_len);
        return -13;
    }
    int data_len = msg->data->len - ext_header_len;
    if (msg->p_total > 1) {
        data_len -= QR_HASH_CHECK_LEN;
    }
    sha256_Update(&context, (const unsigned char *) msg->data->str + ext_header_len, data_len);
    sha256_Final(&context, digest);
    sha256_Raw(digest, 32, digest);
    unsigned int n = read_be(digest);
    n = n % 1000000;
    if (!n) n = 1;
    return n;
}

int tx_save_history(const ProtoClientMessage *msg, DBTxCoinInfo *db) {
    DBTxInfo tx[1];
    tx->msg_type = msg->type;
    tx->time = msg->time;
    tx->time_zone = msg->time_zone;
    tx->client_id = msg->client_id;
    memcpy(&tx->flag, db, sizeof(DBTxCoinInfo));
    tx->data = proto_client_message_serialize(msg);
    if (!tx->data) {
        db_error("serialize msg false");
        return -1;
    }
    int ret = storage_saveTxsInfo(tx);
    cstr_free(tx->data);
    if (ret != 0) {
        db_error("saveTxsInfo false");
        return -1;
    }
    return 0;
}
