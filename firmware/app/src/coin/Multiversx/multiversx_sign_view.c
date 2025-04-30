#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Multiversx/multiversx_sign.c"

#endif

#include "coin_util_hw.h"

enum {
	TXS_LABEL_TOTAL_VALUE,
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
	const char *coin_uname = NULL;
	const char *name = NULL;
	const char *symbol = NULL;
	int ret;
	uint8_t coin_decimals = 0;
	
	coin_state *s = (coin_state *) session;
	if (!s) {
		db_error("invalid session");
		return -1;
	}

	MultiversxSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;
	memset(db, 0, sizeof(DBTxCoinInfo));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	coin_type = msg->coin.type;
	coin_uname = msg->coin.uname;

	if ((char)msg->operation_type==OP_TYPE_MTOKEN) {
		if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
			db_error("invalid dtoken name:%s or symbol:%s", msg->token.name, msg->token.symbol);
			return -1;
		}
		
		name = msg->token.name;
		symbol = msg->token.symbol;
	} else if((char)msg->operation_type==OP_TYPE_DAPP ){
		name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
		symbol = msg->action.dapp.app_name;
	} else if((char)msg->operation_type==OP_TYPE_MSG){
		name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
		symbol = msg->action.msg.app_name;
	} else{
		const CoinConfig *config = getCoinConfig(msg->coin.type, msg->coin.uname);
		if (!config) {
			db_msg("not config type:%d name:%s", msg->coin.type, msg->coin.uname);
			name = msg->coin.uname;
			symbol = msg->coin.uname;
		}else{
			name = config->name;
			symbol = config->symbol;
		}
	}
	
	if (((char) msg->operation_type == OP_TYPE_MTOKEN) || \
        ((char) msg->operation_type == OP_TYPE_MCOIN) ) {

		if ((char) msg->operation_type == OP_TYPE_MCOIN) {
			coin_decimals = 18;
		} else if ((char) msg->operation_type == OP_TYPE_MTOKEN) {
			coin_decimals = msg->token.decimals;
		}

		memset(tmpbuf, 0, sizeof(tmpbuf));
		view_add_txt(TXS_LABEL_TOTAL_VALUE, msg->action.sendCoins.valueStr);

		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		view_add_txt(TXS_LABEL_FEED_VALUE, msg->action.sendCoins.fee);

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		omit_string(tmpbuf, msg->action.sendCoins.sender, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
		omit_string(tmpbuf, msg->action.sendCoins.receiver, 26, 11);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;
	} else if ((char) msg->operation_type == OP_TYPE_DAPP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.dapp.displayStr);
	} else if ((char) msg->operation_type == OP_TYPE_MSG) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		// view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.msg.displayStr);
		const char *message = msg->action.msg.serialize;
		omit_string(tmpbuf, message, 50, 50);
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, tmpbuf);
	}  else {
        db_error("invalid operation_type:%d", msg->operation_type);
        return UNSUPPORT_MSG_UPGRADE_TRY_AGAIN;
	}
	db->coin_type = coin_type;
	strlcpy(db->coin_name, name, sizeof(db->coin_name));
	strlcpy(db->coin_symbol, symbol, sizeof(db->coin_symbol));
	strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

	view->coin_type = coin_type;
	view->coin_uname = coin_uname;
	view->coin_name = name;
	view->coin_symbol = symbol;

	if (msg->operation_type == OP_TYPE_MCOIN) {
		const char *memo = msg->action.sendCoins.memo;
		db_msg("sendCoins  memo:%s", memo);
        view->total_height = 3 * SCREEN_HEIGHT;
        if (is_printable_str(memo)) {
            view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
            view_add_txt(TXS_LABEL_MEMO_CONTENT, memo);
        } else {
            view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_HEX_TITLE));
            ret = strlen(memo);
            if (ret * 2 < (int) sizeof(tmpbuf)) {
                bin_to_hex((const unsigned char *)memo, ret, tmpbuf);
                view_add_txt(TXS_LABEL_MEMO_CONTENT, tmpbuf);
            } else {
                char *hex = (char *) malloc((ret + 1) * 2);
                if (hex) {
                    memset(hex, 0, (ret + 1) * 2);
                    bin_to_hex((const unsigned char *)memo, ret, hex);
                    view_add_txt(TXS_LABEL_MEMO_CONTENT, hex);
                    free(hex);
                }
            }
        }
    } else {
        view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
        view_add_txt(TXS_LABEL_MEMO_CONTENT, "");
    }

	//save coin info
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
