#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Eos/eos_sign.c"

#endif

enum {
	TXS_LABEL_TOTAL_VALUE,
	TXS_LABEL_TOTAL_MONEY,
	TXS_LABEL_PAYFROM_TITLE,
	TXS_LABEL_PAYFROM_ADDRESS,
	TXS_LABEL_PAYTO_TITLE,
	TXS_LABEL_PAYTO_ADDRESS,
	TXS_LABEL_MEMO_TITLE,
	TXS_LABEL_MEMO_CONTENT,

	TXS_LABEL_TITLE_AMOUNT1,
	TXS_LABEL_TITLE_AMOUNT2,
	TXS_LABEL_TITLE_ADDR1,
	TXS_LABEL_TITLE_ADDR2,
	TXS_LABEL_VALUE_AMOUNT1,
	TXS_LABEL_VALUE_AMOUNT2,
	TXS_LABEL_VALUE_ADDR1,
	TXS_LABEL_VALUE_ADDR2,
	TXS_LABEL_MAXID,
};

static int label_mk_map[TXS_LABEL_MAXID] = {
		MK_bnc_label_total_value,
		MK_bnc_label_total_money,
		MK_bnc_label_payfrom_title,
		MK_bnc_label_payfrom_address,
		MK_bnc_label_payto_title,
		MK_bnc_label_payto_address,
		MK_bnc_label_memo_title,
		MK_bnc_label_memo_content,
		MK_eos_amount_title_1,
		MK_eos_amount_title_2,
		MK_eos_addr_title_1,
		MK_eos_addr_title_2,
		MK_eos_amount_value_1,
		MK_eos_amount_value_2,
		MK_eos_addr_value_1,
		MK_eos_addr_value_2,
};

#define _dview_add_txt(id, value) do{view_add_txt(id,value);id++;}while(0)

