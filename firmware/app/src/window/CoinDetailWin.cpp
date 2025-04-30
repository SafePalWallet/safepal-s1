#define LOG_TAG "CoinDetailWin"

#include "CoinDetailWin.h"
#include "wallet_manager.h"
#include "minigui_comm.h"
#include "GuiMain.h"
#include "debug.h"
#include "storage_manager.h"
#include "wallet_proto.h"
#include "wallet_util.h"
#include "PicDialog.h"
#include "coin_adapter.h"
#include "wallet_util_hw.h"
#include "coin_util_hw.h"

#include "CommonQRShow.h"
#include "ReceiveAddrList.h"

enum {
	COIND_ICON_COIN = 0,
	COIND_ICON_RECEIVE_BG,
	COIND_ICON_HISTORY_BG,
	COIND_ICON_MAXID,
};

enum {
	COIND_LABEL_COIN_NAME = 0,
	COIND_LABEL_COIN_SYMBOL,
	COIND_LABEL_SIGN_TOTAL,
	COIND_LABEL_RECEIVE,
	COIND_LABEL_HISTORY,
	COIND_LABEL_MAXID,
};

CoinDetailWin::CoinDetailWin() {
	singleAddress = false;
	mNaviIndex = 0;
	mCoinChanged = 0;
	mSignTotal = 0;
	mCoinType = 0;
	mHDNode = NULL;
	memset(mCoinUname, 0, sizeof(mCoinUname));
	memset(mAddressUname, 0, sizeof(mAddressUname));

	mBitmapLogo = (BITMAP *) calloc(1, sizeof(BITMAP));

	int icon_mk_map[COIND_ICON_MAXID] = {
			MK_coind_icon,
			MK_coind_receive_bg,
			MK_coind_history_bg,
	};

	int label_mk_map[COIND_LABEL_MAXID] = {
			MK_coind_label_coin_name,
			MK_coind_label_coin_symbol,
			MK_coind_label_sign_total,
			MK_coind_label_receive,
			MK_coind_label_history,
	};

	initLayout(COIND_ICON_MAXID, icon_mk_map, COIND_LABEL_MAXID, label_mk_map);
}

int CoinDetailWin::onCreate(HWND hWnd) {
	createLayoutWidgets(hWnd);
	updateIcon(COIND_ICON_RECEIVE_BG, 1);
	updateIcon(COIND_ICON_HISTORY_BG, 0);
	return 0;
}

int CoinDetailWin::onResume() {
	setLabelText(COIND_LABEL_HISTORY, res_getLabel(LANG_LABEL_COIN_SIGN_HISTORY));
	if (mCoinType == COIN_TYPE_EOS) {
		setLabelText(COIND_LABEL_RECEIVE, "Public Key");
	} else {
		setLabelText(COIND_LABEL_RECEIVE, res_getLabel(LANG_LABEL_COIN_SIGN_RECEIVE));
	}
	if (mCoinChanged || Global_Have_New_DBTX2) {
		Global_Have_New_DBTX2 = 0;
		mCoinChanged = 0;
		refreshCoinInfo();
		if (mNaviIndex) {
			naviButton(0);
		}
	} else {
		char tmpbuf[20];
		snprintf(tmpbuf, sizeof(tmpbuf), res_getLabel(LANG_LABEL_COIN_SIGN_TOTAL), mSignTotal); // lang changed
		setLabelText(COIND_LABEL_SIGN_TOTAL, tmpbuf, 1);
	}

	if (mNaviIndex && !mSignTotal) {
		naviButton(0);
	}
	return 0;
}

