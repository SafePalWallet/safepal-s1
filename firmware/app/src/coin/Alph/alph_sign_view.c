#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Alph/alph_sign.c"

#endif

#include "coin_util_hw.h"

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

static int on_sign_show(void *session, DynamicViewCtx *view) {
    char tmpbuf[128];
    int coin_type = 0;
    const char *coin_uname = "";
    const char *name = "";
    const char *symbol = "";
    double send_value = 0;
    uint8_t coin_decimals = 0;
    const CoinConfig *config = NULL;
    int ret;

    coin_state *s = (coin_state *) session;
    if (!s) {
        db_error("invalid session");
        return -1;
    }

    AlphSignTxReq *msg = &s->req;
    DBTxCoinInfo *db = &view->db;
    memset(db, 0, sizeof(DBTxCoinInfo));
    memset(tmpbuf, 0, sizeof(tmpbuf));

    coin_type = msg->coin.type;
    coin_uname = msg->coin.uname;
    if (msg->operation_type == ALPH_TRANSFER) {
        config = getCoinConfig(coin_type, coin_uname);
        if (!config) {
            db_msg("not config type:%d name:%s", msg->coin.type, msg->coin.uname);
            name = msg->coin.uname;
            symbol = msg->coin.uname;
            coin_decimals = 9;
        } else {
            name = config->name;
            symbol = config->symbol;
            coin_decimals = config->decimals;
        }
    } else if ((char) msg->operation_type == ALPH_TOKEN_TRANSFER) { // token
        if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
            db_error("invalid token name:%s or symbol:%s", msg->token.name, msg->token.symbol);
            return -2;
        }
        db_msg("msg->token.name:%s", msg->token.name);
        db_msg("msg->token.symbol:%s", msg->token.symbol);
        db_msg("msg->token.decimals:%d", msg->token.decimals);

        name = msg->token.name;
        symbol = msg->token.symbol;
        coin_decimals = msg->token.decimals;
    } else if ((char) msg->operation_type == ALPH_DAPP) {
        name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
        symbol = msg->action.dapp.appName;
    } else if ((char) msg->operation_type == ALPH_NFT) {
        name = msg->action.sendCoins.appName;
        symbol = msg->action.sendCoins.appName;
    } else {
        db_error("invalid type");
        return -3;
    }

    if ((char) msg->operation_type == ALPH_TRANSFER || (char) msg->operation_type == ALPH_TOKEN_TRANSFER) {
        if (proto_check_exchange(&msg->exchange) != 0) {
            db_error("invalid exchange");
            return -3;
        }

        coin_decimals = msg->token.decimals;
        // amount
        memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.amount.bytes, msg->action.sendCoins.amount.size, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
        db_msg("alph send_value:%.8lf", send_value);

        memset(tmpbuf, 0, sizeof(tmpbuf));
        snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
        view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);

        // fee
        view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
        memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.fee.bytes, msg->action.sendCoins.fee.size, 18, &send_value, tmpbuf, sizeof(tmpbuf));
        db_msg("fee send_value:%.8lf", send_value);

        snprintf(tmpbuf, sizeof(tmpbuf), "%s ALPH", tmpbuf);
        db_msg("feed value:%s", tmpbuf);
        view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

        // address
        view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
        omit_string(tmpbuf, msg->action.sendCoins.from, 26, 11);
        view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

        view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
        omit_string(tmpbuf, msg->action.sendCoins.to, 26, 11);
        view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

        view->total_height = 2 * SCREEN_HEIGHT;
    } else if ((char) msg->operation_type == ALPH_DAPP) {
        db->tx_type = TX_TYPE_APP_SIGN_MSG;
        view->total_height = SCREEN_HEIGHT;
        view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.dapp.content);
    } else if ((char) msg->operation_type == ALPH_NFT) {
        view_add_txt(TXS_LABEL_TOTAL_VALUE, "Contract");

        memset(tmpbuf, 0, sizeof(tmpbuf));
        omit_string(tmpbuf, msg->action.sendCoins.contract, 8, 8);
        view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

        view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
        memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.fee.bytes, msg->action.sendCoins.fee.size, 18, &send_value, tmpbuf, sizeof(tmpbuf));
        db_msg("fee send_value:%.8lf", send_value);
        snprintf(tmpbuf, sizeof(tmpbuf), "%s ALPH", tmpbuf);
        db_msg("feed value:%s", tmpbuf);
        view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

        //address
        view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
        memset(tmpbuf, 0, sizeof(tmpbuf));
        wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, 0, 0);
        omit_string(tmpbuf, tmpbuf, 26, 11);
        view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

        view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));

        omit_string(tmpbuf, msg->action.sendCoins.to, 26, 11);
        view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

        view->total_height = 2 * SCREEN_HEIGHT;
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
