#ifndef __GUI_COMMON_WINDOW_H__
#define __GUI_COMMON_WINDOW_H__

#include "common.h"
#include "minigui_comm.h"
#include "resource.h"
#include "widgets.h"
#include "PicObj.h"
#include "win.h"
#include "passwd_util.h"

#define WINDOW_COMMON        "Common"
#define WINDOW_SCANQR        "ScanQr"

extern int RegisterWltWindows();

extern void UnRegisterWltWindows();

PROC_RET CommonWindowProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

class CommonWindow {
public:
	CommonWindow();

	virtual ~CommonWindow();

	HWND getHwnd() {
		return mHwnd;
	}

	PROC_RET commonWinProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	virtual int createMainWindow(int winid, HWND hParentWnd);

	int isHold();

protected:
	HWND mHwnd;
	int mWindowId;

	int mIconSize;
	int mLabelSize;
	PicObj **mIcons;
	HWND *mLabels;
	layout_icon_param *mIconParams;
	layout_label_param *mLabelParams;

	int mScreenHeight;
	int mScreenWidth;
	int mTotalHeight;
	int mScrollSize;

	virtual PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
		return DefaultWindowProc(hWnd, message, wParam, lParam);
	}

	virtual int keyProc(int keyCode, int isLongPress) = 0;

	virtual int onCreate(HWND hWnd) {
		return createLayoutWidgets(hWnd);
	}

	virtual int onDestory() {
		return destoryLayoutWidgets();
	}

	virtual int onResume() {
		return 0;
	}

	virtual int onPause() {
		return 0;
	}

	virtual int getIconState(int id) { return -1; }

	virtual int onUpdateIcon(int id, int state) { return 0; }

	virtual int onScrollWindow(int scroll_size) { return 0; }

	int initLayout(int icon_size, const int *icon_mk_map, int label_size, const int *label_mk_map);

	int createLayoutWidgets(HWND hParent, const HWND *subHwnds = NULL);

	int destoryLayoutWidgets();

	HWND getIconHwnd(int id);

	HWND getLabelHwnd(int id);

	int updateIcon(int id, bool show = false);

	int updateIcon(int id, int state, bool show = false);

	int showIcon(int id, bool on);

	int setLabelText(int id, const char *text, int show = -1);

	void flushLayoutWidget(int state);

	int scrollWindow(int direction); // -1 up 1 down
	int resetScrollSize();

	int changeWindow(int windowID);
};

#endif
