#define LOG_TAG "win"

#include "win.h"
#include "win_comm.h"
#include "debug.h"

static HWND GHwnds[WINDOWID_MAXID];

static const char *GWinName[WINDOWID_MAXID] = {
		"MAIN",
		"MAINPANEL",
		"SCAN",
		"QRPROC",
		"TXSHOW",
		"TXMORE",
		"SETTING",
		"GUIDE",
		"COINS_MANAGER",
		"COIN_INFO",
		"SIGN_HISTORY",
		"FACTORY",
		"ABOUT",
		"OTAOK",
		"PASSPHRASE",
		"VERIFY",
		"VERIFY_CODE",
		"RAW_DATA",
};

void win_init() {
	int i;
	for (i = 0; i < WINDOWID_MAXID; i++) {
		GHwnds[i] = HWND_INVALID;
	}
}

const char *win_get_name(int winid) {
	return (winid >= 0 && winid < WINDOWID_MAXID) ? GWinName[winid] : "UNKOWN";
}

void win_set_hwnd(int winid, HWND hWnd) {
	GHwnds[winid] = hWnd;
}

HWND win_get_hwnd(int winid) {
	return GHwnds[winid];
}

int win_show_window(int winid) {
	HWND hwnd = win_get_hwnd(winid);
	if (IS_VALID_HWND(hwnd)) {
		return ShowWindow(hwnd, SW_SHOWNORMAL);
	} else {
		db_error("not VALID HWND id:%d", winid);
	}
	return 0;
}

int win_hide_window(int winid) {
	HWND hwnd = win_get_hwnd(winid);
	if (IS_VALID_HWND(hwnd)) {
		return ShowWindow(hwnd, SW_HIDE);
	} else {
		db_error("not VALID HWND id:%d", winid);
	}
	return 0;
}

int win_send_message(int winid, int iMsg, WPARAM wParam, LPARAM lParam) {
	HWND hwnd = win_get_hwnd(winid);
	if (IS_VALID_HWND(hwnd)) {
		return SendMessage(hwnd, iMsg, wParam, lParam);
	} else {
		db_error("not VALID HWND id:%d", winid);
	}
	return -1;
}

int win_send_notify(int winid, int iMsg, WPARAM wParam, LPARAM lParam) {
	HWND hwnd = win_get_hwnd(winid);
	if (IS_VALID_HWND(hwnd)) {
		return SendNotifyMessage(hwnd, iMsg, wParam, lParam);
	} else {
		db_error("not VALID HWND id:%d", winid);
	}
	return -1;
}
