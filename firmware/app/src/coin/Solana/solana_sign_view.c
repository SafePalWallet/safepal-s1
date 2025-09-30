#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Solana/sol_sign.c"

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
	TXS_LABEL_SIMPLE_FEE_TITLE,
	TXS_LABEL_SIMPLE_FEE_VALUE,
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
		MK_eth_label_simple_fee_title,
		MK_eth_label_simple_fee_value,
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

	SolSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;
	memset(db, 0, sizeof(DBTxCoinInfo));
	memset(tmpbuf, 0, sizeof(tmpbuf));

	coin_type = msg->coin.type;
	coin_uname = msg->coin.uname;
	if (((char)msg->operation_type==OP_TYPE_TOKENTRANSFER) || ((char)msg->operation_type==OP_TYPE_CREATE_AND_TRANSFER) ||
	    ((char)msg->operation_type==OP_TYPE_TOEKN2022_TRANSFER) || ((char)msg->operation_type==OP_TYPE_TOEKN2022_CREATE_AND_TRANSFER) ||
		((char)msg->operation_type==OP_TYPE_REG_NONCE) || ((char)msg->operation_type==OP_TYPE_TRANSFER_HARDWARE) ||
		((char)msg->operation_type==OP_TYPE_TOKENTRANSFER_HARDWARE) || ((char)msg->operation_type==OP_TYPE_TOKENTRANSFER_HARDWARE_CREATE) ||
		((char)msg->operation_type==OP_TYPE_TOEKN2022_TRANSFER_HARDWARE) || ((char)msg->operation_type==OP_TYPE_TOEKN2022_TRANSFER_HARDWARE_CREATE)

	) {
		if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
			db_error("invalid dtoken name:%s or symbol:%s", msg->token.name, msg->token.symbol);
			return -1;
		}
		
		name = msg->token.name;
		symbol = msg->token.symbol;
	} else if (((char)msg->operation_type==OP_TYPE_NFT_TRANSFER) || ((char)msg->operation_type==OP_TYPE_NFT_CREATE_AND_TRANSFER)) {
		if (is_empty_string(msg->action.token_transfer.app_name) || is_empty_string(msg->action.token_transfer.app_name)) {
			db_error("msg->action.token_transfer.app_name null");
			return -1;
		}
		
		name = msg->action.token_transfer.app_name;
		symbol = msg->action.token_transfer.app_name;
	} else if ((char)msg->operation_type==OP_TYPE_MNFT) {
		if (is_empty_string(msg->action.mNFT.app_name) || is_empty_string(msg->action.mNFT.app_name)) {
			db_error("msg->action.mNFT.app_name null");
			return -1;
		}
		
		name = msg->action.mNFT.app_name;
		symbol = msg->action.mNFT.app_name;
	} else if ((char)msg->operation_type==OP_TYPE_COMPRESSED_NFT_TRANSFER) {
		if (is_empty_string(msg->action.compressedNFT.app_name) || is_empty_string(msg->action.compressedNFT.app_name)) {
			db_error("msg->action.compressedNFT.app_name null");
			return -1;
		}
		
		name = msg->action.compressedNFT.app_name;
		symbol = msg->action.compressedNFT.app_name;
    } else if ((char) msg->operation_type == OP_TYPE_DAPP ||
               (char) msg->operation_type == OP_TYPE_MSG ||
               (char) msg->operation_type == OP_TYPE_SWAP) {
        name = "Data:";
        symbol = res_getLabel(LANG_LABEL_TX_SIGN);
    } else {
		config = getCoinConfig(msg->coin.type, msg->coin.uname);
		if (!config) {
			db_msg("not config type:%d name:%s", msg->coin.type, msg->coin.uname);
			name = msg->coin.uname;
			symbol = msg->coin.uname;
		}else{
			name = config->name;
			symbol = config->symbol;
		}
	}
	
	if (((char) msg->operation_type == OP_TYPE_TRANSFER) || \
        ((char) msg->operation_type == OP_TYPE_TOKENTRANSFER) || \
        ((char) msg->operation_type == OP_TYPE_CREATE_AND_TRANSFER) || \
		((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER) || \
        ((char) msg->operation_type == OP_TYPE_TOEKN2022_CREATE_AND_TRANSFER)) {
		if (proto_check_exchange(&msg->exchange) != 0) {
			db_error("invalid exchange");
			return -1;
		}
		double ex_rate = proto_get_exchange_rate_value(&msg->exchange);
		const char *money_symbol = proto_get_money_symbol(&msg->exchange);

		db_msg("ex_rate:%f", ex_rate);
		db_msg("money_symbol:%s", money_symbol);

		if ((char) msg->operation_type == OP_TYPE_TRANSFER) {
			send_amount = msg->action.transfer.value;
			send_value = ((double) send_amount) / 1000000000;
			coin_decimals = 9;
		} else if (((char) msg->operation_type == OP_TYPE_TOKENTRANSFER) || \
            ((char) msg->operation_type == OP_TYPE_CREATE_AND_TRANSFER) || \
			((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER) || \
            ((char) msg->operation_type == OP_TYPE_TOEKN2022_CREATE_AND_TRANSFER)) {
			send_amount = msg->action.token_transfer.amount;
			send_value = proto_coin_real_value(send_amount, msg->action.token_transfer.decimals);
			coin_decimals = msg->action.token_transfer.decimals;
		}

		db_msg("send_value:%.8lf", send_value);

		memset(tmpbuf, 0, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);
		view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
		strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
		snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", ex_rate * send_value);

		snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%.2f", money_symbol, ex_rate * send_value);
		view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		send_amount = 0;
		if ((char) msg->operation_type == OP_TYPE_TRANSFER) {
			send_amount = msg->action.transfer.fee;
		} else if (((char) msg->operation_type == OP_TYPE_TOKENTRANSFER) || \
            ((char) msg->operation_type == OP_TYPE_CREATE_AND_TRANSFER) || \
			((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER) || \
            ((char) msg->operation_type == OP_TYPE_TOEKN2022_CREATE_AND_TRANSFER)) {
			send_amount = msg->action.token_transfer.fee;
		}
		db_msg("send_amount:%lld", send_amount);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_amount, 9);
		snprintf(tmpbuf, sizeof(tmpbuf), "%s SOL", tmpbuf);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		const char *uname2 = coin_uname;
		if (strcmp(msg->coin.path, sol_get_hd_path(COIN_TYPE_SOLANA, COIN_UNAME_SOL2)) == 0) {
			uname2 = COIN_UNAME_SOL2;
		}
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, uname2, 0, 0);
		omit_string(tmpbuf, tmpbuf, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));

		omit_string(tmpbuf, msg->action.transfer.recipient, 26, 11);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;

		if (((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER) || \
            ((char) msg->operation_type == OP_TYPE_TOEKN2022_CREATE_AND_TRANSFER)) {
			view->total_height = 3 * SCREEN_HEIGHT;
			const char *memo = msg->action.token_transfer.memo;
			if (!is_empty_string(memo)) {
				db_msg("memo no empty:%s", memo);
				if (is_printable_str(memo)) {
					view_add_txt(TXS_LABEL_DATA_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
					view_add_txt(TXS_LABEL_DATA_CONTENT, memo);
				} else {
					view_add_txt(TXS_LABEL_DATA_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_HEX_TITLE));
					ret = strlen(memo);
					if (ret * 2 < (int) sizeof(tmpbuf)) {
						bin_to_hex((const unsigned char *)memo, ret, tmpbuf);
						view_add_txt(TXS_LABEL_DATA_CONTENT, tmpbuf);
					} else {
						char *hex = (char *) malloc((ret + 1) * 2);
						if (hex) {
							memset(hex, 0, (ret + 1) * 2);
							bin_to_hex((const unsigned char *)memo, ret, hex);
							view_add_txt(TXS_LABEL_DATA_CONTENT, hex);
							free(hex);
						}
					}
				}
			} else {
				view_add_txt(TXS_LABEL_DATA_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
				view_add_txt(TXS_LABEL_DATA_CONTENT, "");
			}
		}
	} else if ((char) msg->operation_type == OP_TYPE_NFT_TRANSFER || 
        (char) msg->operation_type == OP_TYPE_NFT_CREATE_AND_TRANSFER || 
		(char) msg->operation_type == OP_TYPE_MNFT) {
		view_add_txt(TXS_LABEL_TOTAL_VALUE, "Token address");

		char tmpbuf[128];
		memset(tmpbuf, 0, sizeof(tmpbuf));
		const char *mint = msg->operation_type == OP_TYPE_MNFT ? 
		msg->action.mNFT.token_mint_address : msg->action.token_transfer.token_mint_address;
		memcpy(tmpbuf, mint, 7);
		tmpbuf[7] = '.';
		tmpbuf[8] = '.';
		tmpbuf[9] = '.';
		int len = strlen(mint);
		memcpy(tmpbuf + 10, mint + (len - 8), 8);
		db_msg("tmpbuf:%s", tmpbuf);
		view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);


		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		send_amount = msg->operation_type == OP_TYPE_MNFT ?
		msg->action.mNFT.fee : msg->action.token_transfer.fee;

		db_msg("send_amount:%lld", send_amount);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_amount, 9);
		snprintf(tmpbuf, sizeof(tmpbuf), "%s SOL", tmpbuf);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		const char *uname2 = coin_uname;
		if (strcmp(msg->coin.path, sol_get_hd_path(COIN_TYPE_SOLANA, COIN_UNAME_SOL2)) == 0) {
			uname2 = COIN_UNAME_SOL2;
		}
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, uname2, 0, 0);
		omit_string(tmpbuf, tmpbuf, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));

		const char *recipient = msg->operation_type == OP_TYPE_MNFT ?
		msg->action.mNFT.recipient : msg->action.token_transfer.to;
		omit_string(tmpbuf, recipient, 26, 11);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;

    } else if ((char) msg->operation_type == OP_TYPE_COMPRESSED_NFT_TRANSFER) {
		// view_add_txt(TXS_LABEL_TOTAL_VALUE, "Merkle Tree");
		CompressedNFT compressedNFT = msg->action.compressedNFT;
		const char *treeAddress = compressedNFT.treeAddress;

		// char tmpbuf[128];
		// memset(tmpbuf, 0, sizeof(tmpbuf));
		// memcpy(tmpbuf, treeAddress, 7);
		// tmpbuf[7] = '.';
		// tmpbuf[8] = '.';
		// tmpbuf[9] = '.';
		// int len = strlen(treeAddress);
		// memcpy(tmpbuf + 10,treeAddress + (len - 8), 8);
		// db_msg("tmpbuf:%s", tmpbuf);
		// view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

		view_add_txt(TXS_LABEL_TOTAL_MONEY, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));


		// view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		send_amount = compressedNFT.fee;

		db_msg("send_amount:%lld", send_amount);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_amount, 9);
		snprintf(tmpbuf, sizeof(tmpbuf), "%s SOL", tmpbuf);
		db_msg("feed value:%s", tmpbuf);
		// view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);
		view_add_txt(TXS_LABEL_FEED_TILE, tmpbuf);

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		const char *uname2 = coin_uname;
		if (strcmp(msg->coin.path, sol_get_hd_path(COIN_TYPE_SOLANA, COIN_UNAME_SOL2)) == 0) {
			uname2 = COIN_UNAME_SOL2;
		}
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, uname2, 0, 0);
		omit_string(tmpbuf, tmpbuf, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));

		omit_string(tmpbuf, compressedNFT.newLeafOwner, 26, 11);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;

	} else if ((char) msg->operation_type == OP_TYPE_DAPP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		format_data_to_hex(msg->action.dapp.message_data.bytes, msg->action.dapp.message_data.size, str, sizeof(str));
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, str);
	} else if ((char) msg->operation_type == OP_TYPE_MSG) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, msg->action.msg.message);
	} else if ((char) msg->operation_type == OP_TYPE_SWAP) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		format_data_to_hex(msg->action.swap.message_data.bytes, msg->action.swap.message_data.size, str, sizeof(str));
		view_add_txt(TXS_LABEL_APP_MSG_VALUE, str);
	} else if ((char) msg->operation_type == OP_TYPE_REG_NONCE) {
		db->tx_type = TX_TYPE_APP_SIGN_MSG;
		view->total_height = SCREEN_HEIGHT;
		view_add_txt(TXS_LABEL_TOTAL_VALUE, "Type");
		view_add_txt(TXS_LABEL_TOTAL_MONEY, "Enable Durable Nonces");
		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		double fee = 0;
		double amount = 0;
		ret = sol_getFeeAndAmount(msg, &fee, &amount, NULL);
		if (ret != 0) {
			db_error("sol_getFeeAndAmount failed");
			return ret; 
		}
		snprintf(tmpbuf, sizeof(tmpbuf), "%.9f SOL", fee);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);
	} else if (((char) msg->operation_type == OP_TYPE_TRANSFER_HARDWARE) || \
        ((char) msg->operation_type == OP_TYPE_TOKENTRANSFER_HARDWARE) || \
        ((char) msg->operation_type == OP_TYPE_TOKENTRANSFER_HARDWARE_CREATE) || \
		((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER_HARDWARE) || \
        ((char) msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER_HARDWARE_CREATE)) {
		if (proto_check_exchange(&msg->exchange) != 0) {
			db_error("invalid exchange");
			return -1;
		}
		double ex_rate = proto_get_exchange_rate_value(&msg->exchange);
		const char *money_symbol = proto_get_money_symbol(&msg->exchange);

		db_msg("ex_rate:%f", ex_rate);
		db_msg("money_symbol:%s", money_symbol);

		double fee = 0;
		double amount = 0;
		char memo[128] = {0};
		ret = sol_getFeeAndAmount(msg, &fee, &amount, memo);
		if (ret != 0) {
			db_error("sol_getFeeAndAmount failed");
			return ret; 
		}

		memset(tmpbuf, 0, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%f", amount);
		view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
		strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
		snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", ex_rate * amount);

		snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%.2f", money_symbol, ex_rate * amount);
		view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);

		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));

		memset(tmpbuf, 0, sizeof(tmpbuf));
		snprintf(tmpbuf, sizeof(tmpbuf), "%.9f SOL", fee);
		db_msg("feed value:%s", tmpbuf);
		view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);

		strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		const char *uname2 = coin_uname;
		if (strcmp(msg->coin.path, sol_get_hd_path(COIN_TYPE_SOLANA, COIN_UNAME_SOL2)) == 0) {
			uname2 = COIN_UNAME_SOL2;
		}
		wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, uname2, 0, 0);
		omit_string(tmpbuf, tmpbuf, 26, 11);
		view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));

		omit_string(tmpbuf, msg->action.transaction.to, 26, 11);
		view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);

		view->total_height = 2 * SCREEN_HEIGHT;

		if (((strlen(memo) > 0) && (msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER_HARDWARE))
    || (msg->operation_type == OP_TYPE_TOEKN2022_TRANSFER_HARDWARE_CREATE)) {
			view->total_height = 3 * SCREEN_HEIGHT;
			view_add_txt(TXS_LABEL_DATA_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
			view_add_txt(TXS_LABEL_DATA_CONTENT, memo);
		}
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
