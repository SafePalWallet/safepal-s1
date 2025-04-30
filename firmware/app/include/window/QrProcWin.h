#ifndef _QR_PROC_WIN_H
#define _QR_PROC_WIN_H

#include "key_event.h"
#include "Dialog.h"
#include "AppMain.h"
#include "CommonWindow.h"
#include "wallet_proto.h"
#include "wallet_util.h"

class QrProcWin : public CommonWindow {
public:
	QrProcWin();

	~QrProcWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onResume();

	int onPause();

private:
	int onQrError(int error);

	int onQrResult(WPARAM wParam, ProtoClientMessage *msg);

	int genDeviceInfo(struct pbc_wmessage *device, int type);

	int confirmBindAccount();

	int genPubHDNode(struct pbc_wmessage *wmsg, const CoinInfo *info, unsigned char passhash[PASSWD_HASHED_LEN]);

	int syncAllCoins(struct pbc_wmessage *wmsg, unsigned char passhash[PASSWD_HASHED_LEN], struct pbc_rmessage *req);

	int confirmGetPubkey();

	int procUserActive();

    ProtoClientMessage *mMessage;
	int mQrErrorCode;
};

#endif
