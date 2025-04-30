#ifndef _WALLET_TX_VERIFY_CODE_H
#define _WALLET_TX_VERIFY_CODE_H

#include <time.h>
#include "key_event.h"
#include "resource.h"
#include "Dialog.h"
#include "common.h"
#include "AppMain.h"
#include "CommonWindow.h"
#include "PicObj.h"
#include "wallet_proto_qr.h"
#include "coin_util.h"
#include "coin_adapter.h"
#include "dynamic_win.h"

class TxVerifyCodeWin : public CommonWindow {
public:
	TxVerifyCodeWin();

	~TxVerifyCodeWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

private:
	int initDView();

	int freeWinData();

	int onSignReqData(ProtoClientMessage *req);

	int getIconState(int id);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int doSignReq();

	TxPorcessData mTxp[1];
	DynamicViewCtx mDView[1];
    DBTxCoinInfo mDBTxCoinInfo;

	ProtoClientMessage *mClientMessage;
	HWND mHwndNaviPanel;
	int mShowRet;
    BITMAP *mBitmapLogo;
	int mIsShowDetails;
};

#endif
