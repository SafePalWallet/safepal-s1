#ifdef BUILD_FOR_HW_WALLET
#define LOG_TAG "loading"

#include <unistd.h>
#include "common_c.h"
#include "loading_win.h"

#include <minigui/mgconfig.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

int loading_win_processing = 0;

#define CACHE_LOADING_BITMAP

#define IS_VALID_HWND(x) ((x) && (x) != HWND_INVALID)
#define LOADING_IMG_BUMBER 13
static HWND mParent = HWND_INVALID;
static HWND mBGHwnd = HWND_INVALID;
static HWND mImgHwnd = HWND_INVALID;
static HWND mTitleHwnd = HWND_INVALID;
static HWND mDetailHwnd = HWND_INVALID;
static unsigned int imgno = 0;
static BITMAP mBmps[LOADING_IMG_BUMBER];

static int G_lock = 0;
static int bitmap_inited = 0;

static inline void
LOCK() {
	while (__sync_lock_test_and_set(&(G_lock), 1)) {}
}

static inline void
UNLOCK() {
	__sync_lock_release(&(G_lock));
}

static int init_bitmap() {
	int i;
	int ret;
	char path[64];
#ifdef CACHE_LOADING_BITMAP
	if (bitmap_inited) {
		return 0;
	}
#endif
	for (i = 0; i < LOADING_IMG_BUMBER; i++) {
		if (mBmps[i].bmBits) {
			UnloadBitmap(&mBmps[i]);
			mBmps[i].bmBits = NULL;
		}
		snprintf(path, sizeof(path), "%s/img/loading/%d.png", get_system_res_point(), i + 1);
		ret = LoadBitmapFromFile(HDC_SCREEN, &mBmps[i], path);
		db_msg("load:%s ret:%d", path, ret);
	}
	bitmap_inited = 1;
	return 0;
}

static int free_bitmap() {
#ifndef CACHE_LOADING_BITMAP
	int i;
	bitmap_inited = 0;
	for (i = 0; i < LOADING_IMG_BUMBER; i++) {
		if (mBmps[i].bmBits) {
			UnloadBitmap(&mBmps[i]);
			mBmps[i].bmBits = NULL;
		}
	}
#endif
	return 0;
}

