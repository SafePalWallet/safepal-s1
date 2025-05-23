#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Sui/sui_sign.c"

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

	SuiSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;
	memset(db, 0, sizeof(DBTxCoinInfo));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	coin_type = msg->coin.type;
	coin_uname = msg->coin.uname;
    if ((char) msg->operation_type == SUI_TRANSFER_MAIN || (char) msg->operation_type == SUI_TRANSFER_TOKEN) {
        if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
            db_error("invalid dtoken name:%s or symbol:%s", msg->token.name, msg->token.symbol);
            return -1;
        }

        name = msg->token.name;
        symbol = msg->token.symbol;
    } else if ((char) msg->operation_type == SUI_DAPP ||
               (char) msg->operation_type == SUI_MSG ||
               (char) msg->operation_type == SUI_SWAP ||
               (char) msg->operation_type == SUI_SWAP2) {
        name = "Data:";
        symbol = res_getLabel(LANG_LABEL_TX_SIGN);
    } else if ((char) msg->operation_type == SUI_NFT) {
        if (is_empty_string(msg->action.nft.name) || is_empty_string(msg->action.nft.name)) {
            db_error("msg->action.nft.name null");
            return -1;
        }

        name = msg->action.nft.name;
        symbol = msg->action.nft.name;
    } else {
        db_error("not support operation_type");
        return -2;
    }
	
	if ((char) msg->operation_type == SUI_TRANSFER_MAIN || (char)msg->operation_type==SUI_TRANSFER_TOKEN) {
		if (proto_check_exchange(&msg->exchange) != 0) {
			db_error("invalid exchange");
			return -3;
		}

		uint8_t decimals = msg->token.decimals;
		uint64_t amount = msg->action.sendCoins.kind.amount;
		double send_value = proto_coin_real_value(amount, decimals);

		db_msg("decimals:%d send_value:%.8lf", decimals, send_value);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
		strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
		view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
	
      	
		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		amount = msg->action.sendCoins.gasData.budget;
	    send_value = proto_coin_real_value(amount, 9);
		snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, 0, 0);
		omit_string(tmpbuf, tmpbuf, 20, 20);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		omit_string(tmpbuf, msg->action.sendCoins.kind.to, 20, 20);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;

	} else if ((char) msg->operation_type == SUI_MSG || (char) msg->operation_type == SUI_DAPP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.dapp.content);
	} else if ((char) msg->operation_type == SUI_NFT) {
		view_add_txt(TXS_LABEL_TOTAL_VALUE, "Object Id");

		char tmpbuf[128];
		GasPayment *object = msg->action.nft.kind.object;
		memset(tmpbuf, 0, sizeof(tmpbuf));
		memcpy(tmpbuf, object->objectId, 7);
		tmpbuf[7] = '.';
		tmpbuf[8] = '.';
		tmpbuf[9] = '.';
		int len = strlen(object->objectId);
		memcpy(tmpbuf + 10, object->objectId + (len - 8), 8);
		db_msg("tmpbuf:%s", tmpbuf);
		view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);


		memset(tmpbuf, 0, sizeof(tmpbuf));
		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		uint64_t amount = msg->action.nft.gasData.budget;
	    double send_value = proto_coin_real_value(amount, 9);
		snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, 0, 0);
		omit_string(tmpbuf, tmpbuf, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
		omit_string(tmpbuf, msg->action.nft.kind.to, 20, 20);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);
		view->total_height = 2 * SCREEN_HEIGHT;
    } else if ((char) msg->operation_type == SUI_SWAP || (char) msg->operation_type == SUI_SWAP2) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.swap.content);

        // view_add_txt(TXS_LABEL_SIMPLE_FEE_TITLE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		// view_add_txt(TXS_LABEL_SIMPLE_FEE_VALUE, msg->action.swap.fee);
	} else {
        db_error("invalid operation_type:%d", msg->operation_type);
        return UNSUPPORT_MSG_UPGRADE_TRY_AGAIN;
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
