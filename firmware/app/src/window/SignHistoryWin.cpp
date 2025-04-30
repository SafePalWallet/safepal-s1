#define LOG_TAG "SignHistoryWin"

#include "SignHistoryWin.h"
#include "minigui_comm.h"
#include "GuiMain.h"
#include "debug.h"
#include "storage_manager.h"
#include "wallet_proto_qr.h"
#include "wallet_util_hw.h"
#include "PicDialog.h"
#include "ActionSheetDialog.h"

#define IDC_LIST_PARENT_CONTAINER 650
#define SIGNH_ITEM_SIZE 10
enum {
	SIGNH_LABEL_NAVI,
	SIGNH_LABEL_MAXID,
};

SignHistoryWin::SignHistoryWin() {
	mContainer = HWND_INVALID;
	mViewOffet = 0;
	mViewTotal = 0;
	mTxTotal = 0;
	mListView = NULL;
	mCoinType = 0;
	memset(mCoinUname, 0, sizeof(mCoinUname));
	memset(&mFilter, 0, sizeof(DBTxFilter));
	mBitmapGroup = new BitmapGroup();
	memset(&mTxs, 0, sizeof(mTxs));
	mSelectTxType = 0;
	mLastSelectTxType = 0;
	int label_mk_map[SIGNH_LABEL_MAXID] = {
			MK_sign_history_navi_text,
	};

	initLayout(0, NULL, SIGNH_LABEL_MAXID, label_mk_map);
}

int SignHistoryWin::onCreate(HWND hWnd) {
	createLayoutWidgets(hWnd);
	WIN_RECT rect;
	res_getPos(MK_sign_history_list, &rect);
	db_msg("MK_sign_history_list x:%d y:%d w:%d h:%d", rect.x, rect.y, rect.w, rect.h);
	mContainer = createWidgetWindow(hWnd, 0, rect.x, rect.y, rect.w, rect.h, IDC_LIST_PARENT_CONTAINER, WIDGET_TYPE_NORMAL, 0, -1);
	if (IS_VALID_HWND(mContainer)) {
		SetWindowBkColor(mContainer, res_getBGColor());
		mListView = new ListView(mContainer, MK_sign_history_container);
	}
	return 0;
}

int SignHistoryWin::showSelectTxType() {
	if (mCoinType && mCoinUname[0]) {
		return 0;
	}
	const int item_count = 3;
	const char *items[item_count];
	items[0] = res_getLabel(LANG_LABEL_TX_RECORD_TRANSACTION);
	items[1] = res_getLabel(LANG_LABEL_TX_RECORD_AUTH);
	items[2] = res_getLabel(LANG_LABEL_TX_RECORD_ORDER);
	ActionSheetDialogConfig_t _config;
	ActionSheetDialogConfig_t *config = &_config;
	memset(config, 0, sizeof(ActionSheetDialogConfig_t));
	config->itemsCnt = item_count;
	config->items = items;
	config->initIndex = mLastSelectTxType;
	config->title.data = "";
	//config->flag = ACTION_SHEET_DIALOG_FLAG_NAVI;

	int ret = showActionSheetDialog(mHwnd, config);
	db_msg("select type:%d", ret);
	if ((ret >= 0 && ret < item_count)) {
		if (!GLobal_PIN_Passed && checkPasswdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PASSKB_FLAG_RANDOM) != 0) {
			return -1;
		}
		switch (ret) {
			case 1://auth
				onSetCoinInfo(TX_LIST_COIN_TYPE_AUTH, "AUTH");
				break;
			case 2://trade
				onSetCoinInfo(TX_LIST_COIN_TYPE_TRADE, "TRADE");
				break;
			default:
				onSetCoinInfo(TX_LIST_COIN_TYPE_TRANS, "TRANS");
		}
		mSelectTxType = ret;
		mLastSelectTxType = ret;
		return 0;
	} else {
		db_error("select TX type false %d", ret);
		return -1;
	}
}

int SignHistoryWin::onResume() {
	db_msg("onResume");
	if (mCoinType == 0 && showSelectTxType() != 0) {
		changeWindow(WINDOWID_MAINPANEL);
		return 0;
	}
	int refresh = 0;
	//set_support_long_key(1);
	if (!mTxTotal || Global_Have_New_DBTX) {
		updateTxTotal();
		refresh = 1;
	}
	Global_Have_New_DBTX = 0;
	if (mTxTotal > 0) {
		if (refresh) {
			flushSignHistoryList();
		}
	} else {
		//tips not sign
		showNotSignTips();
		mCoinType = 0; //select again
		onResume();
	}
	return 0;
}

