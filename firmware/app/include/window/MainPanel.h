
#ifndef __MAINPANEL_H__
#define __MAINPANEL_H__

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "minigui_comm.h"

#include "resource.h"
#include "PicObj.h"
#include "CommonWindow.h"

class GuiMain;

class MainPanel : public CommonWindow {
public:
	MainPanel();

	~MainPanel();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

private:
	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int onChangeFont();

	int entranceChanged(int action);

	int onEntranceClicked();

	int getIconState(int id);

	void updateWalletName();

	void flushWinWidget();

	void updateBattery(int cap = 0);

	int mNaviIndex;
	int mResume;
	int mBatteryChanged;
	int mBatteryCharging;
	int mLastDeviceAccountIndex;
};

#endif


