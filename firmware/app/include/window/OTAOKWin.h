#ifndef OTA_OK_WIN_H
#define OTA_OK_WIN_H

#include "CommonWindow.h"

class OTAOKWin : public CommonWindow {
public:
	OTAOKWin();

	~OTAOKWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();
};

#endif