int CoinDetailWin::refreshCoinInfo() {
	db_msg("originalCoinType:%d originalCoinUname:%s", originalCoinType, originalCoinUname);
	if (is_empty_string(originalCoinUname)) {
		db_error("invalid info originalCoinType:%d originalCoinUname:%s", originalCoinType, originalCoinUname);
		return -1;
	}
	char tmpbuf[64];
	DBTxFilter filter;
	memset(&filter, 0, sizeof(filter));
	filter.coin_type = originalCoinType;
	filter.coin_uname = get_coin_main_uname(originalCoinType, originalCoinUname);

	mSignTotal = storage_getTxsCount(&filter);
	if (mSignTotal < 0) {
		db_error("get txcount false type:%d uname:%s", originalCoinType, originalCoinUname);
		mSignTotal = 0;
	}
	const char *name = "";
	const char *symbol = "";
	DBCoinInfo dbcoin;
	const CoinConfig *config = getCoinConfig(originalCoinType, originalCoinUname);
	if (config) {
		name = config->name;
		symbol = config->symbol;
	}
	if (is_empty_string(name)) {
		memset(&dbcoin, 0, sizeof(dbcoin));
		storage_getCoinInfo(originalCoinType, originalCoinUname, &dbcoin);
		name = dbcoin.name;
		symbol = dbcoin.symbol;
	}

	if (strlen(name) < 18) {
		snprintf(tmpbuf, sizeof(tmpbuf), "            %s", name);
		setLabelText(COIND_LABEL_COIN_NAME, tmpbuf, 1);
	} else {
		setLabelText(COIND_LABEL_COIN_NAME, name, 1);
	}
	setLabelText(COIND_LABEL_COIN_SYMBOL, symbol, 1);

	snprintf(tmpbuf, sizeof(tmpbuf), res_getLabel(LANG_LABEL_COIN_SIGN_TOTAL), mSignTotal);
	setLabelText(COIND_LABEL_SIGN_TOTAL, tmpbuf, 1);

    int coin_type = originalCoinType;
    db_msg("coin_type:%#x, global_category:%d", coin_type, GLobal_Is_Coin_EVM_Category);
    if ((!IS_VALID_COIN_TYPE(coin_type)) && GLobal_Is_Coin_EVM_Category) {
        coin_type += COIN_TYPE_UNIVERSAL_EVM_BASE;
    }
	if (mBitmapLogo && mIcons[COIND_ICON_COIN] && get_coin_icon_path(coin_type, originalCoinUname, tmpbuf, sizeof(tmpbuf)) > 0) {
        db_msg("tmpbuf:%s", tmpbuf);
        res_unloadBmp(mBitmapLogo);
		res_loadBmp(tmpbuf, mBitmapLogo);
		mIcons[COIND_ICON_COIN]->update(originalCoinType, true, mBitmapLogo);
	}
	return 0;
}

int CoinDetailWin::onPause() {
	return 0;
}

CoinDetailWin::~CoinDetailWin() {
	if (mBitmapLogo) {
		UnloadBitmap(mBitmapLogo);
		free(mBitmapLogo);
	}
    GLobal_Is_Coin_EVM_Category = 0;
}

PROC_RET CoinDetailWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_SET_COIN_INFO:
			onSetCoinInfo(wParam, (const char *) lParam, true);
			break;
		default:
			break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int CoinDetailWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d isLongPress:%d gProcessing:%d", keyCode, isLongPress, gProcessing);
	if (gProcessing) {
		return 0;
	}
	switch (keyCode) {
		case INPUT_KEY_LEFT:
			GLobal_CoinsWin_EditMode = 0;
			return WINDOWID_COINS_MANAGER;
			break;
		case INPUT_KEY_OK:
			if (!gProcessing) {
				gProcessing = 1;
				onClickOK();
				gProcessing = 0;
			}
			break;
		case INPUT_KEY_DOWN:
			naviButton(1);
			break;
		case INPUT_KEY_UP:
			naviButton(-1);
			break;
		default:
			break;
	}
	return 0;
}

int CoinDetailWin::naviButton(int dst) {
	int idmap[] = {
			COIND_ICON_RECEIVE_BG,
			COIND_ICON_HISTORY_BG
	};
	int max_index = ARRAY_SIZE(idmap) - 1;
	int newindex;
	if (dst) {
		newindex = mNaviIndex + dst;
	} else {
		newindex = 0;
	}
	if (newindex < 0) {
		newindex = max_index;
	} else if (newindex > max_index) {
		newindex = 0;
	}
	if (newindex == mNaviIndex) {
		return 0;
	}
	updateIcon(idmap[mNaviIndex], 0, true);
	updateIcon(idmap[newindex], 1, true);
	mNaviIndex = newindex;
	return 0;
}

