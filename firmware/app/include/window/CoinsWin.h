#ifndef WALLET_COINS_WIN_H
#define WALLET_COINS_WIN_H

#include "CommonWindow.h"
#include "ListView.h"
#include "storage_manager.h"
#include "BitmapGroup.h"

#define ITEM_BUFFER_SIZE 10

class CoinsWin : public CommonWindow {
public:
	CoinsWin();

	~CoinsWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

private:
	int refreshItemList(int init_select = 0);

	int updateItemTotal();

	int updateNaviText();

	int showNotItemTips();

	int onClickItem();

	int mViewOffet;
	int mViewTotal;
	int mItemTotal;
	int mEditMode;
	DBCoinInfo mItems[ITEM_BUFFER_SIZE];
	ListView *mListView;
	BitmapGroup *mBitmapGroup;
};

#endif