static int show_transfer(SignRequest *msg, DBTxCoinInfo *db, DynamicViewCtx *view, Transfer *transfer) {
	char tmpbuf[128];
	int ret;

	Transfer *t = transfer;
	db_msg("action account:%s action_name:%s actor:%s from:%s to:%s memo:%s",
	       t->account, t->action_name, t->actor, t->from, t->to, t->memo);
	db_msg("action symbol:%s decimals:%d amount:%lld", t->symbol, t->decimals, t->amount);

	snprintf(db->coin_name, sizeof(db->coin_name), "%s(%s)", transfer->symbol, transfer->account);
	strlcpy(db->coin_symbol, transfer->symbol, sizeof(db->coin_symbol));
	view->total_height = 2 * SCREEN_HEIGHT;

	int64_t send_amount = transfer->amount;
	int coin_decimals = transfer->decimals ? transfer->decimals : 4;
	double send_value = proto_coin_real_value(send_amount, coin_decimals);
	double ex_rate = proto_get_exchange_rate_value(&msg->exchange);
	const char *money_symbol = proto_get_money_symbol(&msg->exchange);

	db_msg("get send_value:%.8lf str:%s", send_value, tmpbuf);
	snprintf(tmpbuf, sizeof(tmpbuf), "%.8lf", send_value);

	format_coin_real_value(tmpbuf, sizeof(tmpbuf), send_amount, coin_decimals);
	db_msg("get send_amount:%lld str:%s", send_amount, tmpbuf);
	view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);

	strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
	snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", ex_rate * send_value);

	snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%.2f", money_symbol, ex_rate * send_value);
	view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
	strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));

	view_add_txt_off(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE), 10);
	view_add_txt_off(TXS_LABEL_PAYFROM_ADDRESS, transfer->from, 10);
	view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
	view_add_txt(TXS_LABEL_PAYTO_ADDRESS, transfer->to);

	if (!is_empty_string(transfer->memo)) {
		if (is_printable_str(transfer->memo)) {
			view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_TITLE));
			view_add_txt(TXS_LABEL_MEMO_CONTENT, transfer->memo);
		} else {
			view_add_txt(TXS_LABEL_MEMO_TITLE, res_getLabel(LANG_LABEL_TX_MEMO_HEX_TITLE));
			ret = strlen(transfer->memo);
			if (ret * 2 < (int) sizeof(tmpbuf)) {
				bin_to_hex((const unsigned char *) transfer->memo, ret, tmpbuf);
				view_add_txt(TXS_LABEL_MEMO_CONTENT, tmpbuf);
			} else {
				char *hex = (char *) malloc((ret + 1) * 2);
				if (hex) {
					memset(hex, 0, (ret + 1) * 2);
					bin_to_hex((const unsigned char *) transfer->memo, ret, hex);
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
		if (!storage_isCoinExist(db->coin_type, db->coin_uname)) {
			DBCoinInfo dbinfo;
			memset(&dbinfo, 0, sizeof(dbinfo));
			dbinfo.type = (uint8_t) db->coin_type;
			dbinfo.curv = coin_get_curv_id(db->coin_type, db->coin_uname);
			dbinfo.decimals = coin_decimals;
			strncpy(dbinfo.uname, db->coin_uname, COIN_UNAME_MAX_LEN);
			strncpy(dbinfo.name, db->coin_name, COIN_NAME_MAX_LEN);
			strncpy(dbinfo.symbol, db->coin_symbol, COIN_SYMBOL_MAX_LEN);
			storage_save_coin_dbinfo(&dbinfo);
		}
	}
	return 0;
}

static int show_delegatebw(SignRequest *msg, DBTxCoinInfo *db, DynamicViewCtx *view, DelegateBw *delegateBw) {
	char tmpbuf[64];
	DelegateBw *t = delegateBw;
	db_msg("from:%s to:%s net:%lld cpu:%lld decimals:%d transfer:%d", t->from, t->to, t->net, t->cpu, t->decimals, t->transfer);

	strlcpy(db->coin_name, res_getLabel(LANG_LABEL_DELEGATEBW_TITLE), sizeof(db->coin_name));
	int coin_decimals = delegateBw->decimals ? delegateBw->decimals : 4;
	int off = 0;
	if (delegateBw->cpu) {
		if (!delegateBw->net) off = 10;
		view_add_txt_off(TXS_LABEL_TITLE_AMOUNT1, "CPU:", off);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), delegateBw->cpu, coin_decimals);
		snprintf(db->send_value, sizeof(db->send_value), "CPU:%s", tmpbuf);
		strcat(tmpbuf, " EOS");
		view_add_txt_off(TXS_LABEL_VALUE_AMOUNT1, tmpbuf, off);
	}

	if (delegateBw->net) {
		if (!delegateBw->cpu) off = 10;
		view_add_txt_off(TXS_LABEL_TITLE_AMOUNT2, "NET:", off);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), delegateBw->net, coin_decimals);
		if (db->send_value[0]) {
			strlcat(db->send_value, " NET:", sizeof(db->send_value));
			strlcat(db->send_value, tmpbuf, sizeof(db->send_value));
		} else {
			snprintf(db->send_value, sizeof(db->send_value), "NET:%s", tmpbuf);
		}
		strcat(tmpbuf, " EOS");
		view_add_txt_off(TXS_LABEL_VALUE_AMOUNT2, tmpbuf, off);
	}

	view_add_txt(TXS_LABEL_TITLE_ADDR1, res_getLabel(LANG_LABEL_DELEGATEBW_FROM));
	view_add_txt(TXS_LABEL_VALUE_ADDR1, delegateBw->from);

	if (delegateBw->transfer) {
		view_add_txt(TXS_LABEL_TITLE_ADDR2, res_getLabel(LANG_LABEL_DELEGATEBW_TRANSFER));
	} else {
		view_add_txt(TXS_LABEL_TITLE_ADDR2, res_getLabel(LANG_LABEL_DELEGATEBW_TO));
	}
	view_add_txt(TXS_LABEL_VALUE_ADDR2, delegateBw->to);
	return 0;
}

