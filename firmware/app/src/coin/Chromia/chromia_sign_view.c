#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Chromia/chromia_sign.c"

#endif

#include "coin_util_hw.h"
#include <ctype.h>

enum {
    TXS_LABEL_TOTAL_VALUE,
    TXS_LABEL_TOTAL_MONEY,
    TXS_LABEL_FEED_TILE,
    TXS_LABEL_FEED_VALUE,
    TXS_LABEL_PAYFROM_TITLE,
    TXS_LABEL_PAYFROM_ADDRESS,
    TXS_LABEL_PAYTO_TITLE,
    TXS_LABEL_PAYTO_ADDRESS,
    TXS_LABEL_APP_MSG_VALUE,
    TXS_LABEL_MEMO_TITLE,
    TXS_LABEL_MEMO_CONTENT,
    TXS_LABEL_MAXID,
};

static int label_mk_map[TXS_LABEL_MAXID] = {
        MK_eth_label_total_value,
        MK_eth_label_total_money,
        MK_eth_label_feed_tile,
        MK_eth_label_feed_value,
        MK_eth_label_payfrom_title,
        MK_eth_label_payfrom_address,
        MK_eth_label_payto_title,
        MK_eth_label_payto_address,
        MK_eth_label_app_msg_value,
        MK_eth_label_data_title,
        MK_eth_label_data_content,
};

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
}

static int on_sign_show(void *session, DynamicViewCtx *view) {
    char tmpbuf[128] = {0};
    int coin_type = 0;
    const char *coin_uname = "";
    const char *name = "";
    const char *symbol = "";
    uint8_t coin_decimals = 0;
    int ret;

    coin_state *s = (coin_state *) session;
    if (!s) {
        db_error("invalid session");
        return -1;
    }

    ChrSignTxReq *msg = &s->req;
    DBTxCoinInfo *db = &view->db;
    memset(db, 0, sizeof(DBTxCoinInfo));

    coin_type = msg->coin.type;
    coin_uname = msg->coin.uname;
    if (msg->operation_type == CHR_TYPE_TRANSFER) {
        name = msg->coin.uname;
        symbol = msg->coin.uname;
        coin_decimals = msg->token.decimals;
    } else if ((char) msg->operation_type == CHR_TYPE_REGISTER) {
        name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
        symbol = "REGISTER";
    } else {
        db_error("invalid type");
        return -3;
    }

    if ((char) msg->operation_type == CHR_TYPE_TRANSFER) {
        if (proto_check_exchange(&msg->exchange) != 0) {
            db_error("invalid exchange");
            return -3;
        }

        view_add_txt(TXS_LABEL_TOTAL_VALUE, msg->action.sendCoins.amount);

        // fee
        view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
        view_add_txt(TXS_LABEL_FEED_VALUE, "0 CHR");

        // address
        memset(tmpbuf, 0x00, sizeof(tmpbuf));
        view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
        omit_string(tmpbuf, msg->action.sendCoins.from, 26, 11);
        toLowerCase(tmpbuf);
        view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

        memset(tmpbuf, 0x00, sizeof(tmpbuf));
        view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
        memcpy(tmpbuf, msg->action.sendCoins.to + 2, 64);
        toLowerCase(tmpbuf);
        omit_string(tmpbuf, tmpbuf, 26, 11);
        view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

        view->total_height = 2 * SCREEN_HEIGHT;
    } else if ((char) msg->operation_type == CHR_TYPE_REGISTER) {
        memset(tmpbuf, 0x00, sizeof(tmpbuf));
        db->tx_type = TX_TYPE_APP_APPROVAL;
        view->total_height = SCREEN_HEIGHT;
        view_add_txt(TXS_LABEL_TOTAL_VALUE, "BlockchainRID:");
        omit_string(tmpbuf, msg->action.registerAccount.blockchainRid, 8, 9);
        view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
    }

    db->coin_type = coin_type;
    strlcpy(db->coin_name, name, sizeof(db->coin_name));
    strlcpy(db->coin_symbol, symbol, sizeof(db->coin_symbol));
    strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

    db_msg("db coin_type:%d coin_uname:%s coin_name:%s coin_symbol:%s", db->coin_type, db->coin_uname, db->coin_name, db->coin_symbol);

    view->coin_type = coin_type;
    view->coin_uname = coin_uname;
    view->coin_name = name;
    view->coin_symbol = symbol;

    // save coin info
    if (view->msg_from == MSG_FROM_QR_APP) {
        if (!storage_isCoinExist(coin_type, coin_uname)) {
            DBCoinInfo dbinfo;
            memset(&dbinfo, 0, sizeof(dbinfo));
            dbinfo.type = (uint8_t) coin_type;
            dbinfo.curv = coin_get_curv_id(coin_type, coin_uname);
            dbinfo.decimals = coin_decimals;
            strncpy(dbinfo.uname, coin_uname, COIN_UNAME_MAX_LEN);
            strncpy(dbinfo.name, name, COIN_NAME_MAX_LEN);
            strncpy(dbinfo.symbol, symbol, COIN_SYMBOL_MAX_LEN);
            storage_save_coin_dbinfo(&dbinfo);
        }
    }
    return 0;
}

#endif
