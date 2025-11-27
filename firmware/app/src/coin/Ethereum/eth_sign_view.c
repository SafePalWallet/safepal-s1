#ifdef COIN_SIGN_VIEW_FILE

#ifdef VIEW_CODE_IN_IDE

#include "coin/Ethereum/eth_sign.c"

#endif

#include "coin_util_hw.h"
#include "math.h"

enum {
	TXS_LABEL_TOTAL_VALUE,
	TXS_LABEL_TOTAL_MONEY,
	TXS_LABEL_NFT_ID_TITLE,
	TXS_LABEL_NFT_ID_VALUE,
	TXS_LABEL_NFT_AMOUNT_TITLE,
	TXS_LABEL_NFT_AMOUNT_VALUE,
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
	TXS_LABEL_APPROVE_TOKEN_TITLE,
	TXS_LABEL_APPROVE_TOKEN_VALUE,
	TXS_LABEL_APPROVE_AMOUNT_TITLE,
	TXS_LABEL_APPROVE_AMOUNT_VALUE,
	TXS_LABEL_SIMPLE_FEE_TITLE,
	TXS_LABEL_SIMPLE_FEE_VALUE,
	TXS_LABEL_APP_MSG_VALUE,
	TXS_LABEL_MAXID,
};

static int label_mk_map[TXS_LABEL_MAXID] = {
		MK_eth_label_total_value,
		MK_eth_label_total_money,
		MK_eth_label_nft_id_title,
		MK_eth_label_nft_id_value,
		MK_eth_label_nft_amount_title,
		MK_eth_label_nft_amount_value,
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
		MK_eth_label_approve_token_title,
		MK_eth_label_approve_token_value,
		MK_eth_label_approve_amount_title,
		MK_eth_label_approve_amount_value,
		MK_eth_label_simple_fee_title,
		MK_eth_label_simple_fee_value,
		MK_eth_label_app_msg_value,
};

static char mName[COIN_NAME_BUFFSIZE];

#define APPROVE_ACTION_NFT_APPROVE              (1)
#define APPROVE_ACTION_NFT_REVOKE_APPROVE       (2)
#define APPROVE_ACTION_NFT_MAKE_OFFER           (3)
#define APPROVE_ACTION_NFT_BUY                  (4)
#define APPROVE_ACTION_NFT_REVOKE_OFFER         (5)
#define APPROVE_ACTION_NFT_LIST                 (6)
#define APPROVE_ACTION_NFT_REVOKE_LISTING       (7)
#define APPROVE_ACTION_NFT_ACCEPT_OFFER         (8)
#define APPROVE_ACTION_NFT_TOKEN_APPROVE        (9)
#define APPROVE_ACTION_NFT_TOKEN_REVOKE_APPROVE (10)
#define APPROVE_ACTION_MAX                      (11)

#define TOKEN_EXTRA_TYPE_COIN     (1)
#define TOKEN_EXTRA_TYPE_TOKEN    (2)
#define TOKEN_EXTRA_TYPE_ERC721   (3)
#define TOKEN_EXTRA_TYPE_ERC1155  (4)

