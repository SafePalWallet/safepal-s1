#ifndef TRUNK_BUTTON_H
#define TRUNK_BUTTON_H

#include "minigui_comm.h"

enum {
	BUTTON_STATE_NORMAL = 0,
	BUTTON_STATE_SELECTED,
	BUTTON_STATE_HIGHLIGHT,
	BUTTON_STATE_DISABLE,
	BUTTON_STATE_MAX
};

class Button {

public:
	Button(HWND parent, int id);

	~Button();

	int init(WIN_RECT rect, const char *title, int state);

	int setState(int state);

	int getState(int state);

private:
	int initValues();

	int update();

	int bgWindowPro(HWND hwnd, int msg, WPARAM wparam, LPARAM lparam);

	HWND mParent;
	WIN_RECT mRect;
	int mId;
	int mState;
	const char *mTitle;
	HWND mContainer;
	HWND mBgHwnd;
	HWND mTitleHwnd;
	BITMAP *normalBgImg;
	BITMAP *highlightBgImg;
	BITMAP *disableBbImg;
};

#endif
