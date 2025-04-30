#include "Button.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "minigui_comm.h"
#include "widgets.h"
#include "resource.h"
#include <minigui/gdi.h>

#define IDC_TITLE 650
#define IDC_BG    651

Button::Button(HWND parent, int id) {
	initValues();
	mParent = parent;
	mId = id;
}

Button::~Button() {
	db_msg("clear button");
	if (IS_VALID_HWND(mTitleHwnd)) {
		DestroyWindow(mTitleHwnd);
		mTitleHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(mBgHwnd)) {
		DestroyWindow(mBgHwnd);
		mBgHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(mContainer)) {
		DestroyWindow(mContainer);
		mContainer = HWND_INVALID;
	}
	initValues();
}

int Button::initValues() {
	mParent = HWND_INVALID;
	mContainer = HWND_INVALID;
	mBgHwnd = HWND_INVALID;
	memset(&mRect, 0, sizeof(WIN_RECT));
	mTitle = NULL;
	mState = BUTTON_STATE_NORMAL;
	mId = 0;
	return 0;
}

int Button::bgWindowPro(HWND hwnd, int msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case MSG_PAINT:
			db_msg("MSG_PAINT");
			break;
		case MSG_CREATE:
			db_msg("MSG_CREATE");
			break;
		case MSG_DESTROY:
			db_msg("MSG_DESTROY");
			break;
		default:
			break;
	}
	return DefaultWindowProc(hwnd, msg, wparam, lparam);
}

int Button::init(WIN_RECT rect, const char *title, int state) {
	int x = rect.x;
	int y = rect.y;
	int w = rect.w;
	int h = rect.h;
	mRect = rect;
	mTitle = title;
	mState = state;
	mContainer = createWidgetWindow(mParent, 0, x, y, w, h, mId, WIDGET_TYPE_NORMAL, 0, -1);
	SetWindowBkColor(mContainer, res_getBGColor());
	mBgHwnd = createWidgetWindow(mContainer, 0, 0, 0, w, h, IDC_BG, WIDGET_TYPE_IMG, 0, -1);
	mTitleHwnd = createWidgetWindow(mContainer, 0, 0, 0, w, h, IDC_TITLE, WIDGET_TYPE_TEXT, 4, 0);
	setLabelTextColor(mTitleHwnd, res_getTextColor(0));
	SetWindowMText(mTitleHwnd, title);
	update();
	return 0;
}

int Button::update() {
	static BITMAP *selBgBitmap = NULL;
	static BITMAP *norBgBitmap = NULL;
	const char *constr = NULL;
	RECT _rect;
	PRECT rect = &_rect;
	memset(rect, 0, sizeof(RECT));
	GetClientRect(mBgHwnd, rect);
	HDC hdc = HDC_INVALID;

#if 0
	int sx = 0;
	int sy = 0;
	hdc = BeginPaint(mBgHwnd);
	db_msg("mBgHwnd:%d hdc:%d", mBgHwnd, hdc);
	sx = mRect.h / 2;
	sy = sx;
	SetBrushColor(hdc, PIXEL_blue);
	db_msg("Fill left circle x:%d y:%d r:%d", sx, sy, mRect.h / 2);
	FillCircle(hdc, sx, sy, mRect.h / 2);
	FillBox(hdc, sx, mRect.y, mRect.w - mRect.h, mRect.h);
	db_msg("Fille box x:%d y:%d w:%d h:%d", sx, mRect.y, mRect.w - mRect.h, mRect.h);
	sx = mRect.w - mRect.h / 2;
	sy = mRect.h / 2;
	FillCircle(hdc, sx, sy, mRect.h / 2);
	db_msg("Fille right circle x:%d y:%d r:%d", sx, sy, mRect.h / 2);
	EndPaint(mBgHwnd, hdc);
#endif
	if (!selBgBitmap) {
		selBgBitmap = (BITMAP *) malloc(sizeof(BITMAP));
		memset(selBgBitmap, 0, sizeof(BITMAP));
		constr = res_getString2(MK_system_icon, "btn_long_sel_bg_img");
		res_loadBmp(constr, selBgBitmap);
	}
	if (!norBgBitmap) {
		norBgBitmap = (BITMAP *) malloc(sizeof(BITMAP));
		memset(norBgBitmap, 0, sizeof(BITMAP));
		constr = res_getString2(MK_system_icon, "btn_long_bg_img");
		res_loadBmp(constr, norBgBitmap);
	}
	GetClientRect(mBgHwnd, rect);
	switch (mState) {
		case BUTTON_STATE_NORMAL:
		case BUTTON_STATE_DISABLE: {
			SendMessage(mBgHwnd, STM_SETIMAGE, (DWORD) norBgBitmap, 0);
		}
			break;
		case BUTTON_STATE_HIGHLIGHT:
		case BUTTON_STATE_SELECTED: {
			SendMessage(mBgHwnd, STM_SETIMAGE, (DWORD) selBgBitmap, 0);
		}
			break;
		default:
			break;
	}
	InvalidateRect(mBgHwnd, rect, TRUE);
	return 0;
}

int Button::setState(int state) {
	if (mState == state) {
		return 0;
	}
	mState = state;
	update();
	return 0;
}

int Button::getState(int state) {
	return mState;
}