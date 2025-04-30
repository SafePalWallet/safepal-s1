#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Ckb/ckb_sign.c"

#endif

#include "coin_util_hw.h"

#define CKB_SHORT_ADDR_LEN (42)

enum {
    TXS_LABEL_TOTAL_TILE,
    TXS_LABEL_TOTAL_VALUE,
    TXS_LABEL_TOTAL_MONEY,
    TXS_LABEL_FEED_TILE,
    TXS_LABEL_FEED_VALUE,
    TXS_LABEL_TITLE2,
    TXS_LABEL_VALUE2,
    TXS_LABEL_TITLE3,
    TXS_LABEL_AMOUNT,
    TXS_LABEL_MAXID,
};

static int label_mk_map[TXS_LABEL_MAXID] = {
        MK_txs_label_total_tile,
        MK_txs_label_total_value,
        MK_txs_label_total_money,
        MK_txs_label_feed_tile,
        MK_txs_label_feed_value,
        MK_cmsg_label_title2,
        MK_cmsg_label_value2,
        MK_cmsg_label_title3,
        MK_cmsg_label_amount,
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

static int on_sign_show_more(void *session, DynamicViewCtx *view) {
    char tmpbuf[128];
    coin_state *s = (coin_state *) session;
    if (!s) {
        db_error("invalid session");
        return -1;
    }
    CkbSignTxReq *msg = &s->req;

    const CoinConfig *coinConfig = getCoinConfig(COIN_TYPE_CKB, msg->coin.uname);
    if (NULL == coinConfig) {
        db_error("not support type:%d name:%s", msg->coin.type, msg->coin.uname);
        return -181;
    }
    if (msg->input_n <= 0) {
        db_error("invalid input n:%d", msg->input_n);
        return -101;
    }

    uint32_t i;
    int input_n = msg->input_n;
    int change_count = 0;
    uint16_t coin_decimals = coinConfig->decimals;

    for (i = 0; i < msg->output_n; i++) {
        if (msg->outputs[i].is_change_address & CKB_OUTPUT_FLAG_CHANGE) {
            change_count++;
        }
    }
    int item_count = input_n + change_count;
    int mScreenHeight = SCREEN_HEIGHT;
    int first_output_offset = 0;
    int output_item_height = res_getInt(MK_txs_sign, "output_item_height", 0);
    if (!output_item_height) output_item_height = mScreenHeight;

    int num_perpage = mScreenHeight / output_item_height;
    //move ok to right pos
    int total_height = first_output_offset + mScreenHeight * ((item_count + num_perpage - 1) / num_perpage);
    db_msg("total_height:%d mScreenHeight:%d item_count:%d num_perpage:%d output_item_height:%d",
           total_height, mScreenHeight, item_count, num_perpage, output_item_height);
    view->total_height = total_height;
    view->coin_type = COIN_TYPE_CKB;
    view->coin_uname = coinConfig->uname;
    view->coin_name = coinConfig->name;
    view->coin_symbol = coinConfig->symbol;

    int item_index = 0;
    int viewid = 100;
    int offset;
    if (change_count > 0) {
        for (i = 0; i < msg->output_n; i++) {
            if (!(msg->outputs[i].is_change_address & CKB_OUTPUT_FLAG_CHANGE)) continue;

            offset = first_output_offset + (item_index / num_perpage) * mScreenHeight + (item_index % num_perpage) * output_item_height;

            dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_index, viewid++, res_getLabel(LANG_LABEL_TXS_CHANGE_TITLE), offset);
            db_msg("outputs.address.length %d %d", strlen(msg->outputs[i].address), CKB_SHORT_ADDR_LEN);
            if (strlen(msg->outputs[i].address) > CKB_SHORT_ADDR_LEN) {
                omit_string(tmpbuf, msg->outputs[i].address, 10, 27);
                dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_address, viewid++, tmpbuf, offset);
            } else {
                dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_address, viewid++, msg->outputs[i].address, offset);
            }
            format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->outputs[i].capacity, coin_decimals);
            dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_value, viewid++, tmpbuf, offset);

            item_index++;
        }
    }

    int addr_index = 0;
    if (input_n) {
        for (i = 0; i < msg->input_n; i++) {
            if (input_n > 1) {
                snprintf(tmpbuf, sizeof(tmpbuf), res_getLabel(LANG_LABEL_TXS_FROM_INDEX), addr_index + 1);
            } else {
                strlcpy(tmpbuf, res_getLabel(LANG_LABEL_TXS_FROM_TITLE), sizeof(tmpbuf));
            }
            offset = first_output_offset + (item_index / num_perpage) * mScreenHeight + (item_index % num_perpage) * output_item_height;
            dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_index, viewid++, tmpbuf, offset);

            db_msg("inputs.address.length %d %d", strlen(msg->inputs[i].address), CKB_SHORT_ADDR_LEN);
            if (strlen(msg->inputs[i].address) > CKB_SHORT_ADDR_LEN) {
                omit_string(tmpbuf, msg->inputs[i].address, 10, 27);
                dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_address, viewid++, tmpbuf, offset);
            } else {
                dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_address, viewid++, msg->inputs[i].address, offset);
            }

            format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->inputs[i].value, coin_decimals);
            dwin_add_txt_offset(view, MK_txs_dlabel_pay_from_value, viewid++, tmpbuf, offset);

            addr_index++;
            item_index++;
        }
    }
    return 0;
}

