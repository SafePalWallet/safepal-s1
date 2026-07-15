#define LOG_TAG "TxVerifyCode"

#include <stdlib.h>
#include <CommonWindow.h>
#include <widgets.h>
#include "minigui_comm.h"
#include "GuiMain.h"
#include "TxVerifyCodeWin.h"
#include "debug.h"
#include "wallet_util_hw.h"
#include "coin_util_hw.h"
#include "device.h"
#include "storage_manager.h"
#include "wallet_manager.h"
#include "common_util.h"
#include "qr_pack.h"
#include "protobuf_util.h"
#include "sha2.h"
#include "tx_common.h"

enum {
    TXS_ICON_COIN_TYPE = 0,
    TXS_ICON_NAVI_CANCEL,
	TXS_ICON_NAVI_DETAILS,
	TXS_ICON_NAVI_OK,
	TXS_ICON_MAXID,
};

enum {
    TXS_LABEL_COIN_SYMBOL,
    TXS_LABEL_VERIFY_CODE_TIPS,
	TXS_LABEL_VERIFY_CODE,
	TXS_LABEL_CANCEL,
	TXS_LABEL_DETAIL,
	TXS_LABEL_MAXID,
};

TxVerifyCodeWin::TxVerifyCodeWin() {
	int icon_mk_map[TXS_ICON_MAXID] = {
            MK_sign_icon_coin_type,
            MK_sign_icon_navi_cancel,
			MK_sign_icon_navi_more,
			MK_sign_icon_navi_ok,
	};

	int label_mk_map[TXS_LABEL_MAXID] = {
            MK_sign_label_coin_symbol,
            MK_tx_verify_code_tip,
            MK_tx_verify_code_value,
			MK_sign_label_cancel,
			MK_sign_label_more,
	};

    memset(mDView, 0, sizeof(DynamicViewCtx));
    memset(&mDBTxCoinInfo, 0, sizeof(DBTxCoinInfo));
    memset(mTxp, 0, sizeof(TxPorcessData));

	mHwndNaviPanel = HWND_INVALID;
	mClientMessage = NULL;
	mShowRet = -1;
	mIsShowDetails = 0;
    mBitmapLogo = (BITMAP *) calloc(1, sizeof(BITMAP));
    initLayout(TXS_ICON_MAXID, icon_mk_map, TXS_LABEL_MAXID, label_mk_map);
}

TxVerifyCodeWin::~TxVerifyCodeWin() {
    if (mBitmapLogo) {
        res_unloadBmp(mBitmapLogo);
    }
}

PROC_RET TxVerifyCodeWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_QR_RESULT:
			onSignReqData((ProtoClientMessage *) lParam);
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int TxVerifyCodeWin::keyProc(int keyCode, int isLongPress) {
    switch (keyCode) {
        case INPUT_KEY_OK:
            db_msg("INPUT_KEY_OK");
            if (!gProcessing && mShowRet == 0) {
                gProcessing = 1;
                doSignReq();
                gProcessing = 0;
            }
            break;
        case INPUT_KEY_UP:
            break;
        case INPUT_KEY_DOWN:
            break;
        case INPUT_KEY_LEFT:
            return WINDOWID_MAINPANEL;
            break;
        case INPUT_KEY_RIGHT:
            mIsShowDetails = 1;
            GuiMain::getInstance()->sendMessage(WINDOWID_TXSHOW, MSG_TXSHOW_MSG, 0, (LPARAM) mClientMessage);
            return WINDOWID_TXSHOW;
            break;
    }
    return 0;
}

int TxVerifyCodeWin::getIconState(int id) {
	switch (id) {
		case TXS_ICON_NAVI_CANCEL:
		case TXS_ICON_NAVI_DETAILS:
		case TXS_ICON_NAVI_OK:
			return 0;
	}
	return -1;
}