int loading_win_start(HWND hwnd, const char *title, const char *detail, int style) {
	if (loading_win_processing) {
		db_msg("loading,skip...");
		return 0;
	}
	db_msg("start");
	LOCK();
	if (loading_win_processing) {
		UNLOCK();
		return 0;
	}
	if (init_bitmap() != 0) {
		db_error("init bitmap false");
		UNLOCK();
		return -1;
	}
	mParent = hwnd;
	if (style == 0 && !IS_VALID_HWND(mBGHwnd)) {
		mBGHwnd = CreateWindowEx("Common", "",
		                         WS_CHILD,
		                         WS_EX_USEPARENTFONT,
		                         100,
		                         0, 0, 240, 240,
		                         mParent, 0);
		if (!IS_VALID_HWND(mBGHwnd)) {
			UNLOCK();
			return -1;
		}
		db_msg("create mParent:%d mBGHwnd:%d", mParent, mBGHwnd);
		SetWindowBkColor(mBGHwnd, PIXEL_black);
		ShowWindow(mBGHwnd, SW_SHOWNORMAL);
	}

	if (!IS_VALID_HWND(mImgHwnd)) {
		mImgHwnd = CreateWindowEx(CTRL_STATIC, "",
		                          WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
		                          WS_EX_TRANSPARENT,
		                          101,
		                          90, 60, 60, 60,
		                          style == 1 ? mParent : mBGHwnd, 0);
		if (!IS_VALID_HWND(mImgHwnd)) {
			UNLOCK();
			return -1;
		}
		db_msg("create mParent:%d mImgHwnd:%d", mBGHwnd, mImgHwnd);
		ShowWindow(mImgHwnd, SW_SHOWNORMAL);
	}

	if (!IS_VALID_HWND(mTitleHwnd) && title) {
		mTitleHwnd = CreateWindowEx(CTRL_STATIC, "",
		                            WS_CHILD | WS_VISIBLE | SS_CENTER,
		                            WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
		                            102,
		                            0, 130, 240, 34,
		                            mBGHwnd, 0);
		if (!IS_VALID_HWND(mTitleHwnd)) {
			UNLOCK();
			return -1;
		}
		SetWindowElementAttr(mTitleHwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, PIXEL_lightwhite));
		SetWindowFont(mTitleHwnd, (PLOGFONT) Global_Font20);
		db_msg("create mParent:%d mTitleHwnd:%d", mBGHwnd, mTitleHwnd);
		ShowWindow(mTitleHwnd, SW_SHOWNORMAL);
	}

	if (!IS_VALID_HWND(mDetailHwnd) && detail) {
		mDetailHwnd = CreateWindowEx(CTRL_STATIC, "",
		                             WS_CHILD | WS_VISIBLE | SS_CENTER,
		                             WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
		                             103,
		                             0, 170, 240, 60,
		                             mBGHwnd, 0);
		if (!IS_VALID_HWND(mDetailHwnd)) {
			UNLOCK();
			return -1;
		}
		SetWindowElementAttr(mDetailHwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, PIXEL_lightwhite));
		SetWindowFont(mDetailHwnd, (PLOGFONT) Global_Font18);
		db_msg("create mParent:%d mDetailHwnd:%d", mBGHwnd, mDetailHwnd);
		ShowWindow(mDetailHwnd, SW_SHOWNORMAL);
	}

	loading_win_processing = 1;


	if (IS_VALID_HWND(mTitleHwnd)) {
		ShowWindow(mTitleHwnd, title ? SW_SHOW : SW_HIDE);
		if (title) {
			SetWindowText(mTitleHwnd, title);
		}
	}
	if (IS_VALID_HWND(mDetailHwnd)) {
		ShowWindow(mTitleHwnd, detail ? SW_SHOW : SW_HIDE);
		if (detail) {
			SetWindowText(mDetailHwnd, detail);
		}
	}

	UNLOCK();
	imgno = LOADING_IMG_BUMBER;
	loading_win_refresh();
	return 0;
}

void loading_win_stop() {
	db_msg("stop");
	LOCK();
	loading_win_processing = 0;
	mParent = HWND_INVALID;
	if (IS_VALID_HWND(mBGHwnd)) {
		DestroyWindow(mBGHwnd);
		mBGHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(mImgHwnd)) {
		DestroyWindow(mImgHwnd);
		mImgHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(mTitleHwnd)) {
		DestroyWindow(mTitleHwnd);
		mTitleHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(mDetailHwnd)) {
		DestroyWindow(mDetailHwnd);
		mDetailHwnd = HWND_INVALID;
	}
	free_bitmap();
	UNLOCK();
}

int loading_win_refresh() {
	if (loading_win_processing && IS_VALID_HWND(mImgHwnd)) {
		imgno++;
		if (imgno >= LOADING_IMG_BUMBER) {
			imgno = 0;
		}
		SendMessage(mImgHwnd, STM_SETIMAGE, (WPARAM) &mBmps[imgno], 0);
		InvalidateRect(mImgHwnd, NULL, TRUE);
		MSG msg;
		while (HavePendingMessage(Global_MainHwnd)) {
			if (PeekMessage(&msg, Global_MainHwnd, 0, 0, PM_REMOVE)) {
				DispatchMessage(&msg);
			}
		}
	}
	return 0;
}

int loading_win_refresh_timeout(int time_ms) {
	while (time_ms > 0) {
		loading_win_refresh();
		usleep((time_ms > 100 ? 100 : time_ms) * 1000);
		time_ms -= 100;
	}
	return 0;
}

#endif