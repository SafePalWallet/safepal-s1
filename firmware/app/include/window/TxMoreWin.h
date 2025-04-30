#ifndef _WALLET_TX_MORE_WIN_H
#define _WALLET_TX_MORE_WIN_H

#include <time.h>
#include "CommonWindow.h"
#include "coin_adapter.h"
#include "dynamic_win.h"

class TxMoreWin : public CommonWindow {
public:
	TxMoreWin();

	~TxMoreWin();

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

	HWND mHwndNaviPanel;
	TxPorcessData *mTxp;
	DynamicViewCtx mDView[1];
};

#endif
