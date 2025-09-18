#ifndef _WALLET_TX_RAW_DATA_WIN_H
#define _WALLET_TX_RAW_DATA_WIN_H

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

class TxRawDataWin : public CommonWindow {
public:
	TxRawDataWin();

	~TxRawDataWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

private:
	int initDView();

	int freeWinData();

	void onTxPorcessData(TxPorcessData *txp);

	int getIconState(int id);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int onScrollWindow(int scroll_size);

    int doSignReq();

    int onSignReqData(ProtoClientMessage *req);

    HWND mHwndNaviPanel;
    TxPorcessData mTxp[1];
	DynamicViewCtx mDView[1];
    DBTxCoinInfo mDBTxCoinInfo;
    ProtoClientMessage *mClientMessage;
    int mShowRet;
    BITMAP *mBitmapLogo;
    int mMsgFrom;
    char *mRawDataStr;
};

#endif
