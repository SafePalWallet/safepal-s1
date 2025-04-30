#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include "minigui_comm.h"

#include "key_event.h"
#include "AppMain.h"
#include "GuiInterface.h"
#include "CommonWindow.h"
#include "ScanQrCode.h"

/* MainWindow */
#define ID_TIMER_KEY 100

class GuiMain : public GuiCallBack {
public:
	static GuiMain *getInstance();

	GuiMain();

	HWND getHwnd() {
		return mHwnd;
	}

	CommonWindow *getWindowObject(int windowID, int create = 0);

	int createWindow(int windowID);

	int destoryWindow(int windowID);

	int sendMessage(int windowID, int iMsg, WPARAM wParam, LPARAM lParam);

	~GuiMain();

	int onInitAppMain();

	int onLosePower();

	int onUiEvent(int event, int param, long param2);

	int createMainWindows();

	void initStage2();

	void msgLoop();

	int hookKeyMsg(HWND dst_wnd, int msg, WPARAM wparam, LPARAM lparam);

	PROC_RET mainWindowProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	bool isShutdownIng();

	int getCurWindowID();

	void changeWindow(int windowID);

	void xchangeWindow(int windowID);

	void showShutdownLogo();

	void checkShutdown();

	int onQrResult(int type, qr_packet *packet);

private:
	void updateWindowFont();

	int keyProc(int keyCode, int isLongPress);

	void DispatchKeyEvent(int winid, int keyCode, int isLongPress);

	HWND mHwnd;
	CommonWindow *mWindows[WINDOWID_MAXID];
	bool mFinishConfig;
	bool mKeyShutDown;

	int mCurWindowID;

	int mHookKey;
	HWND mHookHwnd;
};

#endif