int SignHistoryWin::onPause() {
	//set_support_long_key(0);
	return 0;
}

SignHistoryWin::~SignHistoryWin() {
	if (mListView) {
		mListView->clean();
		delete mListView;
		mListView = NULL;
	}
}

int SignHistoryWin::updateTxTotal() {
	mTxTotal = storage_getTxsCount(getDBTxFilter());
	db_msg("tx total:%d", mTxTotal);
	return mTxTotal;
}

int SignHistoryWin::flushSignHistoryList(int init_select) {
	const int icon_mk[] = {MK_sign_history_icon, MK_sign_history_navi, MK_sign_order_type};
	int text_mk[] = {MK_sign_history_code, MK_sign_history_time, MK_sign_history_account, MK_sign_history_legal};
	if (mSelectTxType == 1) { //auth
		text_mk[0] = MK_sign_history_code_s;
		text_mk[1] = MK_sign_history_auth_time;
		text_mk[2] = MK_sign_history_auth_amount;
	} else if (mSelectTxType == 2) {//trade
		text_mk[2] = MK_sign_history_order_price;
		text_mk[3] = MK_sign_history_order_amount;
	}
	if (mViewOffet < 0 || mViewOffet >= mTxTotal) mViewOffet = 0;
	mViewTotal = storage_queryTxsInfo(&mTxs[0], SIGNH_ITEM_SIZE, mViewOffet, getDBTxFilter());
	db_msg("view offset:%d total:%d txtotal:%d", mViewOffet, mViewTotal, mTxTotal);

	if (mViewTotal < 1) {
		if (mViewTotal < 0) {
			mViewTotal = 0;
		}
		mListView->clean();

		if (mTxTotal && mViewOffet) {
			mViewOffet = 0;
			updateTxTotal();
			if (mTxTotal > 0) {
				db_msg("reflush sign history list");
				flushSignHistoryList();
			}
		}
		return 0;
	}
	db_msg("view total:%d", mViewTotal);
	mListView->init(mViewTotal, ARRAY_SIZE(icon_mk), icon_mk, ARRAY_SIZE(text_mk), text_mk);
	BITMAP *navi_bitmap = mBitmapGroup->getBmp(MK_sign_history_navi, "icon0");
	BITMAP *buy_bitmap = mBitmapGroup->getBmp(MK_sign_order_type, "icon1");
	BITMAP *sell_bitmap = mBitmapGroup->getBmp(MK_sign_order_type, "icon2");
	BITMAP *cancel_bitmap = mBitmapGroup->getBmp(MK_sign_order_type, "icon3");
	char str[128];
	int i;
	DBTxInfo *tx;
    int coin_type;
	for (i = 0; i < mViewTotal; i++) {
		tx = &mTxs[i];
		memset(str, 0, sizeof(str));

        // img
        coin_type = tx->coin_type;
        if (tx->flag & DB_FLAG_NFT) {
            coin_type += COIN_TYPE_NFT_BASE;
        } else if ((!IS_VALID_COIN_TYPE(coin_type)) && (tx->flag & DB_FLAG_UNIVERSAL_EVM)) {
            coin_type += COIN_TYPE_UNIVERSAL_EVM_BASE;
        }
		int ret = get_coin_icon_path(coin_type, tx->coin_uname, str, sizeof(str));
        db_msg("tx->flag=%x,coin_type=%d,ret=%d",tx->flag,coin_type,ret);
        mListView->setIconImage(i, 0, mBitmapGroup->getBmp(str));
		mListView->setLabelText(i, 0, tx->coin_symbol);
		mListView->setIconImage(i, 1, navi_bitmap);
		// time
		format_time(str, sizeof(str), tx->time, tx->time_zone, 0);
		mListView->setLabelText(i, 1, str);
		//setLabelTextColor(mListView->getLabelHwnd(i, 1), R->getDisableColor());
		if (mSelectTxType == 1) { //auth
			str[0] = 0;
			if (tx->tx_type == TX_TYPE_SIGN_MSG) {
				snprintf(str, sizeof(str), "%s", res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG));
			} else if (tx->tx_type == TX_TYPE_LOGIN) {
				snprintf(str, sizeof(str), "%s %s", tx->coin_uname, res_getLabel(LANG_LABEL_TX_METHOD_LOGIN));
			} else if (tx->tx_type == TX_TYPE_WITHDRAW) {
				snprintf(str, sizeof(str), "%s %s", tx->coin_uname, res_getLabel(LANG_LABEL_TX_METHOD_WITHDRAW));
			}
			if (str[0] && strcmp(tx->coin_symbol, str) != 0) {
				mListView->setLabelText(i, 0, str);
			}
			if (tx->tx_type == TX_TYPE_APP_APPROVAL) {
				snprintf(str, sizeof(str), "%s %s", res_getLabel(LANG_LABEL_TX_METHOD_APPROVE), tx->send_value);
			} else if (tx->tx_type == TX_TYPE_APP_SIGN_MSG) {
				snprintf(str, sizeof(str), "%s", res_getLabel(LANG_LABEL_TX_METHOD_SIGN_MSG));
			} else {
				snprintf(str, sizeof(str), "%s", tx->send_value);
			}
			mListView->setLabelText(i, 2, str);
		} else if (mSelectTxType == 2) { //trade
			db_msg("flag:%d", tx->flag);
			if (tx->flag & 4) {
				mListView->setIconImage(i, 2, cancel_bitmap);
			} else if (tx->flag & 1) {
				mListView->setIconImage(i, 2, buy_bitmap);
			} else if (tx->flag & 2) {
				mListView->setIconImage(i, 2, sell_bitmap);
			} else {
				mListView->setIconImage(i, 2, NULL);
			}
			snprintf(str, sizeof(str), "%s %s", res_getLabel(LANG_LABEL_ORDER_PRICE), tx->currency_value);
			mListView->setLabelText(i, 2, str);
			setLabelTextColor(mListView->getLabelHwnd(i, 2), res_getTextColor(0));

			snprintf(str, sizeof(str), "%s %s", res_getLabel(LANG_LABEL_ORDER_AMOUNT), tx->send_value);
			mListView->setLabelText(i, 3, str);
			setLabelTextColor(mListView->getLabelHwnd(i, 3), res_getTextColor(0));
		} else {
			mListView->setIconImage(i, 2, NULL);

			mListView->setLabelText(i, 2, tx->send_value);
			setLabelTextColor(mListView->getLabelHwnd(i, 2), res_getTextHColor(0));

			if (tx->currency_value[0]) {
				if (tx->currency_symbol[0] == 'K' && tx->currency_symbol[1] == 'B' && tx->currency_symbol[2] == 0) { //show XX KB
					snprintf(str, sizeof(str), "≈%s %s", tx->currency_value, tx->currency_symbol);
				} else {
					snprintf(str, sizeof(str), "≈%s%s", tx->currency_symbol, tx->currency_value);
				}
				db_msg("currency_value:%s str:%s", tx->currency_value, str);
			} else {
				str[0] = 0;
			}
			mListView->setLabelText(i, 3, str);
			setLabelTextColor(mListView->getLabelHwnd(i, 3), res_getDisableColor());
		}
	}

	if (init_select < mViewTotal) {
		mListView->select(init_select);
	} else {
		mListView->select(0);
	}
	updateNaviText();
	return 0;
}

