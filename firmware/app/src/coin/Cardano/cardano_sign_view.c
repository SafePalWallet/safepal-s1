#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Cardano/cardano_sign.c"

#endif

#include "coin_util_hw.h"

enum {
    TXS_LABEL_TOTAL_TILE,
    TXS_LABEL_TOTAL_VALUE,
    TXS_LABEL_TOTAL_MONEY,
    TXS_LABEL_FEED_TILE,
    TXS_LABEL_FEED_VALUE,
    TXS_LABEL_PAYFROM_TITLE,
    TXS_LABEL_PAYFROM_ADDRESS,
    TXS_LABEL_PAYTO_TITLE,
    TXS_LABEL_PAYTO_ADDRESS,
    TXS_LABEL_DATA_TITLE,
    TXS_LABEL_DATA_CONTENT,
    TXS_LABEL_APP_MSG_VALUE,
    TXS_LABEL_MAXID,
};

static int label_mk_map[TXS_LABEL_MAXID] = {
        MK_txs_label_total_tile,
        MK_txs_label_total_value,
        MK_txs_label_total_money,
        MK_txs_label_feed_tile,
        MK_txs_label_feed_value,
        MK_eth_label_payfrom_title,
        MK_eth_label_payfrom_address,
        MK_eth_label_payto_title,
        MK_eth_label_payto_address,
        MK_eth_label_data_title,
        MK_eth_label_data_content,
        MK_eth_label_app_msg_value,
};

static int check_add_value(int64_t *rs, int64_t v) {
    if (v <= 0) return 0;
    if (LLONG_MAX - *rs < v) {
        return 0;
    } else {
        *rs += v;
        return 1;
    }
}

static int on_sign_show(void *session, DynamicViewCtx *view) {
    char tmpbuf[128] = {0};

    coin_state *s = (coin_state *) session;
    if (!s) {
        db_error("invalid session");
        return -101;
    }

    CardanoSignTxReq *msg = &s->req;
    DBTxCoinInfo *db = &view->db;
    memset(db, 0, sizeof(DBTxCoinInfo));
    if (proto_check_exchange(&msg->exchange) != 0) {
        db_error("invalid exchange");
        return -102;
    }
    const CoinConfig *coinConfig = getCoinConfig(COIN_TYPE_CARDANO, "ADA");
    if (NULL == coinConfig) {
        db_error("not support type:%d name:%s", msg->coin.type, msg->coin.uname);
        return -103;
    }

    const char *name = NULL;
    const char *symbol = NULL;
    int coin_type = msg->coin.type;
    const char *coin_uname = msg->coin.uname;
    uint8_t coin_decimals = msg->token.decimals;
    const char *toAdress = NULL;
    const char *changeAdress = NULL;
    uint8_t haveChange = 0;
    if (msg->operation_type == ADA_TOKEN) {
        name = msg->token.name;
        symbol = msg->token.symbol;
    } else {
        name = coinConfig->name;
        symbol = coinConfig->symbol;
    }
    db_msg("name:%s, symbol:%s, coin_uname:%s, coin_type:%#x", name, symbol, coin_uname, coin_type);

    db->coin_type = coin_type;
    strlcpy(db->coin_name, name, sizeof(db->coin_name));
    strlcpy(db->coin_symbol, symbol, sizeof(db->coin_symbol));
    strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

    view->total_height = SCREEN_HEIGHT;
    view->coin_type = coin_type;
    view->coin_uname = coin_uname;
    view->coin_name = name;
    view->coin_symbol = symbol;

    int64_t send_main_coin_value = 0;
    int64_t send_token_value = 0;

    int64_t value;
    for (int i = 0; i < msg->output_n; i++) {
        value = msg->outputs[i].value;
        if (is_empty_string(msg->outputs[i].address)) {
            db_error("invalid output no:%d value:%lld", i, value);
            break;
        }

        if (!msg->outputs[i].is_change_address) {
            if (!check_add_value(&send_main_coin_value, value)) {
                db_error("invalid output no:%d value:%lld", i, value);
                break;
            }

            view->total_height = 2 * SCREEN_HEIGHT;
            toAdress = msg->outputs[i].address;

            if (msg->operation_type == ADA_TOKEN) {
                if (msg->outputs[i].assets_n != 1) {
                    db_error("Only one token can be transferred at a time");
                    return -104;
                }

                CardanoAssets assets = msg->outputs[i].assets[0];

                if (!check_add_value(&send_token_value, assets.amount)) {
                    db_error("invalid output token");
                    break;
                }
            }
        } else {
            haveChange ++;
            changeAdress = msg->outputs[i].address;
        }
    }

    db_msg("send_main_coin_value:%d send_token_value:%d", send_main_coin_value, send_token_value);
    if (msg->operation_type == ADA_TOKEN) {
        format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_token_value, coin_decimals);
        snprintf(tmpbuf, sizeof(tmpbuf), "%s %s", tmpbuf, symbol);
        view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
        strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
        snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", proto_coin_currency_value(&msg->exchange, send_token_value, coin_decimals));
    }

    format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_main_coin_value, coinConfig->decimals);
    snprintf(tmpbuf, sizeof(tmpbuf), "%s %s", tmpbuf, coinConfig->symbol);
    if (msg->operation_type == ADA_TOKEN) {
        view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
    } else {
        view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
        strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
        snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", proto_coin_currency_value(&msg->exchange, send_main_coin_value, coin_decimals));
    }
    const char *money_symbol = proto_get_money_symbol(&msg->exchange);
    strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

    view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
    if (strlen(toAdress) >= 42) {
        memzero(tmpbuf, sizeof(tmpbuf));
        omit_string(tmpbuf, toAdress, 10, 27);
        view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);
    } else {
        view_add_txt(TXS_LABEL_PAYTO_ADDRESS, toAdress);
    }

    view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
    format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->fee, coinConfig->decimals);
    snprintf(tmpbuf, sizeof(tmpbuf), "%s %s", tmpbuf, coinConfig->symbol);
    view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

    if(haveChange > 0) {
        view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_CHANGE_TITLE));
        if (strlen(changeAdress) >= 42) {
            memzero(tmpbuf, sizeof(tmpbuf));
            omit_string(tmpbuf, changeAdress, 10, 27);
            view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);
        } else {
            view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, changeAdress);
        }
    }

    //save coin info
    if (coin_type && view->msg_from == MSG_FROM_QR_APP) {
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