static int show_undelegatebw(SignRequest *msg, DBTxCoinInfo *db, DynamicViewCtx *view, UnDelegateBw *unDelegateBw) {
	char tmpbuf[64];
	UnDelegateBw *t = unDelegateBw;
	db_msg("from:%s to:%s net:%lld cpu:%lld ", t->from, t->to, t->net, t->cpu);

	strlcpy(db->coin_name, res_getLabel(LANG_LABEL_UNDELEGATEBW_TITLE), sizeof(db->coin_name));

	int coin_decimals = unDelegateBw->decimals ? unDelegateBw->decimals : 4;
	int off = 0;
	if (unDelegateBw->cpu) {
		if (!unDelegateBw->net) off = 10;
		view_add_txt_off(TXS_LABEL_TITLE_AMOUNT1, "CPU:", off);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), unDelegateBw->cpu, coin_decimals);
		snprintf(db->send_value, sizeof(db->send_value), "CPU:%s", tmpbuf);
		strcat(tmpbuf, " EOS");
		view_add_txt_off(TXS_LABEL_VALUE_AMOUNT1, tmpbuf, off);
	}

	if (unDelegateBw->net) {
		if (!unDelegateBw->cpu) off = 10;
		view_add_txt_off(TXS_LABEL_TITLE_AMOUNT2, "NET:", off);
		format_coin_real_value(tmpbuf, sizeof(tmpbuf), unDelegateBw->net, coin_decimals);
		if (db->send_value[0]) {
			strlcat(db->send_value, " NET:", sizeof(db->send_value));
			strlcat(db->send_value, tmpbuf, sizeof(db->send_value));
		} else {
			snprintf(db->send_value, sizeof(db->send_value), "NET:%s", tmpbuf);
		}
		strcat(tmpbuf, " EOS");
		view_add_txt_off(TXS_LABEL_VALUE_AMOUNT2, tmpbuf, off);
	}

	view_add_txt(TXS_LABEL_TITLE_ADDR1, res_getLabel(LANG_LABEL_UNDELEGATEBW_FROM));
	view_add_txt(TXS_LABEL_VALUE_ADDR1, unDelegateBw->from);

	view_add_txt(TXS_LABEL_TITLE_ADDR2, res_getLabel(LANG_LABEL_UNDELEGATEBW_TO));
	view_add_txt(TXS_LABEL_VALUE_ADDR2, unDelegateBw->to);

	return 0;
}

static int show_buyram(SignRequest *msg, DBTxCoinInfo *db, DynamicViewCtx *view, BuyRam *buyRam) {
	char tmpbuf[64];
	char account_value[32];
	BuyRam *t = buyRam;
	db_msg("payer:%s receiver:%s  decimals:%d amount:%lld get_memory:%lld", t->payer, t->receiver, t->decimals, t->amount, t->get_memory);

	strlcpy(db->coin_name, res_getLabel(LANG_LABEL_BUYRAM_TITLE), sizeof(db->coin_name));

	int coin_decimals = buyRam->decimals ? buyRam->decimals : 4;

	view_add_txt(TXS_LABEL_TITLE_AMOUNT1, res_getLabel(LANG_LABEL_ORDER_AMOUNT));
	format_coin_real_value(account_value, sizeof(account_value), buyRam->amount, coin_decimals);
	strlcpy(db->send_value, account_value, sizeof(db->send_value));

	snprintf(tmpbuf, sizeof(tmpbuf), "%s EOS", account_value);
	view_add_txt(TXS_LABEL_VALUE_AMOUNT1, tmpbuf);

	format_coin_real_value(account_value, sizeof(account_value), buyRam->get_memory, 3);
	strlcpy(db->currency_value, account_value, sizeof(db->currency_value));
	strcpy(db->currency_symbol, "KB");
	snprintf(tmpbuf, sizeof(tmpbuf), "≈%s KB", account_value);
	view_add_txt(TXS_LABEL_VALUE_AMOUNT2, tmpbuf);

	view_add_txt(TXS_LABEL_TITLE_ADDR1, res_getLabel(LANG_LABEL_BUYRAM_PAYER));
	view_add_txt(TXS_LABEL_VALUE_ADDR1, buyRam->payer);

	view_add_txt(TXS_LABEL_TITLE_ADDR2, res_getLabel(LANG_LABEL_BUYRAM_RECEIVER));
	view_add_txt(TXS_LABEL_VALUE_ADDR2, buyRam->receiver);

	return 0;
}