int SignHistoryWin::updateNaviText() {
	char str[64];
	snprintf(str, sizeof(str), "%d / %d", mListView->getSelectIndex() + mViewOffet + 1, mTxTotal);
	setLabelText(SIGNH_LABEL_NAVI, str);
	db_msg("navi:%s", str);
	return 0;
}

int SignHistoryWin::showNotSignTips() {
	int ret = 0;
	int flag = 0;
	do {
		ret = picDialog(mHwnd, "no_signature_tips", NULL, NULL, 0);
		if (ret == KEY_EVENT_BACK || !ret) {
			flag = 1;
		}
	} while (!flag);

	return ret;
}

PROC_RET SignHistoryWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_SET_COIN_INFO:
			onSetCoinInfo(wParam, (const char *) lParam);
			break;
		default:
			break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int SignHistoryWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d isLongPress:%d", keyCode, isLongPress);
	int selectindex;
	switch (keyCode) {
		case INPUT_KEY_LEFT:
			if (mCoinType > 0 && is_not_empty_string(mCoinUname) && IS_VALID_HWND(win_get_hwnd(WINDOWID_COIN_DETAIL))) {
				return WINDOWID_COIN_DETAIL;
			} else {
				mCoinType = 0;
				onResume();
				return 0;
				//return WINDOWID_MAINPANEL;
			}
			break;
		case INPUT_KEY_OK:
		case INPUT_KEY_RIGHT: {
			onClickTx();
		}
			break;
		case INPUT_KEY_DOWN: {
			if (isLongPress) {
				if (mViewOffet + mViewTotal < mTxTotal) {
					selectindex = mTxTotal % SIGNH_ITEM_SIZE;
					if (selectindex) {
						mViewOffet = (mTxTotal / SIGNH_ITEM_SIZE) * SIGNH_ITEM_SIZE;
					} else {
						mViewOffet = mTxTotal - SIGNH_ITEM_SIZE;
						if (mViewOffet < 0) mViewOffet = 0;
					}
					flushSignHistoryList(-1);
				} else {
					mListView->select(-1);
					updateNaviText();
				}
				break;
			}
			selectindex = mListView->getSelectIndex();
			db_msg("selectindex:%d view total:%d", selectindex, mViewTotal);
			if ((selectindex + 1) == mViewTotal && mViewTotal > 0) {
				db_msg("select index:%d0 view offset:%d total:%d txtotal:%d try page down", selectindex, mViewOffet, mViewTotal, mViewTotal);
				if ((mViewOffet + mViewTotal) < mTxTotal) {
					mViewOffet += mViewTotal;
					flushSignHistoryList();
				}
			} else {
				mListView->move(1);
				updateNaviText();
			}
			break;
		}
		case INPUT_KEY_UP: {
			if (isLongPress) {
				if (mViewOffet > 0) {
					mViewOffet = 0;
					flushSignHistoryList(0);
				} else {
					mListView->select(0);
					updateNaviText();
				}
				break;
			}
			selectindex = mListView->getSelectIndex();
			if (selectindex == 0) {
				if (mViewOffet > 0) {
					db_msg("select index0 offset:%d,try page up", mViewOffet);
					mViewOffet -= SIGNH_ITEM_SIZE;
					if (mViewOffet < 0) mViewOffet = 0;
					flushSignHistoryList(-1);
				}
			} else {
				mListView->move(-1);
				updateNaviText();
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

int SignHistoryWin::onClickTx() {
	if (!mViewTotal) {
		db_error("invalid total:%d", mViewTotal);
		return -1;
	}
	int selectindex = mListView->getSelectIndex();
	if (selectindex < 0 || selectindex >= SIGNH_ITEM_SIZE) {
		db_error("invalid index:%d", selectindex);
		return -1;
	}
	int id = mTxs[selectindex].id;
	if (!id) {
		db_error("invalid id:%d", id);
		return -1;
	}
	DBTxInfo tx;
	memset(&tx, 0, sizeof(DBTxInfo));
	if (storage_getTxsInfo(id, &tx) != 1) {
		db_error("get txs info:%d false", id);
		return -1;
	}
	if (!tx.data || !tx.data->str) {
		db_error("get txs:%d empty data", id);
		return -1;
	}
	ProtoClientMessage *msg = proto_client_message_unserialize((const unsigned char *) tx.data->str, tx.data->len);
	cstr_free(tx.data);
	if (!msg) {
		db_error("unserialize client msg false");
		return -1;
	}
	int winid = get_message_process_winid(msg);
	if (!winid) {
		db_error("invalid msg type:%d", msg->type);
		proto_client_message_delete(msg);
		return -1;
	}
    if (winid == WINDOWID_TX_VERIFY_CODE) {
        winid = WINDOWID_TXSHOW;
    }
	GuiMain::getInstance()->sendMessage(winid, MSG_HISTORY_QR_RESULT, msg->type, (LPARAM) msg);
	changeWindow(winid);
	return 0;
}

DBTxFilter *SignHistoryWin::getDBTxFilter() {
	switch (mSelectTxType) {
		case 1://auth
			mFilter.tx_type = 1001;
			break;
		case 2://trade
			mFilter.tx_type = 1;
			break;
		default:
			mFilter.tx_type = 0;
	}
	if (mCoinType) {
		mFilter.coin_type = mCoinType;
		mFilter.coin_uname = get_coin_main_uname(mCoinType, mCoinUname);
		return &mFilter;
	} else {
		mFilter.coin_type = TX_LIST_COIN_TYPE_TRANS;
		mFilter.coin_uname = "";
		return &mFilter;
	}
}

int SignHistoryWin::onSetCoinInfo(int type, const char *uname) {
	int change = 0;
	mSelectTxType = 0;
	if (type == 0 || !uname) {
		if (mCoinType) {
			change = 1;
		}
	} else if (mCoinType != type || strcmp(uname, mCoinUname) != 0) {
		change = 1;
	}
	if (change) {
		mTxTotal = 0; //force refresh
		mCoinType = type;
		if (is_not_empty_string(uname)) {
			strlcpy(mCoinUname, uname, sizeof(mCoinUname));
		} else {
			memset(mCoinUname, 0, sizeof(mCoinUname));
		}
	}
	return 0;
}
