#define LOG_TAG "RegisterWin"

#include "minigui_comm.h"

#include "CommonWindow.h"
#include "debug.h"

typedef PROC_RET (*Proc)(HWND, PROC_MSG_TYPE, WPARAM, LPARAM);

#define DEFINE_PROC(f) extern PROC_RET f(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam)

DEFINE_PROC(CommonWindowProc);

DEFINE_PROC(ScanQrCodeProc);

static const char *windowClass[] = {
		WINDOW_COMMON,
		WINDOW_SCANQR,
};

static Proc windowProc[] = {
		CommonWindowProc,
		ScanQrCodeProc,
};

int RegisterWltWindows() {
	unsigned int count;
	for (count = 0; count < ARRAY_SIZE(windowClass); count++) {
		WNDCLASS WndClass;
		WndClass.spClassName = windowClass[count];
		WndClass.opMask = 0;
		WndClass.dwStyle = WS_CHILD | WS_VISIBLE;
		WndClass.dwExStyle = WS_EX_USEPARENTFONT;
		WndClass.hCursor = GetSystemCursor(0);
		WndClass.WinProc = windowProc[count];
		if (WndClass.WinProc != ScanQrCodeProc) {
			WndClass.iBkColor = res_getBGColor();
		} else {
			WndClass.iBkColor = RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00);
		}
		WndClass.dwAddData = 0;
		if (RegisterWindowClass(&WndClass) == FALSE) {
			db_error("register %s failed", windowClass[count]);
			return -1;
		}
	}
	return 0;
}

void UnRegisterWltWindows() {
	for (unsigned int count = 0; count < ARRAY_SIZE(windowClass); count++) {
		UnregisterWindowClass(windowClass[count]);
	}
}