static int on_sign_show(void *session, DynamicViewCtx *view) {
    char tmpbuf[128];
    if (view->show_more) {
        return on_sign_show_more(session, view);
    }
    coin_state *s = (coin_state *) session;
    if (!s) {
        db_error("invalid session");
        return -1;
    }

    CkbSignTxReq *msg = &s->req;
    DBTxCoinInfo *db = &view->db;

    memset(db, 0, sizeof(DBTxCoinInfo));
    if (proto_check_exchange(&msg->exchange) != 0) {
        db_error("invalid exchange");
        return -102;
    }
    const CoinConfig *coinConfig = getCoinConfig(COIN_TYPE_CKB, msg->coin.uname);
    if (NULL == coinConfig) {
        db_error("not support type:%d name:%s", msg->coin.type, msg->coin.uname);
        return -181;
    }
    storage_save_coin_info(coinConfig);
    int64_t in_value = 0;
    int64_t out_value = 0;
    int change_item_index = -1;
    int64_t change_value = 0;
    int64_t feed_value = 0;
    int64_t send_value = 0;
    int out_item_count = 0;
    int64_t value;
    int err = 0;
    uint16_t coin_decimals = coinConfig->decimals;
    db_msg("decimals:%d ", coin_decimals);
    do {
        if (msg->input_n <= 0) {
            err = 101;
            db_error("input:0");
            break;
        }
        if (msg->output_n <= 0) {
            err = 102;
            db_error("output:0");
            break;
        }
        for (uint32_t i = 0; i < msg->input_n; i++) {
            value = msg->inputs[i].value;
            if (is_empty_string(msg->inputs[i].address)) {
                err = 110;
                db_error("invalid input no:%d value:%lld", i, value);
                break;
            }
            if (!check_add_value(&in_value, value)) {
                err = 112;
                db_error("invalid input no:%d value:%lld", i, value);
                break;
            }
            if (is_empty_string(msg->inputs[i].tx_hash)) {
                err = 114;
                db_error("input no:%d empty path", i);
                break;
            }
        }
        if (err) {
            break;
        }
        for (uint32_t i = 0; i < msg->output_n; i++) {
            value = msg->outputs[i].capacity;
            if (is_empty_string(msg->outputs[i].address)) {
                err = 130;
                db_error("invalid input no:%d value:%lld", i, value);
                break;
            }
            if (!check_add_value(&out_value, value)) {
                err = 131;
                db_error("invalid output no:%d value:%lld", i, value);
                break;
            }
            if (msg->outputs[i].is_change_address & CKB_OUTPUT_FLAG_CHANGE) {
                if (change_item_index != -1) {
                    err = 132;
                    db_error("more change address");
                    break;
                }
                if (is_empty_string(msg->outputs[i].address)) {
                    err = 133;
                    db_error("empty change path");
                    break;
                }
                change_item_index = i;
                change_value += value;
            } else {
                out_item_count++;
            }
        }
        if (err) {
            break;
        }
        if (out_value >= in_value) {
            err = 139;
            db_error("out_value:%lld > in_value:%lld", out_value, in_value);
            break;
        }
    } while (0);

    if (err) {
        return err > 0 ? -err : err;
    }

    feed_value = in_value - out_value;
    send_value = in_value - change_value;

    const char *money_symbol = proto_get_money_symbol(&msg->exchange);
    db_msg("in_value:%lld out_value:%lld change_value:%lld feed_value:%lld", in_value, out_value, change_value, feed_value);

    if (msg->operation_type == OP_TYPE_TRANSFER) {
        int mScreenHeight = SCREEN_HEIGHT;
        int first_output_offset = res_getInt(MK_txs_sign, "first_output_offset", 0);
        int output_item_height = res_getInt(MK_txs_sign, "output_item_height", 0);
        if (!output_item_height) output_item_height = mScreenHeight;

        int num_perpage = mScreenHeight / output_item_height;

        //move ok to right pos
        int total_height = first_output_offset + mScreenHeight * ((out_item_count + num_perpage - 1) / num_perpage);

        db_msg("first_output_offset:%d output_item_height:%d out_item_count:%d total_height:%d num_perpage:%d",
               first_output_offset, output_item_height, out_item_count, total_height, num_perpage);

        db->coin_type = COIN_TYPE_CKB;
        strlcpy(db->coin_name, coinConfig->name, sizeof(db->coin_name));
        strlcpy(db->coin_symbol, coinConfig->symbol, sizeof(db->coin_symbol));
        strlcpy(db->coin_uname, coinConfig->uname, sizeof(db->coin_uname));

        view->has_more = 1;
        view->total_height = total_height;
        view->coin_type = COIN_TYPE_CKB;
        view->coin_uname = coinConfig->uname;
        view->coin_name = coinConfig->name;
        view->coin_symbol = coinConfig->symbol;

        view_add_txt(TXS_LABEL_TOTAL_TILE, res_getLabel(LANG_LABEL_TXS_VALUE_TITLE));

        format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_value, coin_decimals);
        view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);

        strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
        snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", proto_coin_currency_value(&msg->exchange, send_value, coin_decimals));
        strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

        snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%s", money_symbol, db->currency_value);
        view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

        view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
        format_coin_real_value(tmpbuf, sizeof(tmpbuf), feed_value, coin_decimals);
        view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

        int item_index = 0;
        int viewid = 100;
        int offset;
        for (uint32_t i = 0; i < msg->output_n; i++) {
            if (!(msg->outputs[i].is_change_address & CKB_OUTPUT_FLAG_CHANGE)) {
                offset = first_output_offset + (item_index / num_perpage) * mScreenHeight + (item_index % num_perpage) * output_item_height;
                if (out_item_count > 1) {
                    snprintf(tmpbuf, sizeof(tmpbuf), res_getLabel(LANG_LABEL_TXS_PAYTO_INDEX), item_index + 1);
                } else {
                    strlcpy(tmpbuf, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE), sizeof(tmpbuf));
                }
                dwin_add_txt_offset(view, MK_txs_dlabel_payto_index, viewid++, tmpbuf, offset);

                db_msg("sendOutItem.address.length %d %d", strlen(msg->outputs[i].address), CKB_SHORT_ADDR_LEN);
                if (strlen(msg->outputs[i].address) > CKB_SHORT_ADDR_LEN) {
                    omit_string(tmpbuf, msg->outputs[i].address, 10, 27);
                    dwin_add_txt_offset(view, MK_txs_dlabel_payto_address, viewid++, tmpbuf, offset);
                } else {
                    dwin_add_txt_offset(view, MK_txs_dlabel_payto_address, viewid++, msg->outputs[i].address, offset);
                }

                format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->outputs[i].capacity, coin_decimals);
                dwin_add_txt_offset(view, MK_txs_dlabel_payto_value, viewid++, tmpbuf, offset);

                item_index++;
            }
        }
    } else if (msg->operation_type == OP_TYPE_UNLOCK) {
        db->tx_type = TX_TYPE_APP_APPROVAL;
        db->coin_type = COIN_TYPE_CKB;
        strlcpy(db->coin_symbol, "sign message", 13);
        strlcpy(db->coin_uname, coinConfig->uname, sizeof(db->coin_uname));

        view->total_height = SCREEN_HEIGHT;
        view->coin_type = COIN_TYPE_CKB;
        view->coin_uname = coinConfig->uname;
        //view->coin_name = coinConfig->name;
        view->coin_symbol = "sign message";

        view_add_txt(TXS_LABEL_TITLE2, res_getLabel(LANG_LABEL_RECEIVE_ADDRESS));
        if (strlen(msg->outputs[0].address) > CKB_SHORT_ADDR_LEN) {
            omit_string(tmpbuf, msg->outputs[0].address, 10, 27);
            view_add_txt(TXS_LABEL_VALUE2, tmpbuf);
        } else {
            view_add_txt(TXS_LABEL_VALUE2, msg->inputs[0].address);
        }

        int off = 0;
        view_add_txt_off(TXS_LABEL_TITLE3, res_getLabel(LANG_LABEL_ORDER_AMOUNT), off);
        format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->inputs[0].value, coin_decimals);
        view_add_txt_off(TXS_LABEL_AMOUNT, tmpbuf, off);

        strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
        snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", proto_coin_currency_value(&msg->exchange, send_value, coin_decimals));
        strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));
    } else {
        db_error("invalid operation_type:%d", msg->operation_type);
        return UNSUPPORT_MSG_UPGRADE_TRY_AGAIN;
    }

    return 0;
}

#endif
