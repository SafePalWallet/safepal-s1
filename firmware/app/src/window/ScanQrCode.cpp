#define LOG_TAG "ScanQrCode"

#include <stdlib.h>
#include "ScanQrCode.h"
#include "GuiMain.h"
#include "debug.h"
#include "device.h"
#include "Dialog.h"
#include "PicDialog.h"

enum {
	QRS_ICON_SCAN_BG0 = 0,
	QRS_ICON_SCAN_BG1,
	QRS_ICON_SCAN_BG2,
	QRS_ICON_SCAN_BG3,
	QRS_ICON_PAGE_BG,
	QRS_ICON_MAXID,
};

enum {
	QRS_LABEL_PAGE = 0,
	QRS_LABEL_MAXID,
};


PROC_RET ScanQrCodeProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	ScanQrCode *win = (ScanQrCode *) GetWindowAdditionalData(hWnd);
	return win ? win->commonWinProc(hWnd, message, wParam, lParam) : DefaultWindowProc(hWnd, message, wParam, lParam);
}

ScanQrCode::ScanQrCode() {
	mChangeWinFromId = 0;
}

int ScanQrCode::createMainWindow(int winid, HWND hParentWnd) {
	RECT rect;
	db_msg("createWindow start");
	GetWindowRect(hParentWnd, &rect);
	mWindowId = winid;
	mHwnd = CreateWindowEx(WINDOW_SCANQR, "", WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
	                       WINDOWID_SCAN, rect.left, rect.top, RECTW(rect), RECTH(rect), hParentWnd, (DWORD) this);
	if (mHwnd == HWND_INVALID) {
		db_error("create window failed");
		return -1;
	}
	db_msg("createWindow end");
	return 0;
}

ScanQrCode::~ScanQrCode() {

}

PROC_RET ScanQrCode::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_QR_RESULT:
			onQrResult(wParam, (qr_packet *) lParam);
			break;
		case MSG_QR_CHUNK: {
			onQrChunk(wParam, (int) lParam);
		}
			break;
		case MSG_QR_ERROR: {
			onQrError(wParam);
		}
			break;
		case MSG_CHANGE_WIN_FROM:
			if (wParam > 0 && wParam < WINDOWID_MAXID) {
				mChangeWinFromId = wParam;
			}
			break;
		case MSG_DISABLE_QRSCAN_WIN:
			Global_QR_Proc_Result = wParam;
			changeWindow(mChangeWinFromId ? mChangeWinFromId : WINDOWID_MAINPANEL);
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int ScanQrCode::keyProc(int keyCode, int isLongPress) {
	switch (keyCode) {
		case INPUT_KEY_ESC:
		case INPUT_KEY_OK:
		case INPUT_KEY_LEFT:
			return mChangeWinFromId ? mChangeWinFromId : WINDOWID_MAINPANEL;
	}
	return 0;
}

int ScanQrCode::onCreate(HWND hWnd) {
	RECT rect;
	HWND hMainWin;
	HDC secDC, MainWindowDC;

	hMainWin = GetMainWindowHandle(hWnd);
	GetClientRect(hMainWin, &rect);
	secDC = GetSecondaryDC(hMainWin);
	MainWindowDC = GetDC(hMainWin);

	SetMemDCAlpha(secDC, 0, 0);
	BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), secDC, 0, 0, 0);

	ReleaseDC(MainWindowDC);

	HWND subs[3];
	subs[0] = mHwnd;
	subs[1] = HWND_INVALID;
	subs[2] = HWND_INVALID;
	int ret = createLayoutWidgets(hWnd, subs);
	if (ret != 0) {
		db_error("createLayoutWidgets ret:%d", ret);
		return ret;
	}
	flushLayoutWidget(1);
	return 0;
}

int ScanQrCode::onResume() {
	Global_QR_Proc_Result = 0;
	updateIcon(QRS_ICON_PAGE_BG, 0, false);
	setLabelText(QRS_LABEL_PAGE, "");

	//usleep(2000 * 1000);        
	set_temp_screen_time(180);//screenon
	use_powerkey_as_esc(1);
	return 0;
}

int ScanQrCode::onPause() {
	mChangeWinFromId = 0;
	set_temp_screen_time(0);
	use_powerkey_as_esc(0);
	return 0;
}

int ScanQrCode::getIconState(int id) {
	switch (id) {
		case QRS_ICON_SCAN_BG0:
		case QRS_ICON_SCAN_BG1:
		case QRS_ICON_SCAN_BG2:
		case QRS_ICON_SCAN_BG3:
			return 0;
		case QRS_ICON_PAGE_BG:
			return -1;
		default:
			return -1;
	}
}

int ScanQrCode::onQrResult(int type, qr_packet *packet) {
	return 0;
}

int ScanQrCode::onQrChunk(int index, int total) {
	db_debug("index:%d total:%d", index, total);
	char buf[32];
	snprintf(buf, sizeof(buf), "%d/%d", index + 1, total);
	
	AppMain::getInstance()->qrChunk(strlen(buf), buf);

	return 0;
}

int ScanQrCode::onQrError(int error) {
	db_debug("error:%d", error);
	AppMain::getInstance()->stopPreview();

	if (error == QR_DECODE_UNKOWN_CLIENT) {
		picDialog(mHwnd, "not_bind_with_app_tips", res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
	} else {
		dialog_error3(mHwnd, error, "Scan failed.");
	}
	
	AppMain::getInstance()->startPreview();

	return 0;
}
