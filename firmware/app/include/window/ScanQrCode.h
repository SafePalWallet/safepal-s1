#ifndef _WALLET_SCANQR_H
#define _WALLET_SCANQR_H

#include <time.h>
#include "key_event.h"
#include "resource.h"
#include "Dialog.h"
#include "common.h"
#include "AppMain.h"
#include "CommonWindow.h"
#include "qr_pack.h"

class ScanQrCode : public CommonWindow {
public:
	ScanQrCode();

	~ScanQrCode();

	int createMainWindow(int winid, HWND hParentWnd);

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int getIconState(int id);

	int onQrResult(int type, qr_packet *packet);

	int onQrChunk(int index, int total);

	int onQrError(int error);

private:
	int mChangeWinFromId;
};

#endif
