#define LOG_TAG "TxShow"

#include <stdlib.h>
#include <CommonWindow.h>
#include <widgets.h>
#include "minigui_comm.h"
#include "GuiMain.h"
#include "TxShowWin.h"
#include "debug.h"
#include "wallet_util_hw.h"
#include "coin_util_hw.h"

enum {
	TXS_ICON_COIN_TYPE = 0,
	TXS_ICON_NAVI_UP_DOWN,
	TXS_ICON_NAVI_CANCEL,
	TXS_ICON_NAVI_MORE,
	TXS_ICON_NAVI_OK,
	TXS_ICON_MAXID,
};

enum {
	TXS_LABEL_COIN_SYMBOL,
	TXS_LABEL_COIN_NAME,
	TXS_LABEL_CANCEL,
	TXS_LABEL_MORE,
	TXS_LABEL_MAXID,
};

TxShowWin::TxShowWin() {
	int icon_mk_map[TXS_ICON_MAXID] = {
			MK_sign_icon_coin_type,
			MK_sign_icon_navi_up_down,
			MK_sign_icon_navi_cancel,
			MK_sign_icon_navi_more,
			MK_sign_icon_navi_ok,
	};

	int label_mk_map[TXS_LABEL_MAXID] = {
			MK_sign_label_coin_symbol,
			MK_sign_label_coin_name,
			MK_sign_label_cancel,
			MK_sign_label_more,
	};

	memset(mDView, 0, sizeof(DynamicViewCtx));
	memset(mTxp, 0, sizeof(TxPorcessData));

	mHwndNaviPanel = HWND_INVALID;
	mMsgFrom = MSG_FROM_QR_APP;
	mClientMessage = NULL;
	mShowRet = -1;
	mIsShowMore = 0;
	mBitmapLogo = (BITMAP *) calloc(1, sizeof(BITMAP));
	initLayout(TXS_ICON_MAXID, icon_mk_map, TXS_LABEL_MAXID, label_mk_map);
}

TxShowWin::~TxShowWin() {
	if (mBitmapLogo) {
		res_unloadBmp(mBitmapLogo);
	}
}

PROC_RET TxShowWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case MSG_HISTORY_QR_RESULT:
		case MSG_TXSHOW_MSG: {
			mMsgFrom = (message == MSG_HISTORY_QR_RESULT) ? MSG_FROM_QR_HISTORY : MSG_FROM_QR_APP;
			onSignReqData((ProtoClientMessage *) lParam);
		}
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int TxShowWin::keyProc(int keyCode, int isLongPress) {
	switch (keyCode) {
		case INPUT_KEY_OK:
			db_msg("INPUT_KEY_OK");
			if (mScrollSize + mScreenHeight >= mTotalHeight) {
				if (mMsgFrom == MSG_FROM_QR_APP && mShowRet == 0) {
					if (!gProcessing) {
						gProcessing = 1;
						doSignReq();
						gProcessing = 0;
					}
				}
			}
			break;
		case INPUT_KEY_UP:
			scrollWindow(-1);
			break;
		case INPUT_KEY_DOWN:
			scrollWindow(1);
			break;
		case INPUT_KEY_LEFT:
			if (mMsgFrom == MSG_FROM_QR_HISTORY) {
				return WINDOWID_SIGN_HISTORY;
			} else if (mMsgFrom == MSG_FROM_QR_APP && (mScrollSize + mScreenHeight) >= mTotalHeight) {
				return WINDOWID_TX_VERIFY_CODE;
			}
			break;
		case INPUT_KEY_RIGHT:
			if (mScrollSize + mScreenHeight < mTotalHeight) {
				//scrollWindow(1); //like KEY_DOWN
			} else if (mDView->has_more) {
				mIsShowMore = 1;
				GuiMain::getInstance()->sendMessage(WINDOWID_TXMORE, MSG_TXMORE_TXP_MSG, 0, (LPARAM) mTxp);
				return WINDOWID_TXMORE;
			}
			break;
	}
	return 0;
}

int TxShowWin::getIconState(int id) {
	switch (id) {
		case TXS_ICON_NAVI_CANCEL:
		case TXS_ICON_NAVI_MORE:
		case TXS_ICON_NAVI_OK:
			return 0;
	}
	return -1;
}

int TxShowWin::onCreate(HWND hWnd) {
	int ret;
	WIN_RECT rect;

	res_getPos(MK_sign_navi_panel, &rect);
	mHwndNaviPanel = CreateWindowEx(CTRL_STATIC, "",
	                                WS_CHILD | WS_VISIBLE,
	                                WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
	                                1,
	                                rect.x, rect.y, rect.w, rect.h,
	                                hWnd, (DWORD) this);

	if (mHwndNaviPanel == HWND_INVALID) {
		db_error("create status bar failed");
		return -1;
	}
	ShowWindow(mHwndNaviPanel, SW_SHOWNORMAL);

	HWND subs[3];
	subs[0] = mHwndNaviPanel;
	subs[1] = HWND_INVALID;
	subs[2] = HWND_INVALID;
	ret = createLayoutWidgets(hWnd, subs);
	if (ret != 0) {
		db_error("createLayoutWidgets ret:%d", ret);
		return ret;
	}
	return 0;
}

