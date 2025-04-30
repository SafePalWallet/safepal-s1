#ifndef TRUNK_ABOUTWIN_H
#define TRUNK_ABOUTWIN_H

#include "CommonWindow.h"
#include "ListView.h"
#include "storage_manager.h"
#include "BitmapGroup.h"

class AboutWin : public CommonWindow {
public:
	AboutWin();

	~AboutWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

private:
	int onClickItem();

	HWND mContainer;
	ListView *mListView;
};

#endif