static int show_sellram(SignRequest *msg, DBTxCoinInfo *db, DynamicViewCtx *view, SellRam *sellRam) {
	char tmpbuf[64];
	char account_value[32];
	SellRam *t = sellRam;
	db_msg("account:%s ram_bytes:%lld get_eos:%lld", t->seller_account, t->ram_bytes, t->get_eos);
	strlcpy(db->coin_name, res_getLabel(LANG_LABEL_SELLRAM_TITLE), sizeof(db->coin_name));


	view_add_txt_off(TXS_LABEL_TITLE_AMOUNT1, res_getLabel(LANG_LABEL_ORDER_AMOUNT), 10);
	format_coin_real_value(account_value, sizeof(account_value), sellRam->ram_bytes, 3);
	strlcpy(db->currency_value, account_value, sizeof(db->currency_value));
	strcpy(db->currency_symbol, "KB");
	snprintf(tmpbuf, sizeof(tmpbuf), "%s KB", account_value);
	view_add_txt_off(TXS_LABEL_VALUE_AMOUNT1, tmpbuf, 10);

	format_coin_real_value(account_value, sizeof(account_value), sellRam->get_eos, 4);
	snprintf(db->send_value, sizeof(db->send_value), "+%s", account_value);
	snprintf(tmpbuf, sizeof(tmpbuf), "≈%s EOS", account_value);
	view_add_txt_off(TXS_LABEL_VALUE_AMOUNT2, tmpbuf, 15);

	view_add_txt_off(TXS_LABEL_TITLE_ADDR1, res_getLabel(LANG_LABEL_ACCOUNT_NAME), 20);
	view_add_txt_off(TXS_LABEL_VALUE_ADDR1, sellRam->seller_account, 20);

	return 0;
}

static int on_sign_show(void *session, DynamicViewCtx *view) {
	coin_state *s = (coin_state *) session;
	if (!s) {
		db_error("invalid session");
		return -1;
	}
	int coin_type = COIN_TYPE_EOS;
	const CoinConfig *config = getCoinConfig(coin_type, "EOS");
	if (!config) {
		return -401;
	}
	DBTxCoinInfo *db = &view->db;
	SignRequest *msg = &s->req;
	memset(db, 0, sizeof(DBTxCoinInfo));

#if 1
	RefBlock *rb = &msg->ref_block;
	db_msg("action_type:%d chain_id:%d -> %s", msg->action_type, rb->chain_id.size, debug_ubin_to_hex(rb->chain_id.bytes, rb->chain_id.size));
	db_msg("refblock expiration:%lld ref_block_num:%lld ref_block_prefix:%lld net_usage_words:%d",
	       rb->expiration, rb->ref_block_num, rb->ref_block_prefix, rb->net_usage_words);
	db_msg("refblock max_cpu_usage_ms:%d delay_sec:%d", rb->max_cpu_usage_ms, rb->delay_sec);
#endif

	if (msg->coin.type != COIN_TYPE_EOS || is_empty_string(msg->coin.uname)) {
		db_error("invalid coin msg");
		return -102;
	}
	if (msg->action_type != ACTION_TYPE_TRANSFER && strcmp(msg->coin.uname, "EOS") != 0) {
		db_error("invalid action:%d coin uname:%s", msg->action_type, msg->coin.uname);
		return -103;
	}

	db->coin_type = coin_type;
	strlcpy(db->coin_uname, msg->coin.uname, sizeof(db->coin_uname));
	if (msg->action_type != ACTION_TYPE_TRANSFER) {
		strlcpy(db->coin_name, config->name, sizeof(db->coin_name));
		strlcpy(db->coin_symbol, config->symbol, sizeof(db->coin_symbol));
	}

	view->coin_type = db->coin_type;
	view->coin_uname = db->coin_uname;
	view->coin_name = db->coin_name;
	view->coin_symbol = db->coin_symbol;

	int ret = -1;
	switch (msg->action_type) {
		case ACTION_TYPE_TRANSFER:
			ret = show_transfer(msg, db, view, &msg->action.transfer);
			break;
		case ACTION_TYPE_DELEGATEBW:
			ret = show_delegatebw(msg, db, view, &msg->action.delegateBw);
			break;
		case ACTION_TYPE_UNDELEGATEBW:
			ret = show_undelegatebw(msg, db, view, &msg->action.unDelegateBw);
			break;
		case ACTION_TYPE_BUYRAM:
			ret = show_buyram(msg, db, view, &msg->action.buyRam);
			break;
		case ACTION_TYPE_SELLRAM:
			ret = show_sellram(msg, db, view, &msg->action.sellRam);
			break;
		default:
			ret = -105;
	}
	return ret;
}


#endif