int CoinDetailWin::onSetCoinInfo(int type, const char *uname, bool force) {
    int temp_type = type;
    const char *temp_uname = uname;
	if (force) {
		singleAddress = false;
		originalCoinType = type;
		strlcpy(originalCoinUname, uname, sizeof(originalCoinUname));
		if (type == COIN_TYPE_BRC20 || type == COIN_TYPE_RUNE) {
			temp_type = COIN_TYPE_BITCOIN;
			temp_uname = "BTC";
			singleAddress = true;
		} else if (type == COIN_TYPE_RUNE_TEST) {
			temp_type = COIN_TYPE_BITCOIN_TEST;
			temp_uname = "tBTC";
			singleAddress = true;
		}

        if (type == COIN_TYPE_BITCOIN) {
            if (!strcmp(temp_uname, "BTC") || !strcmp(temp_uname, COIN_UNAME_BTC2) || !strcmp(temp_uname, COIN_UNAME_BTC3) || !strcmp(temp_uname, COIN_UNAME_BTC4)) {
                if (gSettings->mBtcReceiveAddrType == 0) {
                    temp_uname = "BTC";
                } else if (gSettings->mBtcReceiveAddrType == 1) {
                    temp_uname = COIN_UNAME_BTC2;
                } else if (gSettings->mBtcReceiveAddrType == 2) {
                    temp_uname = COIN_UNAME_BTC3;
                } else if (gSettings->mBtcReceiveAddrType == 3) {
                    temp_uname = COIN_UNAME_BTC4;
                }
            }
        }
	}
	db_msg("onSetCoinInfo originalCoinType:%d type:%d singleAddress:%d", originalCoinType, type, singleAddress);

	int change = (type == COIN_TYPE_RUNE || type == COIN_TYPE_BRC20 || type == COIN_TYPE_BITCOIN) ? 1 : 0;
	if (temp_type == 0 || !temp_uname) {
		if (mCoinType) {
			change = 1;
		}
	} else if (mCoinType != temp_type || strcmp(temp_uname, mCoinUname) != 0) {
		change = 1;
	}
	if (change) {
		mCoinChanged = 1;
		mCoinType = temp_type;
		if (temp_uname) {
			strlcpy(mCoinUname, temp_uname, sizeof(mCoinUname));
		} else {
			memset(mCoinUname, 0, sizeof(mCoinUname));
		}
		memcpy(mAddressUname, mCoinUname, sizeof(mAddressUname));
		if (mCoinType == COIN_TYPE_SOLANA && !getCoinConfig(mCoinType, mCoinUname)) {
			strcpy(mAddressUname, "SOL");
		}
	}
	return 0;
}

static int genAddressFromIndex(void *user, char *address, int size, uint32_t index) {
	if (!user) {
		db_error("user is NULL");
		return -1;
	}
	return ((CoinDetailWin *) (user))->genAddress(address, size, index);
}

int CoinDetailWin::genAddress(char *address, int size, uint32_t index) {
	if (!mHDNode) {
		db_error("mHDNode is NULL");
		return -1;
	}
	return wallet_genAddress(address, size, mHDNode, mCoinType, mAddressUname, index, 0);
}

