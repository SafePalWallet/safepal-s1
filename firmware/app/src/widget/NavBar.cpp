#define LOG_TAG "NavBar"

#include "NavBar.h"
#include "minigui_comm.h"
#include "debug.h"
#include "resource.h"

typedef enum {
	NAV_BAR_WINDOW_TYPE_NONE,
	NAV_BAR_WINDOW_TYPE_LABEL,
	NAV_BAR_WINDOW_TYPE_IMAGE
} NAV_BAR_WINDOW_TYPE;


NavBar::NavBar(HWND parent) {
	mParent = parent;
	initValue();
}

int NavBar::init(const char *backTxt, const char *nextTxt, const char *currentTxt, int initState, int style) {
	char str[24] = {0};
	mState = initState;
	mStyle = style;
	mBackTxt = backTxt;
	mNextTxt = nextTxt;
	mCurrentTxt = currentTxt;
	WIN_RECT _rect;
	WIN_RECT *rect = &_rect;
	HWND hwnd = HWND_INVALID;
	int font = 0;
	const char *constrf = NULL;
	label_set_param label_set;
	res_getRect(MK_nav_bar, "pos", &_rect);
	db_msg("nav bar x:%d y:%d w:%d h:%d style:%d", rect->x, rect->y, rect->w, rect->h, style);
	mHeight = _rect.h;
	mWidth = _rect.w;
	mContainer = createWindow(mParent, rect, NAV_BAR_WINDOW_TYPE_NONE, NAV_BAR_IDC_CONTAINER);
	SetWindowBkColor(mContainer, res_getBGColor1(mStyle));

	snprintf(str, sizeof(str), "back_img%d", mStyle);
	loadBitMap(preImg, str);
	snprintf(str, sizeof(str), "next_img%d", mStyle);
	loadBitMap(nextImg, str);

	if (mState & NAV_BAR_ITEM_SHOW_UP_DOWN) {
		snprintf(str, sizeof(str), "up_down_img%d", mStyle);
		loadBitMap(okmg, str);
	} else if (mState & NAV_BAR_ITEM_SHOW_OK) {
		loadBitMap(okmg, "ok_img");
	}

	initImgWindow(mContainer, "ok_img_pos", okmg, NAV_BAR_IDC_OK);
	initImgWindow(mContainer, "back_pos", preImg, NAV_BAR_IDC_BACK_IMG);
	initImgWindow(mContainer, "next_pos", nextImg, NAV_BAR_IDC_NEXT_IMG);

	if (mBackTxt) {
		db_msg("back txt:%s", mBackTxt);
		res_getLabelSetParam(&label_set, MK_nav_bar, "back_txt_config");
		hwnd = createWidgetWindow(mContainer, 0, label_set.x, label_set.y, label_set.w, label_set.h, NAV_BAR_IDC_BACK_TXT, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
		if (IS_VALID_HWND(hwnd)) {
			mHwnds[NAV_BAR_IDC_BACK_TXT] = hwnd;
			SetWindowMText(hwnd, mBackTxt);
			setLabelTextColor(hwnd, res_getTextColor(mStyle));
		}
	}
	if (mNextTxt) {
		db_msg("next txt:%s", mNextTxt);
		res_getLabelSetParam(&label_set, MK_nav_bar, "next_txt_config");
        if (!strncmp(mNextTxt, "{11}", strlen("{11}"))) {
            label_set.y += 3;
        }
		hwnd = createWidgetWindow(mContainer, 0, label_set.x, label_set.y, label_set.w, label_set.h, NAV_BAR_IDC_BACK_TXT, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
		if (IS_VALID_HWND(hwnd)) {
			mHwnds[NAV_BAR_IDC_NEXT_TXT] = hwnd;
			SetWindowMText(hwnd, mNextTxt);
			setLabelTextColor(hwnd, res_getTextColor(mStyle));
		}
	}
    if (mCurrentTxt) {
        db_msg("center txt:%s", mCurrentTxt);
        res_getLabelSetParam(&label_set, MK_nav_bar, "current_txt_config");
        hwnd = createWidgetWindow(mContainer, 0, label_set.x, label_set.y, label_set.w, label_set.h, NAV_BAR_IDC_CENTER_TXT, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
        if (IS_VALID_HWND(hwnd)) {
            mHwnds[NAV_BAR_IDC_CENTER_TXT] = hwnd;
            SetWindowMText(hwnd, mCurrentTxt);
            setLabelTextColor(hwnd, res_getTextColor(mStyle));
        }
    }

    updateState(mState);

	return 0;
}

int NavBar::initValue() {
	mContainer = HWND_INVALID;
	mState = NAV_BAR_ITEM_SHOW_NONE;
	mBackTxt = NULL;
	mNextTxt = NULL;
    mCurrentTxt = NULL;
	for (int i = 0; i < NAV_BAR_IDC_MAX; ++i) {
		mHwnds[i] = HWND_INVALID;
	}
	okmg = newMallocBitmap();
	nextImg = newMallocBitmap();
	preImg = newMallocBitmap();

	return 0;
}

int NavBar::height() {
	return mHeight;
}

int NavBar::width() {
	return mWidth;
}

int NavBar::updateWindowState(HWND hwnd, int show) {
	if (!IS_VALID_HWND(hwnd)) {
		return -1;
	}
	db_msg("hwnd:%d show:%d", hwnd, show);
	if (show) {
		ShowWindow(hwnd, SW_SHOW);
	} else {
		ShowWindow(hwnd, SW_HIDE);
	}
	return 0;
}

HWND NavBar::createWindow(HWND parent, WIN_RECT *rect, int type, int id) {
	HWND hwnd = HWND_INVALID;
	DWORD option = 0;
	DWORD exStyle = 0;

	if (type == NAV_BAR_WINDOW_TYPE_NONE) {
		option = WS_CHILD | WS_VISIBLE;
		exStyle = WS_EX_USEPARENTFONT;
	} else if (type == NAV_BAR_WINDOW_TYPE_IMAGE) {
		option = WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE;
		exStyle = WS_EX_TRANSPARENT;
	} else if (type == NAV_BAR_WINDOW_TYPE_LABEL) {
		option = WS_CHILD | WS_VISIBLE;
		exStyle = WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT;
	}
	hwnd = CreateWindowEx(CTRL_STATIC, "",
	                      option,
	                      exStyle,
	                      id,
	                      rect->x, rect->y, rect->w, rect->h,
	                      parent, 0);
	db_msg("parent:%d hwnd:%d type:%d id:%d", parent, hwnd, type, id);
	return hwnd;
}

int NavBar::initImgWindow(HWND parent, const char *posKey, BITMAP *bitmap, int id) {
	WIN_RECT _rect;
	WIN_RECT *rect = &_rect;
	HWND hwnd = HWND_INVALID;
	res_getRect(MK_nav_bar, posKey, &_rect);
	hwnd = createWindow(mContainer, rect, NAV_BAR_WINDOW_TYPE_IMAGE, id);
	if (IS_VALID_HWND(hwnd)) {
		db_msg("new hwnd:%d id:%d", hwnd, id);
		mHwnds[id] = hwnd;
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) bitmap, 0);
		InvalidateRect(hwnd, NULL, TRUE);
		ShowWindow(hwnd, SW_SHOWNORMAL);
		return 0;
	}

	db_error("invalid hwnd:%d", hwnd);
	return -1;
}

BITMAP *NavBar::newMallocBitmap() {
	BITMAP *bitmap = (BITMAP *) malloc(sizeof(BITMAP));
	if (!bitmap) {
		db_error("new icon_img memory false");
	}
	memset(bitmap, 0, sizeof(BITMAP));
	return bitmap;
}

int NavBar::loadBitMap(BITMAP *bitmap, const char *key) {
	const char *file = NULL;
	if (NULL == key || NULL == bitmap) {
		db_error("load bitmap failed, invalid paras bitmap:%p, key:%s", bitmap, key);
		return -1;
	}
	file = res_getString2(MK_nav_bar, key);
	if (file) {
		int ret = LoadBitmapFromFile(HDC_SCREEN, bitmap, file);
		if (ret != ERR_BMP_OK) {
			db_error("load bitmap path:%s failed", file);
		} else {
			db_msg("load bitmap path:%s OK", file);
		}
	}
	return 0;
}

NavBar::~NavBar() {
	db_msg("release NavBar");
	cleanMemory();
}

int NavBar::cleanMemory() {
	db_msg("clear memory");
	const int bitmapCnt = 3;
	BITMAP *bitmaps[bitmapCnt] = {okmg, nextImg, preImg};
	for (int j = 0; j < bitmapCnt; ++j) {
		if (bitmaps[j]) {
			if (bitmaps[j]->bmBits) {
				UnloadBitmap(bitmaps[j]);
			}
			free(bitmaps[j]);
		}
	}
	if (IS_VALID_HWND(mContainer)) {
		DestroyWindow(mContainer);
	}
	for (int i = 0; i < NAV_BAR_IDC_MAX; ++i) {
		if (IS_VALID_HWND(mHwnds[i])) {
			DestroyWindow(mHwnds[i]);
		}
	}

	return 0;
}

int NavBar::updateState(int state) {
	mState = state;
	char str[64] = {0};

	if (okmg->bmBits) {
		UnloadBitmap(okmg);
	}
	if (mState & NAV_BAR_ITEM_SHOW_UP_DOWN) {
		snprintf(str, sizeof(str), "up_down_img%d", mStyle);
	} else if (mState & NAV_BAR_ITEM_SHOW_OK) {
		snprintf(str, sizeof(str), "ok_img");
	} else if (mState & NAV_BAR_ITEM_SHOW_UP) {
		snprintf(str, sizeof(str), "up_img%d", mStyle);
	} else if (mState & NAV_BAR_ITEM_SHOW_DOWN) {
		snprintf(str, sizeof(str), "down_img%d", mStyle);
	}
	loadBitMap(okmg, str);
	SendMessage(mHwnds[NAV_BAR_IDC_OK], STM_SETIMAGE, (WPARAM) okmg, 0);
	InvalidateRect(mHwnds[NAV_BAR_IDC_OK], NULL, TRUE);

	if ((mState & NAV_BAR_ITEM_SHOW_UP_DOWN) || (mState & NAV_BAR_ITEM_SHOW_OK) ||
	    (mState & NAV_BAR_ITEM_SHOW_UP) || (mState & NAV_BAR_ITEM_SHOW_DOWN)) {
		updateWindowState(mHwnds[NAV_BAR_IDC_OK], 1);
	} else {
		updateWindowState(mHwnds[NAV_BAR_IDC_OK], 0);
	}
	updateWindowState(mHwnds[NAV_BAR_IDC_BACK_TXT], mState & NAV_BAR_ITEM_SHOW_BACK);
	updateWindowState(mHwnds[NAV_BAR_IDC_BACK_IMG], mState & NAV_BAR_ITEM_SHOW_BACK);

	updateWindowState(mHwnds[NAV_BAR_IDC_NEXT_TXT], mState & NAV_BAR_ITEM_SHOW_NEXT);
	updateWindowState(mHwnds[NAV_BAR_IDC_NEXT_IMG], mState & NAV_BAR_ITEM_SHOW_NEXT);

    updateWindowState(mHwnds[NAV_BAR_IDC_CENTER_TXT], mState & NAV_BAR_ITEM_SHOW_CURRENT);

	return 0;
}