int TxVerifyCodeWin::onCreate(HWND hWnd) {
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



int TxVerifyCodeWin::onResume() {
    char tmpbuf[128];
    db_msg("resume");
    set_temp_screen_time(60);
    if (!mClientMessage) {
        db_error("invalid client msg");
        return -1;
    }
    if (mIsShowDetails) {
        db_msg("jump from tx show,skip");
        mIsShowDetails = 0;
        return 0;
    }
    freeWinData();
    initDView();
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
		dialog_error3(mHwnd, mShowRet, res_getLabel(LANG_LABEL_UNSUPPORT_MSG));
        freeWinData();
        PostMessage(GuiMain::getInstance()->getHwnd(), MSG_CHANGE_WINDOW, WINDOWID_MAINPANEL, 0);
        return -1;
	}

    //get mDView for saving coin and history, but not show mDView
    mShowRet = mTxp->onShow(mTxp->session, mDView);
    db_msg("TX show ret:%d", mShowRet);
    if (mShowRet < 0) {
        if (mShowRet == -181) {
            dialog_error(mHwnd, res_getLabel(LANG_LABEL_WALLET_NO_SUPPORT_TOKEN));
        } else {
            dialog_error3(mHwnd, mShowRet, res_getLabel(LANG_LABEL_UNSUPPORT_MSG));
        }
        freeWinData();
        PostMessage(GuiMain::getInstance()->getHwnd(), MSG_CHANGE_WINDOW, WINDOWID_MAINPANEL, 0);
        return -1;
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

    //show symbol
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

    //for saving history
    memcpy(&mDBTxCoinInfo, &mDView->db, sizeof(DBTxCoinInfo));

    //del mDView
    if (IS_VALID_HWND(mDView->hwnd)) {
        db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
        dwin_destory(mDView);
    }

    int n = TxGetVerifyCode(mClientMessage);
    if (n < 0) {
        db_error("TxGetVerifyCode error ret:%d", ret);
        dialog_error3(0, n, "Failed to generate verification code. Please try again.");
        return n;
    }
    uint8_t code[8];
    memzero(code, sizeof(code));
    for (int i = 0; i < 6; i++) {
        code[5 - i] = n % 10 + 0x30;
        n /= 10;
    }
    setLabelText(TXS_LABEL_VERIFY_CODE_TIPS, res_getLabel(LANG_LABEL_TX_VERIFY_CODE));
    setLabelText(TXS_LABEL_VERIFY_CODE, (const char *) code);

    showIcon(TXS_ICON_NAVI_OK, true);
    setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_TXS_CANCEL));

    showIcon(TXS_ICON_NAVI_DETAILS, true);
    setLabelText(TXS_LABEL_DETAIL, res_getLabel(LANG_LABEL_TX_SHOW_DETAILS), 1);

    WIN_RECT rect;
	res_getPos(MK_sign_navi_panel, &rect);
	int p = (mTotalHeight + mScreenHeight - 1) / mScreenHeight;
	rect.y += (p - 2) * mScreenHeight;
	MoveWindow(mHwndNaviPanel, rect.x, rect.y, rect.w, rect.h, FALSE);
	return 0;
}

int TxVerifyCodeWin::onPause() {
	set_temp_screen_time(0);
	if (!mIsShowDetails) {
		freeWinData();
        if (mClientMessage != NULL) {
            proto_client_message_delete(mClientMessage);
            mClientMessage = NULL;
        }
    }
	return 0;
}

int TxVerifyCodeWin::initDView() {
	if (!IS_VALID_HWND(mDView->hwnd)) {
		dwin_init(mDView, mHwnd, 10);
		mDView->msg_from = MSG_FROM_QR_APP;
	}
	return 0;
}

int TxVerifyCodeWin::freeWinData() {
	if (!mTxp->onShow) {
		return 0;
	}
	if (IS_VALID_HWND(mDView->hwnd)) {
		db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
		dwin_destory(mDView);
	}
    memset(&mDBTxCoinInfo, 0, sizeof(DBTxCoinInfo));
    if (mTxp->onEnd) {
		db_msg("call tx end");
		mTxp->onEnd(mTxp->session);
	}
	memset(mTxp, 0, sizeof(TxPorcessData));
	return 0;
}

int TxVerifyCodeWin::onSignReqData(ProtoClientMessage *req) {
	if (req) {
		db_msg("req:%p type:%d flag:%d", req, req->type, req->flag);
	} else {
		db_msg("empey req");
	}
	db_msg("mClientMessage:%p", mClientMessage);
	freeWinData();
	if (mClientMessage != NULL) {
		proto_client_message_delete(mClientMessage);
		mClientMessage = NULL;
	}
	mShowRet = -1;
	mIsShowDetails = 0;
	mClientMessage = req;
	if (mClientMessage) {
		initDView();
	}
	return 0;
}


int TxVerifyCodeWin::doSignReq() {
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
        if (mDBTxCoinInfo.coin_type > 0) {
            tx_save_history(mClientMessage, &mDBTxCoinInfo);
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
