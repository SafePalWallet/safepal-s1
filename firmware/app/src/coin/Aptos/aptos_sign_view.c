#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Aptos/aptos_sign.c"

#endif

#include "coin_util_hw.h"

enum {
	TXS_LABEL_TOTAL_VALUE,
	TXS_LABEL_TOTAL_MONEY,
	TXS_LABEL_FEED_TILE,
	TXS_LABEL_FEED_VALUE,
	TXS_LABEL_GAS_LIMIT,
	TXS_LABEL_GAS_PRICE,
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
		MK_eth_label_total_value,
		MK_eth_label_total_money,
		MK_eth_label_feed_tile,
		MK_eth_label_feed_value,
		MK_eth_label_gas_limit,
		MK_eth_label_gas_price,
		MK_eth_label_payfrom_title,
		MK_eth_label_payfrom_address,
		MK_eth_label_payto_title,
		MK_eth_label_payto_address,
		MK_eth_label_data_title,
		MK_eth_label_data_content,
		MK_eth_label_app_msg_value,
};

static int on_sign_show(void *session, DynamicViewCtx *view) {
	char tmpbuf[128];
	int coin_type = 0;
	const char *coin_uname = NULL;
	const char *name = NULL;
	const char *symbol = NULL;
	int ret;
	
	coin_state *s = (coin_state *) session;
	if (!s) {
		db_error("invalid session");
		return -1;
	}

	AptosSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;
	memset(db, 0, sizeof(DBTxCoinInfo));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	coin_type = msg->coin.type;
	coin_uname = msg->coin.uname;
	if (((char)msg->operation_type==APT_TRANSFER)) {
		if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
			db_error("invalid dtoken name:%s or symbol:%s", msg->token.name, msg->token.symbol);
			return -1;
		}
		
		name = msg->token.name;
		symbol = msg->token.symbol;
	} else if((char)msg->operation_type==APT_DAPP || (char)msg->operation_type==APT_MSG){
		name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
		symbol = (char)msg->operation_type==APT_DAPP ? msg->action.dapp.app_name : msg->action.msg.app_name;
	} else{
	    db_error("not support operation_type");
		return -1;
	}
	
	if (((char) msg->operation_type == APT_TRANSFER) ) {
		if (proto_check_exchange(&msg->exchange) != 0) {
			db_error("invalid exchange");
			return -1;
		}
		uint8_t hasArguments = is_not_empty_string(msg->action.sendCoins.arguments);
		const char *money_symbol = proto_get_money_symbol(&msg->exchange);

		db_msg("money_symbol:%s", money_symbol);

		uint8_t coin_decimals = msg->token.decimals;
        const char *tempAmount = msg->action.sendCoins.amount;
		if(hasArguments) {
			const char *amount = (tempAmount[0] == '0' && tempAmount[1] == 'x') ? (tempAmount + 2) : tempAmount;
			db_msg("amount:%s",amount);
			char tmpAmount[128];
			int len = hex_to_bin((char *)amount, strlen(amount), (unsigned char *)tmpAmount, strlen(amount)/2);
			if (len < 0) {
				db_error("out error");
				return -5;
			}
			db_msg("tmpAmount:%s",debug_bin_to_hex((const char *)tmpAmount, strlen(amount)/2));

			memset(tmpbuf, 0, sizeof(tmpbuf));
			double send_value = 0;
			ret = bignum2double((const unsigned char *) tmpAmount, strlen(amount)/2, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));

			db_msg("coin_decimals:%d", coin_decimals);
			db_msg("send_value:%.8lf", send_value);

			memset(tmpbuf, 0, sizeof(tmpbuf));
			snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
			view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
		} else if (!hasArguments) {
			view_add_txt(TXS_LABEL_TOTAL_VALUE, "contract");
			memset(tmpbuf, 0, sizeof(tmpbuf));
			omit_string(tmpbuf, msg->action.sendCoins.typeArguments, 6, 14);
		    view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
		}
      	
		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));

	    ret = bignum2double((const unsigned char *) msg->fee.bytes, msg->fee.size, 8, NULL, tmpbuf, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%s APT", tmpbuf);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, 0, 0);
		omit_string(tmpbuf, tmpbuf, 20, 20);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

        if(hasArguments) {
			view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			omit_string(tmpbuf, msg->action.sendCoins.arguments, 20, 20);
			view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);
		}

		view->total_height = 2 * SCREEN_HEIGHT;

	} else if ((char)msg->operation_type==APT_DAPP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		const char *message = msg->action.dapp.content;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, message);
	} else if ((char)msg->operation_type==APT_MSG) {
		db->tx_type = TX_TYPE_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		const char *message = msg->action.msg.message;
        int len = strlen(message);
		char *str = (char *) malloc(sizeof(char) * len/2);
		format_data_to_hex((const unsigned char *)message, len, str, len/2);
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, str);
	    free(str);
	} else {

	}

	db_msg("coin_type:%d", coin_type);
	db_msg("name:%s", name);
	db_msg("symbol:%s", symbol);
	db_msg("coin_uname:%s", coin_uname);

	db->coin_type = coin_type;
	strlcpy(db->coin_name, name, sizeof(db->coin_name));
	strlcpy(db->coin_symbol, symbol, sizeof(db->coin_symbol));
	strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

	view->coin_type = coin_type;
	view->coin_uname = coin_uname;
	view->coin_name = name;
	view->coin_symbol = symbol;

	//save coin info
	if (view->msg_from == MSG_FROM_QR_APP) {
		if (!storage_isCoinExist(coin_type, coin_uname)) {
			DBCoinInfo dbinfo;
			memset(&dbinfo, 0, sizeof(dbinfo));
			dbinfo.type = (uint8_t) coin_type;
			dbinfo.curv = coin_get_curv_id(coin_type, coin_uname);
			dbinfo.decimals = msg->token.decimals;
			strncpy(dbinfo.uname, coin_uname, COIN_UNAME_MAX_LEN);
			strncpy(dbinfo.name, name, COIN_NAME_MAX_LEN);
			strncpy(dbinfo.symbol, symbol, COIN_SYMBOL_MAX_LEN);
			storage_save_coin_dbinfo(&dbinfo);
		}
	}
	return 0;
}


#endif