static int on_sign_show(void *session, DynamicViewCtx *view) {
	char tmpbuf[256];
	coin_state *s = (coin_state *) session;
	if (!s) {
		db_error("invalid session");
		return -1;
	}

	int ret;
	EthSignTxReq *msg = &s->req;
	DBTxCoinInfo *db = &view->db;

	memset(db, 0, sizeof(DBTxCoinInfo));
	if (proto_check_exchange(&msg->exchange) != 0) {
		db_error("invalid exchange");
		return -102;
	}

	double ex_rate = proto_get_exchange_rate_value(&msg->exchange);
	const char *money_symbol = proto_get_money_symbol(&msg->exchange);

	int coin_type = 0;
	const char *coin_uname = "";
	const char *name = "";
	const char *symbol = "";
	uint8_t coin_decimals = 0;
	uint8_t is_transfer = 0;
	uint8_t is_transfer_from = 0;
	uint8_t is_1155_transfer = 0;
	uint8_t is_approval = 0;
	uint8_t trans_token = 0;
	uint8_t trans_nft = 0;
	uint8_t detected_data = 0;
	const CoinConfig *config = NULL;
	do {
		detected_data = 1;
		//transfer(address _to, uint256 _value)     //ERC20
		//transfer(address _to, uint256 _tokenId)   //ERC721
        if (msg->to.size == 20 && msg->value.size == 0 && msg->data.size == 68
            && memcmp(msg->data.bytes, "\xa9\x05\x9c\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0) {
            is_transfer = 1;
            trans_token = 1;
            if (msg->token.type == COIN_TYPE_ERC721 ||
                msg->token.type == COIN_TYPE_BEP721 ||
                msg->token.type == COIN_TYPE_FANTOM721 ||
                msg->token.type == COIN_TYPE_HECO721 ||
                msg->token.type == COIN_TYPE_OPTIMISM721 ||
                msg->token.type == COIN_TYPE_ARBITRUM721 ||
                msg->token.type == COIN_TYPE_AVAX721 ||
                msg->token.type == COIN_TYPE_POLYGON721 ||
                msg->token.type == COIN_TYPE_GODWOKENV1_721 ||
                msg->token.type == COIN_TYPE_KLAYTN_721 ||
                msg->token.type == COIN_TYPE_TLOS_721 ||
                msg->token.type == COIN_TYPE_ZKSYNC_721 ||
                msg->token.type == COIN_TYPE_PZK_721 ||
                msg->token.type == COIN_TYPE_PLS_721 ||
                msg->token.type == COIN_TYPE_CMP_721 ||
                msg->token.type == COIN_TYPE_MANTLE_721 ||
                msg->token.type == COIN_TYPE_LINEA_721 ||
                msg->token.type == COIN_TYPE_BASE_721 ||
                msg->token.type == COIN_TYPE_CUSTOM_EVM_721 ||
                msg->token.type == COIN_TYPE_CORE_721 ||
                msg->token.type == COIN_TYPE_OPBNB_721 ||
                msg->token.type == COIN_TYPE_ZKFAIR_721 ||
                msg->token.type == COIN_TYPE_BLAST_721 ||
                msg->token.type == COIN_TYPE_MANTA_721 ||
                msg->token.type == COIN_TYPE_MERLIN_721 ||
                msg->token.type == COIN_TYPE_BOUNCEBIT_721 ||
                msg->token.type == COIN_TYPE_AIRDAO_721 ||
                msg->token.type == COIN_TYPE_GRAVITY_721 ||
                msg->token.type == COIN_TYPE_TAIKO_721 ||
                msg->token.type == COIN_TYPE_KAVA_721 ||
                msg->token.type == COIN_TYPE_SCROLL_721 ||
                msg->token.type == COIN_TYPE_SEIEVM_721 ||
                msg->token.type == COIN_TYPE_OASISEVM_721 ||
                msg->token.type == COIN_TYPE_SONIC_721 ||
                msg->token.type == COIN_TYPE_LUMIA_721 ||
                msg->token.type == COIN_TYPE_BITLAYER_721 ||
                msg->token.type == COIN_TYPE_BERA_721 ||
                msg->token.type == COIN_TYPE_VICTION_725 ||
                msg->token.type == COIN_TYPE_VALUECHAIN_721 ||
                msg->token.extra_type == TOKEN_EXTRA_TYPE_ERC721) {
                trans_nft = 1;
            }
            break;
        }

		//transferFrom(address src, address dst, uint256 amount) MethodID: 0x23b872dd
        if (msg->to.size == 20 && msg->value.size == 0 && msg->data.size == 100
            && memcmp(msg->data.bytes, "\x23\xb8\x72\xdd\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0) {
            is_transfer_from = 1;
            trans_token = 1;
            if (msg->token.type == COIN_TYPE_ERC721 ||
                msg->token.type == COIN_TYPE_BEP721 ||
                msg->token.type == COIN_TYPE_FANTOM721 ||
                msg->token.type == COIN_TYPE_HECO721 ||
                msg->token.type == COIN_TYPE_OPTIMISM721 ||
                msg->token.type == COIN_TYPE_ARBITRUM721 ||
                msg->token.type == COIN_TYPE_AVAX721 ||
                msg->token.type == COIN_TYPE_POLYGON721 ||
                msg->token.type == COIN_TYPE_GODWOKENV1_721 ||
                msg->token.type == COIN_TYPE_KLAYTN_721 ||
                msg->token.type == COIN_TYPE_TLOS_721 ||
                msg->token.type == COIN_TYPE_ZKSYNC_721 ||
                msg->token.type == COIN_TYPE_PLS_721 ||
                msg->token.type == COIN_TYPE_PZK_721 ||
                msg->token.type == COIN_TYPE_CMP_721 ||
                msg->token.type == COIN_TYPE_MANTLE_721 ||
                msg->token.type == COIN_TYPE_LINEA_721 ||
                msg->token.type == COIN_TYPE_BASE_721 ||
                msg->token.type == COIN_TYPE_CUSTOM_EVM_721 ||
                msg->token.type == COIN_TYPE_CORE_721 ||
                msg->token.type == COIN_TYPE_OPBNB_721 ||
                msg->token.type == COIN_TYPE_ZKFAIR_721 ||
                msg->token.type == COIN_TYPE_BLAST_721 ||
                msg->token.type == COIN_TYPE_MANTA_721 ||
                msg->token.type == COIN_TYPE_MERLIN_721 ||
                msg->token.type == COIN_TYPE_BOUNCEBIT_721 ||
                msg->token.type == COIN_TYPE_AIRDAO_721 ||
                msg->token.type == COIN_TYPE_GRAVITY_721 ||
                msg->token.type == COIN_TYPE_TAIKO_721 ||
                msg->token.type == COIN_TYPE_KAVA_721 ||
                msg->token.type == COIN_TYPE_SCROLL_721 ||
                msg->token.type == COIN_TYPE_SEIEVM_721 ||
                msg->token.type == COIN_TYPE_OASISEVM_721 ||
                msg->token.type == COIN_TYPE_SONIC_721 ||
                msg->token.type == COIN_TYPE_LUMIA_721 ||
                msg->token.type == COIN_TYPE_BITLAYER_721 ||
                msg->token.type == COIN_TYPE_BERA_721 ||
                msg->token.type == COIN_TYPE_VICTION_725 ||
                msg->token.type == COIN_TYPE_VALUECHAIN_721 ||
                msg->token.extra_type == TOKEN_EXTRA_TYPE_ERC721) {
                trans_nft = 1;
            }
            break;
        }

		//0xf242432a safeTransferFrom(address _from, address _to, uint256 _id, uint256 _amount, bytes _data) //ERC1155
		if ((msg->to.size == 20 && msg->value.size == 0 && msg->data.size >= 164
		    && memcmp(msg->data.bytes, "\xf2\x42\x43\x2a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0) ||
            msg->token.extra_type == TOKEN_EXTRA_TYPE_ERC1155) {
			is_1155_transfer = 1;
			trans_token = 1;
			trans_nft = 1;
			break;
		}

		//0x095ea7b3 approve(address spender, uint256 amount)
		if (msg->data.size == 68 && memcmp(msg->data.bytes, "\x09\x5e\xa7\xb3\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0) {
			is_approval = 1;
			if (msg->sign_type != 1) {
				db_error("invalid sign type:%d", msg->sign_type);
				return -103;
			}
			break;
		}

		detected_data = 0;
	} while (0);

	if (msg->sign_type == 1) {
		if (!is_approval) {
			db_error("invalid sign data");
			return -104;
		}
		if (is_empty_string(msg->contract.name)) {
			db_error("invalid contract name");
			return -105;
		}
		if (msg->contract.id.size != 20) {
			db_error("invalid contract id:%d", msg->contract.id.size);
			return -106;
		}
		if (memcmp(msg->data.bytes + 16, msg->contract.id.bytes, 20) != 0) {
			db_error("missmatch contract id and data");
			return -107;
		}
	} else if (msg->sign_type == 2) { //app
		if (msg->data.size < 4) {
			db_error("invalid data size:%d", msg->data.size);
			return -113;
		}
		if (is_empty_string(msg->contract.name)) {
			db_error("invalid contract name");
			return -115;
		}
		if (msg->contract.id.size != 20) {
			db_error("invalid contract id");
			return -116;
		}
		if (msg->to.size != 20) {
			db_error("invalid to size:%d", msg->to.size);
			return -117;
		}
		if (memcmp(msg->to.bytes, msg->contract.id.bytes, 20) != 0) {
			db_error("missmatch contract id and to");
			return -118;
		}
	}

	if (msg->coin.type && is_not_empty_string(msg->coin.uname)) {
		config = getCoinConfig(msg->coin.type, msg->coin.uname);
		if (!config) {
			db_msg("not config type:%#x uname:%s", msg->coin.type, msg->coin.uname);
            if (msg->coin.category == COIN_CATEGORY_EVM && !IS_VALID_COIN_TYPE(msg->coin.type)) {
                db->flag |= DB_FLAG_UNIVERSAL_EVM;
            }
		}
	}
	bool is7702 = msg->transaction_type == ETH_TYPE_7702_UPDATE_TX || msg->transaction_type == ETH_TYPE_7702_TX;
	if (config) {
		name = config->name;
		symbol = config->symbol;
		coin_decimals = config->decimals;
		coin_type = config->type;
		coin_uname = config->uname;
	} else if ((trans_token && msg->token.type) || is7702) {
		if (msg->coin.type && msg->token.type != msg->coin.type) {
			db_error("invalid coin.type:%d token.type:%d", msg->coin.type, msg->token.type);
			return -1;
		}
		if (is_empty_string(msg->token.name) || is_empty_string(msg->token.symbol)) {
            if (trans_nft) {//NFT not has name or symbol
                //run
            } else {
                db_error("invalid token name:%s or symbol:%s", msg->token.name, msg->token.symbol);
                return -1;
            }
		}
		if (msg->token.decimals < 0 || msg->token.decimals > 40) {
			db_error("invalid decimals:%d", msg->token.decimals);
			return -1;
		}
		name = msg->token.name;
		symbol = msg->token.symbol;
		if ((msg->token.type == msg->coin.type) && is_not_empty_string(msg->coin.uname)) {
			coin_uname = msg->coin.uname;
			const char *coin_uname_err = NULL;
			if (strlen(name) < COIN_UNAME_BUFFSIZE) {
				coin_uname_err = name;
			} else {
				coin_uname_err = symbol;
			}
			//clean invalid old data
			if ((view->msg_from == MSG_FROM_QR_APP) && is_not_empty_string(coin_uname_err) && strcmp(coin_uname, coin_uname_err) != 0) {
				if (storage_isCoinExist(msg->token.type, coin_uname_err) > 0) {
					storage_deleteCoinInfo(msg->token.type, coin_uname_err);
				}
			}
		} else if (strlen(name) < COIN_UNAME_BUFFSIZE) {
			coin_uname = name;
		} else {
			coin_uname = symbol;
		}
		coin_decimals = (uint8_t) msg->token.decimals;
		coin_type = msg->token.type;
    } else if (msg->coin.type == COIN_TYPE_CUSTOM_EVM) {
        name = msg->chain_info.name;
        symbol = msg->chain_info.native_symbol;
        coin_decimals = msg->chain_info.native_decimals;
        coin_type = msg->coin.type;
        coin_uname = msg->coin.uname;
		db_msg("COIN_TYPE_CUSTOM_EVM name:%s symbol:%s coin_uname:%s", name, symbol, coin_uname);
    } else if (msg->token.extra_type == TOKEN_EXTRA_TYPE_COIN) {
        name = msg->chain_info.name;
        symbol = msg->chain_info.native_symbol;
        coin_decimals = msg->chain_info.native_decimals;
        coin_type = msg->coin.type;
        coin_uname = msg->coin.uname;
		db_msg("evm name:%s, symbol:%s, coin_decimals:%d, coin_type:%x, uname:%s", name, symbol, coin_decimals, coin_type, coin_uname);
    } else {
        name = "Unkown Message";
        coin_type = msg->coin.type;//unstake
    }
	if (trans_nft && coin_decimals != 0) {
		db_error("invalid trans_nft coin_decimals:%d", coin_decimals);
		coin_decimals = 0;
	}

    db_msg("is_transfer:%d, is_transfer_from:%d, is_1155_transfer:%d", is_transfer, is_transfer_from, is_1155_transfer);
    db_msg("is_approval:%d, trans_token:%d, trans_nft:%d", is_approval, trans_token, trans_nft);
    db_msg("sign_type:%d, msg->nft_order_info.action:%d", msg->sign_type, msg->nft_order_info.action);

	if (msg->sign_type == 1 || msg->sign_type == 2) {
		if (!strcmp(msg->contract.name, "Uniswap")) {
			coin_uname = "Uniswap";
		} else {
			coin_uname = "Dapp";
		}
        symbol = msg->contract.name;//show
        if (msg->sign_type == 1) {
            name = res_getLabel(LANG_LABEL_TX_METHOD_APPROVE);
        } else if (msg->sign_type == 2) {
            name = "Data:";
            symbol = res_getLabel(LANG_LABEL_TX_SIGN);
        }

        //NFT
        if (msg->nft_order_info.action >= APPROVE_ACTION_NFT_APPROVE && msg->nft_order_info.action <= APPROVE_ACTION_NFT_TOKEN_REVOKE_APPROVE) {
            name = "Data:";
            symbol = res_getLabel(LANG_LABEL_TX_SIGN);
        }
	}

    //vic token name
    memzero(mName, sizeof(mName));
    if (coin_type == COIN_TYPE_VICTION) {
        if (strcmp(coin_uname, "VIC")) {
            snprintf(mName, sizeof(mName), "%s%s", name, "(VRC20)");
            name = &mName[0];
        }
    } else if (coin_type == COIN_TYPE_VICTION_21) {
        snprintf(mName, sizeof(mName), "%s%s", name, "(VRC21)");
        name = &mName[0];
    } else if (coin_type == COIN_TYPE_VICTION_25) {
        snprintf(mName, sizeof(mName), "%s%s", name, "(VRC25)");
        name = &mName[0];
    }

	db->coin_type = coin_type;
	strlcpy(db->coin_name, name, sizeof(db->coin_name));
	strlcpy(db->coin_symbol, symbol, sizeof(db->coin_symbol));
	strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

    view->total_height = SCREEN_HEIGHT;
    if (msg->nft_order_info.action > 0 && msg->nft_order_info.action < APPROVE_ACTION_MAX) {
        view->coin_type = coin_type == 0 ? msg->coin.type : coin_type;
    } else {
        view->coin_type = coin_type;
    }
	view->coin_uname = coin_uname;
	view->coin_name = name;
	view->coin_symbol = symbol;

    db_msg("name:%s,symbol:%s,coin_uname:%s,coin_type:%x", name, symbol, coin_uname, coin_type);

	if (msg->sign_type == 0) { //trans
		double send_value = 0;
		view->total_height = 2 * SCREEN_HEIGHT;
		if (trans_nft) {
			view_add_txt(TXS_LABEL_NFT_ID_TITLE, "ID:");
			view_add_txt(TXS_LABEL_NFT_AMOUNT_TITLE, res_getLabel(LANG_LABEL_ORDER_AMOUNT));

			memset(tmpbuf, 0, sizeof(tmpbuf));
			if (is_transfer) {
				bignum_print(msg->data.bytes + 36, 32, coin_decimals, "", tmpbuf, sizeof(tmpbuf));
			} else if (is_transfer_from) {
				bignum_print(msg->data.bytes + 68, 32, coin_decimals, "", tmpbuf, sizeof(tmpbuf));
			} else if (is_1155_transfer) {
				bignum_print(msg->data.bytes + 68, 32, coin_decimals, "", tmpbuf, sizeof(tmpbuf));
			} else {
				tmpbuf[0] = '?';
				tmpbuf[1] = '?';
			}

			view_add_txt(TXS_LABEL_NFT_ID_VALUE, tmpbuf);
			strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if (is_transfer || is_transfer_from) {
				tmpbuf[0] = '1';
			} else if (is_1155_transfer) {
				bignum_print(msg->data.bytes + 100, 32, coin_decimals, "", tmpbuf, sizeof(tmpbuf));
			} else {
				tmpbuf[0] = '?';
				tmpbuf[1] = '?';
			}
			view_add_txt(TXS_LABEL_NFT_AMOUNT_VALUE, tmpbuf);
		}  else if (is7702) {
			if (msg->callInfo.call_n >= 1) {
			    Call calls = msg->callInfo.calls[0];
				ret = bignum2double(calls.data.bytes + 36, 32, msg->token.decimals, &send_value, tmpbuf, sizeof(tmpbuf));
				if (ret != 0) {
					db_error("get send_value false ret:%d", ret);
					tmpbuf[0] = 0;
				}
				snprintf(tmpbuf, sizeof(tmpbuf), "%.6lf", send_value);
				view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);
			}

			
			strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
			snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", ex_rate * send_value);

			snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%.2f", money_symbol, ex_rate * send_value);
			view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
			strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));
		} else {
			ret = 0;
			if (is_transfer) {
				ret = bignum2double(msg->data.bytes + 36, 32, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
			} else if (is_transfer_from) {
				ret = bignum2double(msg->data.bytes + 68, 32, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
			} else if (msg->value.size > 0) {
				ret = bignum2double(msg->value.bytes, msg->value.size, coin_decimals, &send_value, tmpbuf, sizeof(tmpbuf));
			} else {
				tmpbuf[0] = 0;
			}
			if (ret != 0) {
				db_error("get send_value false ret:%d", ret);
				tmpbuf[0] = 0;
			}
			db_msg("get send_value:%.18lf str:%s", send_value, tmpbuf);
			view_add_txt(TXS_LABEL_TOTAL_VALUE, tmpbuf);

			strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));
			snprintf(db->currency_value, sizeof(db->currency_value), "%.2f", ex_rate * send_value);

			snprintf(tmpbuf, sizeof(tmpbuf), "≈%s%.2f", money_symbol, ex_rate * send_value);
			view_add_txt(TXS_LABEL_TOTAL_MONEY, tmpbuf);
			strlcpy(db->currency_symbol, money_symbol, sizeof(db->currency_symbol));
		}

		view_add_txt(TXS_LABEL_FEED_TILE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));
		if (is7702) {
			memset(tmpbuf, 0, sizeof(tmpbuf));
			if (msg->callInfo.type == 1) {
				//gasStation
				view_add_txt(TXS_LABEL_FEED_VALUE, msg->callInfo.gasStation);		
				view_add_txt(TXS_LABEL_GAS_LIMIT, "Gas Balance");	
			} else {
				//token
				if (msg->callInfo.call_n >= 2) {
					Call call = msg->callInfo.calls[1];
					EthTokenInfo feeToken = msg->callInfo.feeToken;
					ret = bignum2double(call.data.bytes + 36, 32, feeToken.decimals, &send_value, tmpbuf, sizeof(tmpbuf));
					snprintf(tmpbuf, sizeof(tmpbuf), "%.6lf", send_value);
					view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);		
					
					view_add_txt(TXS_LABEL_GAS_LIMIT, feeToken.symbol);	
				}
			}
		} else if(msg->transaction_type == ETH_TRANSACTION_TYPE_1559){
			format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->eip1559GasFee.max_fee_per_gas*msg->gas_limit, 18);
			view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);
		}else{
            db_msg("msg->gas_limit:%lld, msg->gas_price:%lld", msg->gas_limit, msg->gas_price);
            uint64_t multiply = 0;
            if (uint64_safe_multiply(msg->gas_limit, msg->gas_price, &multiply)) {
                format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->gas_limit * msg->gas_price, 18);
            } else {
                db_msg("feed override");
                int64_t base = (int64_t) pow(10, 3);
                double feed = 0;
                if (msg->gas_price > msg->gas_limit) {
                    double price = (double) msg->gas_price / base;
                    feed = msg->gas_limit * price;
                    db_msg("base:%lld, price:%f, feed:%f", base, price, feed);
                } else {
                    double limit = (double) msg->gas_limit / base;
                    feed = msg->gas_price * limit;
                    db_msg("base:%lld, limit:%f, feed:%f", base, limit, feed);
                }
                format_coin_real_value(tmpbuf, sizeof(tmpbuf), feed, 15);
            }
			db_msg("feed value:%s", tmpbuf);
			view_add_txt(TXS_LABEL_FEED_VALUE, tmpbuf);
		
			snprintf(tmpbuf, sizeof(tmpbuf), "Gas Limit:%llu", msg->gas_limit);
			view_add_txt(TXS_LABEL_GAS_LIMIT, tmpbuf);
		
			strcpy(tmpbuf, "Gas Price:");
			ret = strlen(tmpbuf);
			format_coin_real_value(tmpbuf + ret, sizeof(tmpbuf) - ret, msg->gas_price, 18);
			view_add_txt(TXS_LABEL_GAS_PRICE, tmpbuf);
		}

		view_add_txt(TXS_LABEL_PAYFROM_TITLE, res_getLabel(LANG_LABEL_TXS_PAYFROM_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
        if (is_1155_transfer || is_transfer_from) {
			tmpbuf[0] = '0';
			tmpbuf[1] = 'x';
			ethereum_address_checksum(msg->data.bytes + 16, tmpbuf + 2, false, 0);
		} else {
            uint32_t index = 0;
            if (is_not_empty_string(msg->coin.path)) {
                db_msg("msg->coin.path:%s", msg->coin.path);
                CoinPathInfo info;
                memzero(&info, sizeof(CoinPathInfo));
                if (parse_coin_path(&info, msg->coin.path) != 0 || info.hn < 3) {
                    db_error("invalid coin type:%d uname:%s path:%s", msg->coin.type, msg->coin.uname, msg->coin.path);
                    return -2;
                }

                db_msg("info.hn:%d,%d,%d", info.hn, info.hvalues[0], info.hvalues[info.hn - 1]);
                db_msg("info.an:%d,%d,%d", info.an, info.avalues[0], info.avalues[info.an - 1]);

                if (info.an == COIN_PATH_MAX_ANUM) {
                    index = info.avalues[info.an - 1];
                }
                ret = wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, index, 0);
            } else {
                ret = wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, coin_type, coin_uname, 0, 0);
            }
            db_msg("ret:%d, index:%d", ret, index);
            if (ret < 0 && msg->coin.category == COIN_CATEGORY_EVM) {
                ret = wallet_gen_address(tmpbuf, sizeof(tmpbuf), NULL, COIN_TYPE_ETH, "ETH", index, 0);
            }
            db_msg("my address ret:%d addr:%s,ret:%d", ret, tmpbuf, ret);
		}
        db_msg("from address len:%d",strlen(tmpbuf));
        if (strlen(tmpbuf) >= 43) {
            omit_string(tmpbuf, tmpbuf, 20, 16);
        }
        view_add_txt(TXS_LABEL_PAYFROM_ADDRESS, tmpbuf);

		view_add_txt(TXS_LABEL_PAYTO_TITLE, res_getLabel(LANG_LABEL_TXS_PAYTO_TITLE));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		if (is7702) {
			int pos = 2;
			tmpbuf[0] = '0';
			tmpbuf[1] = 'x';
			Call calls = msg->callInfo.calls[0];
			ethereum_address_checksum(calls.data.bytes + 16, tmpbuf + pos, false, 0);
			view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);
		} else {
			int pos = 0;
			if (coin_type == COIN_TYPE_XDC) {
				tmpbuf[0] = 'x';
				tmpbuf[1] = 'd';
				tmpbuf[2] = 'c';
				pos = 3;
			} else {
				tmpbuf[0] = '0';
				tmpbuf[1] = 'x';
				pos = 2;
			}
			if (is_transfer) {
				ethereum_address_checksum(msg->data.bytes + 16, tmpbuf + pos, false, 0);
			} else if (is_1155_transfer || is_transfer_from) {
				ethereum_address_checksum(msg->data.bytes + 48, tmpbuf + pos, false, 0);
			} else if (msg->to.size > 0) {
				ethereum_address_checksum(msg->to.bytes, tmpbuf + pos, false, 0);
			} else {
				tmpbuf[0] = 0;
			}
			db_msg("to address len:%d",strlen(tmpbuf));
			if (strlen(tmpbuf) >= 43) {
				omit_string(tmpbuf, tmpbuf, 20, 16);
			}
			view_add_txt(TXS_LABEL_PAYTO_ADDRESS, tmpbuf);
		}

		if (msg->data.size && !detected_data) {
			view->total_height += SCREEN_HEIGHT;
			view_add_txt(TXS_LABEL_DATA_TITLE, "Data:");
			format_data_to_hex(msg->data.bytes, msg->data.size, tmpbuf, sizeof(tmpbuf));
			view_add_txt(TXS_LABEL_DATA_CONTENT, tmpbuf);
		}
	} else if (msg->sign_type == 1) { //approval
		db->tx_type = TX_TYPE_APP_APPROVAL;
		snprintf(tmpbuf, sizeof(tmpbuf), "%s:", res_getLabel(LANG_LABEL_TX_COIN_TYPE));
		view_add_txt(TXS_LABEL_APPROVE_TOKEN_TITLE, tmpbuf);

		if (is_not_empty_string(msg->token.symbol)) {
			snprintf(tmpbuf, sizeof(tmpbuf), "%s", msg->token.symbol);
		} else if (is_not_empty_string(msg->token.name)) {
			snprintf(tmpbuf, sizeof(tmpbuf), "%s", msg->token.name);
		} else {
			tmpbuf[0] = '0';
			tmpbuf[1] = 'x';
			bin_to_hex(msg->to.bytes, 4, tmpbuf + 2);
			tmpbuf[10] = '.';
			tmpbuf[11] = '.';
			tmpbuf[12] = '.';
			bin_to_hex(msg->to.bytes + 16, 4, tmpbuf + 12);
		}
		view_add_txt(TXS_LABEL_APPROVE_TOKEN_VALUE, tmpbuf);
		strlcpy(db->send_value, tmpbuf, sizeof(db->send_value));

		snprintf(tmpbuf, sizeof(tmpbuf), "%s:", res_getLabel(LANG_LABEL_TX_LIMIT));
		view_add_txt(TXS_LABEL_APPROVE_AMOUNT_TITLE, tmpbuf);
		tmpbuf[0] = 0;
		if (buffer_is_ff(msg->data.bytes + 36, 32)) {
			view_add_txt(TXS_LABEL_APPROVE_AMOUNT_VALUE, res_getLabel(LANG_LABEL_TX_UNLIMITED));
		} else if (msg->token.type && msg->token.decimals >= 0) {
			double fee_value = 0;
			ret = bignum2double(msg->data.bytes + 36, 32, msg->token.decimals, &fee_value, tmpbuf, sizeof(tmpbuf));
			if (ret == 0) {
				view_add_txt(TXS_LABEL_APPROVE_AMOUNT_VALUE, tmpbuf);
			}
		}
	} else if (msg->sign_type == 2) { //app message
        db->tx_type = TX_TYPE_APP_SIGN_MSG;
        format_data_to_hex(msg->data.bytes, msg->data.size, tmpbuf, 150);
        view_add_txt(TXS_LABEL_APP_MSG_VALUE, tmpbuf);
    } else {
        db_error("invalid sign_type:%d", msg->sign_type);
        return UNSUPPORT_MSG_UPGRADE_TRY_AGAIN;
    }

    if (msg->sign_type == 1) {
        view->flag |= 0x1;
        //fee
        view_add_txt(TXS_LABEL_SIMPLE_FEE_TITLE, res_getLabel(LANG_LABEL_TXS_FEED_TITLE));

        uint64_t multiply = 0;
        if (uint64_safe_multiply(msg->gas_limit, msg->gas_price, &multiply)) {
            format_coin_real_value(tmpbuf, sizeof(tmpbuf), msg->gas_limit * msg->gas_price, 18);
        } else {
            db_msg("feed override");
            int64_t base = (int64_t) pow(10, 3);
            double feed = 0;
            if (msg->gas_price > msg->gas_limit) {
                double price = (double) msg->gas_price / base;
                feed = msg->gas_limit * price;
                db_msg("base:%lld, price:%f, feed:%f", base, price, feed);
            } else {
                double limit = (double) msg->gas_limit / base;
                feed = msg->gas_price * limit;
                db_msg("base:%lld, limit:%f, feed:%f", base, limit, feed);
            }
            format_coin_real_value(tmpbuf, sizeof(tmpbuf), feed, 15);
        }

        db_msg("feed value:%s", tmpbuf);
        view_add_txt(TXS_LABEL_SIMPLE_FEE_VALUE, tmpbuf);
    }

	//save coin info
    if ((trans_token || (msg->token.extra_type == TOKEN_EXTRA_TYPE_COIN) || is7702) && coin_type && view->msg_from == MSG_FROM_QR_APP && (coin_type != COIN_TYPE_CUSTOM_EVM)) {
        if (!storage_isCoinExist(coin_type, coin_uname)) {
			if (config) {
                storage_save_coin_info(config);
			} else {
                DBCoinInfo dbinfo;
				memset(&dbinfo, 0, sizeof(dbinfo));
				dbinfo.type = (uint8_t) coin_type;
                if ((!IS_VALID_COIN_TYPE(coin_type)) && msg->coin.category == COIN_CATEGORY_EVM) {
                    dbinfo.curv = CURVE_SECP256K1;
                    dbinfo.flag |= DB_FLAG_UNIVERSAL_EVM;
                } else {
                    dbinfo.curv = coin_get_curv_id(coin_type, coin_uname);
                }
                db_msg("db curv:%d, flag:%#x", dbinfo.curv, dbinfo.flag);
				dbinfo.decimals = coin_decimals;
				strncpy(dbinfo.uname, coin_uname, COIN_UNAME_MAX_LEN);
				strncpy(dbinfo.name, name, COIN_NAME_MAX_LEN);
				strncpy(dbinfo.symbol, symbol, COIN_SYMBOL_MAX_LEN);
				storage_save_coin_dbinfo(&dbinfo);
			}
		}
	}
	return 0;
}

#endif