int TxShowWin::onResume() {
	char tmpbuf[128];
	db_msg("resume");
	set_temp_screen_time(60);
	if (!mClientMessage) {
		db_error("invalid client msg");
		return -1;
	}
	if (mIsShowMore) {
		db_msg("jump from show more,skip");
		mIsShowMore = 0;
		return 0;
	}
	freeWinData();
	initDView();
	if (mScrollSize) {
		resetScrollSize();
	}
	int ret = tx_process_client_message(mClientMessage, mTxp);
	if (ret != 0) {
		dialog_system_error2(mHwnd, ret, "init", NULL);
		return 0;
	}
	if (!mTxp->onShow || !mTxp->onInit) {
		db_error("not show or init func");
		return -1;
	}
	mShowRet = mTxp->onInit(mTxp->session);
	if (mShowRet != 0) {
		db_error("TX init ret:%d", mShowRet);
		dialog_error3(mHwnd, mShowRet, "Init tx failed.");
		return -1;
	}
	mShowRet = mTxp->onShow(mTxp->session, mDView);
	db_msg("TX show ret:%d", mShowRet);
    if (mShowRet < 0) {
        if (mShowRet == -181) {
            dialog_error(mHwnd, res_getLabel(LANG_LABEL_WALLET_NO_SUPPORT_TOKEN));
        } else if (mShowRet == UNSUPPORT_MSG_UPGRADE_TRY_AGAIN) {
            dialog_error(mHwnd, res_getLabel(LANG_LABEL_UNSUPPORT_MSG));
        } else {
            dialog_error3(mHwnd, mShowRet, "Show tx info failed.");
        }
        return -1;
    }
	mTotalHeight = mDView->total_height > mScreenHeight ? mDView->total_height : mScreenHeight;

	if (mMsgFrom == MSG_FROM_QR_HISTORY) {
		showIcon(TXS_ICON_NAVI_OK, false);
		setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_BACK));
	} else {
		showIcon(TXS_ICON_NAVI_OK, true);
		setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_BACK));
	}
	if (mDView->has_more) {
		showIcon(TXS_ICON_NAVI_MORE, true);
		setLabelText(TXS_LABEL_MORE, res_getLabel(LANG_LABEL_TXS_MORE), 1);
	} else {
		showIcon(TXS_ICON_NAVI_MORE, false);
		setLabelText(TXS_LABEL_MORE, "", 0);
	}

	if (is_not_empty_string(mDView->coin_symbol)) {
		ret = 1; //font
		if (mDView->flag & 0x1) { //small
			ret = 8;
		}
		SetWindowFont(getLabelHwnd(TXS_LABEL_COIN_SYMBOL), res_getFont(ret));
		setLabelText(TXS_LABEL_COIN_SYMBOL, mDView->coin_symbol, 1);
	} else {
		setLabelText(TXS_LABEL_COIN_SYMBOL, "", 0);
	}
    if (is_not_empty_string(mDView->coin_name)) {
        if (strlen(mDView->coin_name) < 18) {
            snprintf(tmpbuf, sizeof(tmpbuf), "   %s", mDView->coin_name);
            setLabelText(TXS_LABEL_COIN_NAME, tmpbuf, 1);
        } else {
            setLabelText(TXS_LABEL_COIN_NAME, mDView->coin_name, 1);
        }
	} else {
		setLabelText(TXS_LABEL_COIN_NAME, "", 0);
	}

	if (mTotalHeight > mScreenHeight) {
		updateIcon(TXS_ICON_NAVI_UP_DOWN, 1, true);
	} else {
		updateIcon(TXS_ICON_NAVI_UP_DOWN, 0, false);
	}

	//show logo
	PicObj *coin_icon = mIcons[TXS_ICON_COIN_TYPE];
	db_msg("coin_icon:%p mBitmapLogo:%p", coin_icon, mBitmapLogo);
	if (coin_icon && mBitmapLogo) {
		tmpbuf[0] = 0;
		int coin_type = mDView->coin_type;
        if (mDView->db.flag & DB_FLAG_NFT) {
            coin_type += COIN_TYPE_NFT_BASE;
        } else if ((!IS_VALID_COIN_TYPE(mDView->db.coin_type)) && (mDView->db.flag & DB_FLAG_UNIVERSAL_EVM)) {
            coin_type += COIN_TYPE_UNIVERSAL_EVM_BASE;
        }
		ret = get_coin_icon_path(coin_type, mDView->coin_uname, tmpbuf, sizeof(tmpbuf));
        db_msg("ret=%d, tmpbuf=%s,mDView->coin_type=%d",ret, tmpbuf,mDView->coin_type);
		res_unloadBmp(mBitmapLogo);
		if (tmpbuf[0]) {
			res_loadBmp(tmpbuf, mBitmapLogo);
			coin_icon->update(1, true, mBitmapLogo);
		} else {
			coin_icon->hide();
		}
	}
	WIN_RECT rect;
	res_getPos(MK_sign_navi_panel, &rect); //default 2 screen
	int p = (mTotalHeight + mScreenHeight - 1) / mScreenHeight;
	rect.y += (p - 2) * mScreenHeight;
	MoveWindow(mHwndNaviPanel, rect.x, rect.y, rect.w, rect.h, FALSE);
	return 0;
}