int CoinDetailWin::showReceiveAddrList() {
	int ret = 0;
	if (mHDNode) {
		free(mHDNode);
		mHDNode = NULL;
	}
	mHDNode = (HDNode *) calloc(1, sizeof(HDNode));
	if (!mHDNode) {
		db_error("new hdnode false");
        dialog_error3(mHwnd, -401, "QR code generated failed.");
		return -1;
	}
	const char *addr_uname = mAddressUname;
    if (GLobal_Is_Coin_EVM_Category) {
        ret = wallet_getHDNode(COIN_TYPE_ETH, "ETH", mHDNode);
    } else {
        ret = wallet_getHDNode(mCoinType, addr_uname, mHDNode);
    }
	if (ret == -404) {
		unsigned char passhash[PASSWD_HASHED_LEN] = {0};
		ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, PASSKB_FLAG_RANDOM);
		if (ret < 0) {
			memzero(passhash, sizeof(passhash));
			db_error("input passwd ret:%d", ret);
			return 0;
		}
		ret = wallet_genDefaultPubHDNode(passhash, mCoinType, addr_uname);
		memzero(passhash, sizeof(passhash));
		if (ret == 0) { //read again
			ret = wallet_getHDNode(mCoinType, addr_uname, mHDNode);
		}
	}
	if (ret != 0) {
		db_error("get hdnode false type:%d uname:%s ret:%d", mCoinType, addr_uname, ret);
		free(mHDNode);
		mHDNode = NULL;
        dialog_error3(mHwnd, -402, "QR code generated failed.");
		return -1;
	}
	if (!GLobal_PIN_Passed) { //check passwd here,skip input passwd 2 times
		if (checkPasswdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PASSKB_FLAG_RANDOM) != 0) {
			free(mHDNode);
			mHDNode = NULL;
			return 0;
		}
	}
	char address[MAX_ADDR_SIZE];
	int selectIndex = 0;
	int default_qrstyle = 0;
	int ext_style = 0;
	int addr_type = coin_account_type(mCoinType, addr_uname);
	if (addr_type == COIN_ADDR_TYPE_UTX) {
		default_qrstyle = COMMON_QR_STYLE_NAVBAR;
		ext_style = COMMON_QR_STYLE_UPDOWN_KEY;
	}
    if (is_coin_address_long(mCoinType, addr_uname)) { //long address show small
        if (mCoinType == COIN_TYPE_CARDANO) {
            ext_style |= COMMON_QR_STYLE_NAVBAR_NO_BAR;//small QR
        } else {
            ext_style |= COMMON_QR_STYLE_FONT12;
        }
    }
	int right_key = 0;
	int bitcoin_max_index = 0;
	if (IS_BTC_COIN_TYPE(mCoinType)) {
        if (!strcmp(addr_uname, "BTC") || !strcmp(addr_uname, COIN_UNAME_BTC2) || !strcmp(addr_uname, COIN_UNAME_BTC3) || !strcmp(addr_uname, COIN_UNAME_BTC4)) {
            int type = gSettings->mBtcReceiveAddrType;
            if (type == 0) {
                ext_style |= COMMON_QR_STYLE_RIGHT_SEGWIT;
                right_key = 2;
            } else if (type == 1) {
                ext_style |= COMMON_QR_STYLE_RIGHT_NSEGWIT;
                right_key = 2;
            } else if (type == 2) {
                ext_style |= COMMON_QR_STYLE_RIGHT_TAPROOT;
                right_key = 2;
            } else if (type == 3) {
                ext_style |= COMMON_QR_STYLE_RIGHT_LEGACY;
                right_key = 1;
            }
        } else if (!strcmp(addr_uname, COIN_UNAME_LTC2)) {
            ext_style |= COMMON_QR_STYLE_RIGHT_NSEGWIT_LTC;
            right_key = 2;
        } else if (!strcmp(addr_uname, "LTC")) {
            ext_style |= COMMON_QR_STYLE_RIGHT_LEGACY_LTC;
            right_key = 1;
        } else if (!strcmp(addr_uname, "BCH")) {
            ext_style |= COMMON_QR_STYLE_RIGHT_LEGACY_BCH;
            right_key = 1;
        } else if (!strcmp(addr_uname, COIN_UNAME_BCH2)) {
            ext_style |= COMMON_QR_STYLE_RIGHT_CASH;
            right_key = 2;
        }

		const CoinConfig *bitCoinConfig = getCoinConfig(COIN_TYPE_BITCOIN, addr_uname);
		if (bitCoinConfig) {
			bitcoin_max_index = storage_get_coin_max_index(wallet_AccountId(), bitCoinConfig->id);
		}
		if (bitcoin_max_index > 0) selectIndex = bitcoin_max_index + 1;

        if (!strcmp(addr_uname, COIN_UNAME_BTC4) || singleAddress == true || (gSettings->mBtcMultiAddress == 0)) {
			selectIndex = 0;
			bitcoin_max_index = 0;
			ext_style |= COMMON_QR_STYLE_RIGHT_SINGLE;
		}
	} else if (mCoinType == COIN_TYPE_SOLANA) {
		if (!strcmp(addr_uname, "SOL")) {
			ext_style |= COMMON_QR_STYLE_RIGHT_OPTIONAL;
			right_key = 2;
		} else if (!strcmp(addr_uname, COIN_UNAME_SOL2)) {
			ext_style |= COMMON_QR_STYLE_RIGHT_DEFAULT;
			right_key = 2;
		}
		default_qrstyle = COMMON_QR_STYLE_NAVBAR;
	}

	int qrstyle = default_qrstyle;
    if (GLobal_Is_Coin_EVM_Category) {
        ret = wallet_genAddress(address, sizeof(address), mHDNode, COIN_TYPE_ETH, "ETH", selectIndex, 0);
    } else {
        ret = wallet_genAddress(address, sizeof(address), mHDNode, mCoinType, addr_uname, selectIndex, 0);
    }
	if (ret <= 0) {
		db_error("genAddress false type:%d uname:%s index:%d ret:%d", mCoinType, addr_uname, selectIndex, ret);
		free(mHDNode);
		mHDNode = NULL;
        dialog_error3(mHwnd, -403, "QR code generated failed.");
        return -1;
	}
	do {
		ret = showCommonQR(mHwnd, (const unsigned char *) address, strlen(address), address, qrstyle | ext_style);
		if (ret == KEY_EVENT_BACK) {
			break;
		}
		if (ret == KEY_EVENT_OK) {
			if (qrstyle == default_qrstyle) {
				qrstyle = COMMON_QR_STYLE_FULL;
			} else {
				qrstyle = default_qrstyle;
			}
			continue;
		}
        if (right_key && ret == KEY_EVENT_NEXT) {
            //save BTC next addr type
            if (!strcmp(addr_uname, "BTC")) {
                settings_save(SETTING_KEY_BTC_RECV_ADDRESS_TYPE, 1);
            } else if (!strcmp(addr_uname, COIN_UNAME_BTC2)) {
                settings_save(SETTING_KEY_BTC_RECV_ADDRESS_TYPE, 2);
            } else if (!strcmp(addr_uname, COIN_UNAME_BTC3)) {
                settings_save(SETTING_KEY_BTC_RECV_ADDRESS_TYPE, 3);
            } else if (!strcmp(addr_uname, COIN_UNAME_BTC4)) {
                settings_save(SETTING_KEY_BTC_RECV_ADDRESS_TYPE, 0);
            }

            ret = 302; //special code
            break;
        }
		if ((ret == KEY_EVENT_UP || ret == KEY_EVENT_DOWN) && (addr_type == COIN_ADDR_TYPE_UTX)) {
			if (IS_BTC_COIN_TYPE(mCoinType) && strcmp(addr_uname, COIN_UNAME_BTC4)) {
				ret = showAddrList(mHwnd, bitcoin_max_index + 10, bitcoin_max_index + 1, address, &selectIndex, genAddressFromIndex, this);
			} else {
				ret = showAddrList(mHwnd, 10, 1, address, &selectIndex, genAddressFromIndex, this);
			}
			db_msg("ret:%d select index:%d address:%s", ret, selectIndex, address);
			if (ret == KEY_EVENT_OK) {
				continue;
			}
		} else if (ret < 0) {
			break;
		}
	} while (1);

	free(mHDNode);
	mHDNode = NULL;

	return ret;
}

