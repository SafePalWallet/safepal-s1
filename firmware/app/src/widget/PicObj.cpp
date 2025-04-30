#define LOG_TAG "PicObj"

#include "PicObj.h"

#include "debug.h"

#define DEBUG_PICOBJ

PicObj::PicObj(HWND parent, int id, int cfgkey) {
	mParent = parent;
	mId = id;
	mCfgKey = cfgkey;
	memset(&mBmp, 0, sizeof(BITMAP));

	mState = -1;
	mHwnd = HWND_INVALID;
	mShowing = false;
	mImgValid = false;
	mHideInValid = true;
	createHwnd();
}

PicObj::~PicObj() {
	db_msg("del id:%d ckey:0x%x", mId, mCfgKey);
	if (IS_VALID_HWND(mHwnd)) {
		DestroyWindow(mHwnd);
	}
	res_unloadBmp(&mBmp);
}

int PicObj::createHwnd() {
	int retval;
	WIN_RECT rect;
	retval = res_getPos(mCfgKey, &rect);
	if (retval < 0) {
		db_error("get id:%d  key:0x%x failed", mId, mCfgKey);
		return -1;
	}
	mHwnd = CreateWindowEx(CTRL_STATIC, "",
	                       WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
	                       WS_EX_TRANSPARENT,
	                       mId,
	                       rect.x, rect.y, rect.w, rect.h,
	                       mParent, 0);
	if (!IS_VALID_HWND(mHwnd)) {
		db_error("create picobj failed id:%d x:%d y:%d w:%d h:%d", mId, rect.x, rect.y, rect.w, rect.h);
		return -1;
	}
	return 0;
}

int PicObj::update(int state, bool autoShow, BITMAP *pbitmap) {
	db_msg("update id:%d ckey:0x%x ashow:%d HideInValid:%d state:%d -> %d", mId, mCfgKey, autoShow, mHideInValid, mState, state);
	if (state == mState && !pbitmap) {
		if (autoShow) show();
		return 0;
	}
	mState = state;
	int retval;

	res_unloadBmp(&mBmp);
	if (pbitmap == NULL) {
		retval = res_getBmpByState(mCfgKey, SK_NONE, mState, &mBmp);
		pbitmap = &mBmp;
	} else {
		retval = 0;
	}

	if (retval < 0) {
		mImgValid = false;
		hide();
		mHideInValid = true;
		db_msg("get pic id:%d key:0x%x state:%d failed", mId, mCfgKey, mState);
		return -1;
	}
	mImgValid = true;

	if (!IS_VALID_HWND(mHwnd)) {
		if (createHwnd() != 0) {
			return -1;
		}
	}

	SendMessage(mHwnd, STM_SETIMAGE, (WPARAM) pbitmap, 0);
	InvalidateRect(mHwnd, NULL, TRUE);
	if (autoShow || mHideInValid) {
		show();
	}
	return 0;
}

int PicObj::show() {
#ifdef DEBUG_PICOBJ
	db_msg("id:%d ckey:0x%x state:%d", mId, mCfgKey, mState);
#endif
	if (!IS_VALID_HWND(mHwnd) || !mImgValid) {
		mHideInValid = true;
		return -1;
	}
	mHideInValid = false;
	if (!mShowing) {
		mShowing = true;
		ShowWindow(mHwnd, SW_SHOWNORMAL);
	}
	return 0;
}

int PicObj::hideInValid(int state) {
#ifdef DEBUG_PICOBJ
	db_msg("id:%d ckey:0x%x state:%d ->%d", mId, mCfgKey, mState, state);
#endif
	mState = state;
	mHideInValid = true;
	if (!IS_VALID_HWND(mHwnd)) {
		return -1;
	}
	if (mShowing) {
		mShowing = false;
		ShowWindow(mHwnd, SW_HIDE);
	}

	return 0;
}

int PicObj::hide() {
#ifdef DEBUG_PICOBJ
	db_msg("id:%d ckey:0x%x state:%d", mId, mCfgKey, mState);
#endif
	mHideInValid = false;
	if (!IS_VALID_HWND(mHwnd)) {
		return -1;
	}
	if (mShowing) {
		mShowing = false;
		ShowWindow(mHwnd, SW_HIDE);
	}
	return 0;
}

int PicObj::onClick(bool action, int state, int show) {
	int realstate = state == -1 ? DROP_CSTATE_SP(mState) : DROP_CSTATE_SP(state);
	if (action) {
		return update(SET_CSTATE_SP(realstate, CSTATE_SP_CLICK), show != 0);
	} else {
		return update(realstate, show == 1);
	}
}

int PicObj::onActive(bool action, int state, int show) {
	int realstate = state == -1 ? DROP_CSTATE_SP(mState) : DROP_CSTATE_SP(state);
	if (action) {
		return update(SET_CSTATE_SP(realstate, CSTATE_SP_ACTIVE), show != 0);
	} else {
		return update(realstate, show == 1);
	}
}
