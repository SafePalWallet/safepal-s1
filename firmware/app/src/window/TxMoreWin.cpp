#define LOG_TAG "TxMore"

#include <stdlib.h>
#include <CommonWindow.h>
#include <widgets.h>
#include <minigui_comm.h>
#include "common.h"
#include "TxMoreWin.h"
#include "Dialog.h"

enum {
	TXS_ICON_NAVI_UP_DOWN,
	TXS_ICON_NAVI_CANCEL,
	TXS_ICON_MAXID,
};

enum {
	TXS_LABEL_CANCEL,
	TXS_LABEL_MAXID,
};

TxMoreWin::TxMoreWin() {
	int icon_mk_map[TXS_ICON_MAXID] = {
			MK_sign_icon_navi_up_down,
			MK_sign_icon_navi_cancel,
	};

	int label_mk_map[TXS_LABEL_MAXID] = {
			MK_sign_label_cancel,
	};

	mTxp = NULL;
	memset(mDView, 0, sizeof(DynamicViewCtx));

	mHwndNaviPanel = HWND_INVALID;
	initLayout(TXS_ICON_MAXID, icon_mk_map, TXS_LABEL_MAXID, label_mk_map);
}

TxMoreWin::~TxMoreWin() {

}

PROC_RET TxMoreWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_TXMORE_TXP_MSG: {
			onTxPorcessData((TxPorcessData *) lParam);
		}
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int TxMoreWin::keyProc(int keyCode, int isLongPress) {
	switch (keyCode) {
		case INPUT_KEY_OK:
			break;
		case INPUT_KEY_UP:
			scrollWindow(-1);
			break;
		case INPUT_KEY_DOWN:
			scrollWindow(1);
			break;
		case INPUT_KEY_LEFT:
			return WINDOWID_TXSHOW;
		case INPUT_KEY_RIGHT:
			break;
	}
	return 0;
}

int TxMoreWin::getIconState(int id) {
	switch (id) {
		case TXS_ICON_NAVI_CANCEL:
			return 0;
	}
	return -1;
}

static void get_navi_panel_rect(WIN_RECT *rect) {
	res_getPos(MK_sign_navi_panel, rect);
	if (rect->y > SCREEN_HEIGHT) {
		rect->y -= SCREEN_HEIGHT;
	}
}

int TxMoreWin::onCreate(HWND hWnd) {
	int ret;
	WIN_RECT rect;
	get_navi_panel_rect(&rect);

	mHwndNaviPanel = CreateWindowEx(CTRL_STATIC, "",
	                                WS_CHILD | WS_VISIBLE,
	                                WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
	                                1,
	                                rect.x, rect.y, rect.w, rect.h,
	                                hWnd, (DWORD)
			                                this);

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

int TxMoreWin::onResume() {
	int showret;
	if (!mTxp || !mTxp->onShow) {
		db_error("invalid txp");
		changeWindow(WINDOWID_MAINPANEL);
		return 0;
	}
	set_temp_screen_time(60);
	setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_BACK));

	initDView();
	if (mScrollSize) {
		resetScrollSize();
	}
	showret = mTxp->onShow(mTxp->session, mDView);
	db_msg("TX show ret:%d total_height:%d", showret, mDView->total_height);
	if (showret < 0) {
		dialog_error3(mHwnd, showret, "Show tx info failed.");
		return -1;
	}

	mTotalHeight = mDView->total_height > mScreenHeight ? mDView->total_height : mScreenHeight;

	if (mTotalHeight > mScreenHeight) {
		updateIcon(TXS_ICON_NAVI_UP_DOWN, 1, true);
	} else {
		updateIcon(TXS_ICON_NAVI_UP_DOWN, 0, false);
	}

	WIN_RECT rect;
	get_navi_panel_rect(&rect);

	int p = (mTotalHeight + mScreenHeight - 1) / mScreenHeight;
	db_msg("rect:%d total_height:%d p:%d", rect.y, mTotalHeight, p);
	rect.y += (p - 1) * mScreenHeight;
	MoveWindow(mHwndNaviPanel, rect.x, rect.y, rect.w, rect.h, FALSE);
	return 0;
}

int TxMoreWin::onPause() {
	set_temp_screen_time(0);
	freeWinData();
	return 0;
}

int TxMoreWin::initDView() {
	if (!IS_VALID_HWND(mDView->hwnd)) {
		dwin_init(mDView, mHwnd, 2);
		mDView->show_more = 1;
	}
	return 0;
}

int TxMoreWin::freeWinData() {
	if (IS_VALID_HWND(mDView->hwnd)) {
		db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
		dwin_destory(mDView);
	}
	return 0;
}

void TxMoreWin::onTxPorcessData(TxPorcessData *txp) {
	db_msg("txp:%p", txp);
	freeWinData();
	mTxp = txp;
	if (mTxp) {
		initDView();
	}
}

int TxMoreWin::onScrollWindow(int scroll_size) {
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