int CoinDetailWin::changeCoinAlias() {
	const char *p;
	if (strcmp(mCoinUname, mAddressUname) != 0) {
		p = get_coin_next_alias_uname(mCoinType, mAddressUname);
		if (p) {
			strlcpy(mAddressUname, p, sizeof(mAddressUname));
			return 1;
		}
		return 0;
	}
	p = get_coin_next_alias_uname(mCoinType, mCoinUname);
	if (p) {
		onSetCoinInfo(mCoinType, p, false);
		return 1;
	} else {
		return 0;
	}
}

int CoinDetailWin::onClickOK() {
	int ret = 0;
	int flag = 0;

	if (mNaviIndex == 0) { //receive
		do {
			ret = showReceiveAddrList();
			if (ret == 302 && changeCoinAlias() == 1) {

			} else {
				break;
			}
		} while (1);
	} else if (mNaviIndex == 1) {
		if (mSignTotal > 0) {
			if (!GLobal_PIN_Passed) {
				if (checkPasswdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PASSKB_FLAG_RANDOM) != 0) {
					return 0;
				}
			}
			GuiMain::getInstance()->sendMessage(WINDOWID_SIGN_HISTORY, MSG_SET_COIN_INFO, originalCoinType, (LPARAM) get_coin_main_uname(originalCoinType, originalCoinUname));
			changeWindow(WINDOWID_SIGN_HISTORY);
		} else {
			do {
				ret = picDialog(mHwnd, "no_signature_tips", NULL, NULL, 0);
				if (ret == KEY_EVENT_BACK || !ret) {
					flag = 1;
				}
			} while (!flag);
		}
	} else {
        dialog_error3(mHwnd, -201, "Coin list generated failed.");
    }
	return 0;
}
