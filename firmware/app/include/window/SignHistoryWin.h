#ifndef TRUNK_SIGNHISTORYWIN_H
#define TRUNK_SIGNHISTORYWIN_H

#include "CommonWindow.h"
#include "ListView.h"
#include "storage_manager.h"
#include "BitmapGroup.h"

#define SIGNH_ITEM_SIZE 10

class SignHistoryWin : public CommonWindow {
public:
	SignHistoryWin();

	~SignHistoryWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

private:

	int showSelectTxType();

	int flushSignHistoryList(int init_select = 0);

	int updateTxTotal();

	int updateNaviText();

	int showNotSignTips();

	int onClickTx();

	DBTxFilter *getDBTxFilter();

	int onSetCoinInfo(int type, const char *uname);

	int mViewOffet;
	int mViewTotal;
	int mTxTotal;
	DBTxInfo mTxs[SIGNH_ITEM_SIZE];
	ListView *mListView;
	BitmapGroup *mBitmapGroup;
	HWND mContainer;
	int mCoinType;
	char mCoinUname[COIN_UNAME_BUFFSIZE];
	DBTxFilter mFilter;
	int mSelectTxType;
	int mLastSelectTxType;
};

#endif