int TxShowWin::onPause() {
	set_temp_screen_time(0);
	if (!mIsShowMore) {
		freeWinData();
        if (mClientMessage != NULL && mMsgFrom == MSG_FROM_QR_HISTORY) {
            proto_client_message_delete(mClientMessage);
            mClientMessage = NULL;
        }
    }
	return 0;
}

int TxShowWin::onScrollWindow(int scroll_size) {
	WIN_RECT rect;
	HWND hwnd = getIconHwnd(TXS_ICON_NAVI_UP_DOWN);
	db_msg("current ScrollSize:%d TotalHeight:%d scroll_size:%d", mScrollSize, mTotalHeight, scroll_size);
	if (IS_VALID_HWND(hwnd)) {
		int state = 0;
		if (mScrollSize + mScreenHeight != mTotalHeight) {
			if (mScrollSize + mScreenHeight < mTotalHeight) {
				state |= 1;
			}
			if (mScrollSize > 0) {
				state |= 2;
			}
		}
		res_getPos(MK_sign_icon_navi_up_down, &rect);
		db_msg("state:%d", state);
		if (state) {
			MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
		}
		updateIcon(TXS_ICON_NAVI_UP_DOWN, state);
	}
	return 0;
}

int TxShowWin::initDView() {
	if (!IS_VALID_HWND(mDView->hwnd)) {
		dwin_init(mDView, mHwnd, 10);
		mDView->msg_from = mMsgFrom;
	}
	return 0;
}

int TxShowWin::freeWinData() {
	if (!mTxp->onShow) {
		return 0;
	}
	if (IS_VALID_HWND(mDView->hwnd)) {
		db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
		dwin_destory(mDView);
	}
	if (mTxp->onEnd) {
		db_msg("call tx end");
		mTxp->onEnd(mTxp->session);
	}
	memset(mTxp, 0, sizeof(TxPorcessData));
	return 0;
}

int TxShowWin::onSignReqData(ProtoClientMessage *req) {
	if (req) {
		db_msg("req:%p type:%d flag:%d", req, req->type, req->flag);
	} else {
		db_msg("empey req");
	}
    db_msg("mClientMessage:%p", mClientMessage);
    freeWinData();
	mShowRet = -1;
	mIsShowMore = 0;
	mClientMessage = req;
	if (mClientMessage) {
		initDView();
	}
	return 0;
}

static int tx_save_history(const ProtoClientMessage *msg, DBTxCoinInfo *db) {
	DBTxInfo tx[1];
	tx->msg_type = msg->type;
	tx->time = msg->time;
	tx->time_zone = msg->time_zone;
	tx->client_id = msg->client_id;

	memcpy(&tx->flag, db, sizeof(DBTxCoinInfo)); //struct is same,hack copy

	tx->data = proto_client_message_serialize(msg);
	if (!tx->data) {
		db_error("serialize msg false");
		return -1;
	}
	int ret = storage_saveTxsInfo(tx);
	cstr_free(tx->data);
	if (ret != 0) {
		db_error("saveTxsInfo false");
		return -1;
	}
	return 0;
}

int TxShowWin::doSignReq() {
	db_msg("doSignReq");
	if (mShowRet != 0 || !mTxp->onSign) {
		db_error("invalid state");
		return -1;
	}
	unsigned char passhash[PASSWD_HASHED_LEN] = {0};
	int ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, 1);
	if (ret < 0) {
		db_error("input passwd ret:%d", ret);
		return 0;
	}
	db_secure("passhash:%s", debug_ubin_to_hex(passhash, sizeof(passhash)));
	ret = mTxp->onSign(mTxp->session, mHwnd, passhash);
	memzero(passhash, sizeof(passhash));
	if (ret == 0) {
		db_error("TX sign Success");
		if (mDView->db.coin_type > 0) {
			tx_save_history(mClientMessage, &mDView->db);
		}
		changeWindow(WINDOWID_MAINPANEL);
	} else {
		db_error("TX sign false,ret:%d", ret);
		if (ret < 0) {
			dialog_error3(mHwnd, ret, "Sign tx failed.");
		}
	}
	return ret;
}
