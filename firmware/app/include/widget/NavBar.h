#ifndef TRUNK_NAVBAR_H
#define TRUNK_NAVBAR_H

#include "NavBar.h"
#include "minigui_comm.h"

enum {
	NAV_BAR_ITEM_SHOW_NONE = 0,
	NAV_BAR_ITEM_SHOW_BACK = 1 << 0,
	NAV_BAR_ITEM_SHOW_OK = 1 << 1,
	NAV_BAR_ITEM_SHOW_NEXT = 1 << 2,
	NAV_BAR_ITEM_SHOW_UP_DOWN = 1 << 3,
	NAV_BAR_ITEM_SHOW_UP = 1 << 4,
	NAV_BAR_ITEM_SHOW_DOWN = 1 << 5,
	NAV_BAR_ITEM_SHOW_CURRENT = 1 << 6,
	NAV_BAR_ITEM_SHOW_ALL = NAV_BAR_ITEM_SHOW_BACK | NAV_BAR_ITEM_SHOW_OK | NAV_BAR_ITEM_SHOW_NEXT
};

typedef enum {
	NAV_BAR_IDC_CONTAINER = 0,
	NAV_BAR_IDC_OK,
	NAV_BAR_IDC_BACK_IMG,
	NAV_BAR_IDC_BACK_TXT,
	NAV_BAR_IDC_NEXT_IMG,
	NAV_BAR_IDC_NEXT_TXT,
	NAV_BAR_IDC_CENTER_TXT,
	NAV_BAR_IDC_MAX
} NAV_BAR_IDC;

class NavBar {

public:
	NavBar(HWND parent);

	~NavBar();

	int init(const char *backTxt, const char *nextTxt, const char *centerTxt, int initState, int style);

	int initValue();

	int updateState(int state);

	int cleanMemory();

	int height();

	int width();

private:
	int updateWindowState(HWND hwnd, int show);

	HWND createWindow(HWND parent, WIN_RECT *rect, int type, int id);

	int initImgWindow(HWND parent, const char *posKey, BITMAP *bitmap, int id);

	BITMAP *newMallocBitmap();

	int loadBitMap(BITMAP *bitmap, const char *key);

	HWND mParent;
	HWND mContainer;
	HWND mHwnds[NAV_BAR_IDC_MAX];
	const char *mBackTxt;
	const char *mNextTxt;
	const char *mCurrentTxt;
	BITMAP *okmg;
	BITMAP *nextImg;
	BITMAP *preImg;
	int mState;
	int mStyle;
	int mWidth;
	int mHeight;
};

#endif
