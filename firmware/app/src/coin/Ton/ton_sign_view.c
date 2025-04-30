#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Ton/ton_sign.c"

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
	uint64_t send_amount = 0;
	int coin_type = 0;
	const char *coin_uname = NULL;
	const char *name = NULL;
	const char *symbol = NULL;
	double send_value = 0;
	int ret;
	char str[128];
	uint8_t coin_decimals = 0;
	const CoinConfig *config = NULL;
	
	coin_state *s = (coin_state *) session;
	if (!s) {
		db_error("invalid session");
		return -1;
	}

	TonSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;
	memset(db, 0, sizeof(DBTxCoinInfo));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	coin_type = msg->coin.type;
	coin_uname = msg->coin.uname;
	if ((char)msg->operation_type==TON_TRANSFER) {
		if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
			db_error("invalid dtoken name:%s or symbol:%s", msg->token.name, msg->token.symbol);
			return -1;
		}
		
		name = msg->token.name;
		symbol = msg->token.symbol;
	} else if((char)msg->operation_type==TON_DAPP ){
		name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
		symbol = msg->action.dapp.app_name;
	} else if((char)msg->operation_type==TON_MSG){
		name = res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG);
		symbol = msg->action.msg.app_name;
	} else if((char)msg->operation_type==TON_NFT){
		name = msg->action.sendCoins.app_name;
		symbol = msg->action.sendCoins.app_name;
	} else{
		db_msg("invalid type");
		return -2;
	}
	
	if ((char) msg->operation_type == TON_TRANSFER) {
		if (proto_check_exchange(&msg->exchange) != 0) {
			db_error("invalid exchange");
			return -3;
		}

		coin_decimals = msg->token.decimals;
		//amount
		memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.amount.bytes, msg->action.sendCoins.amount.size, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
		db_msg("ton send_value:%.8lf", send_value);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
		view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);

        //fee
		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.fee.bytes, msg->action.sendCoins.fee.size, 9, &send_value, tmpbuf, sizeof(tmpbuf));
		db_msg("fee send_value:%.8lf", send_value);


		snprintf(tmpbuf, sizeof(tmpbuf), "%s TON", tmpbuf);
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

        if (strlen(msg->action.sendCoins.payload) > 0) {
			view->total_height = 3 * SCREEN_HEIGHT;
			//memo        
			view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
			view_add_txt(TXS_LABEL_MEMO_CONTENT, msg->action.sendCoins.payload);
		} else {
			view->total_height = 2 * SCREEN_HEIGHT;
		}
	} else if ((char) msg->operation_type == TON_DAPP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.dapp.content);
	} else if ((char) msg->operation_type == TON_MSG) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.msg.message);
	} else if ((char) msg->operation_type == TON_NFT) {
		coin_decimals = 9;

		view_add_txt(TXS_LABEL_TOTAL_VALUE, "Contract Address");

		memset(tmpbuf, 0, sizeof(tmpbuf));
		omit_string(tmpbuf, msg->action.sendCoins.contract, 8, 8);
		view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
        ret = bignum2double((const unsigned char *) msg->action.sendCoins.fee.bytes, msg->action.sendCoins.fee.size, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
		db_msg("fee send_value:%.8lf", send_value);
		snprintf(tmpbuf, sizeof(tmpbuf), "%s TON", tmpbuf);
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
	} else {
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
