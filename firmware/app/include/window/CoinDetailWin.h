
#ifndef WALLET_COINDETAILWIN_H
#define WALLET_COINDETAILWIN_H

#include "CommonWindow.h"
#include "coin_util.h"
#include "bip32.h"

class CoinDetailWin : public CommonWindow {
public:
	CoinDetailWin();

	~CoinDetailWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int genAddress(char *address, int size, uint32_t index);

private:
	int onSetCoinInfo(int type, const char *uname, bool force);

	int refreshCoinInfo();

	int naviButton(int dst);

	int changeCoinAlias();

	int onClickOK();

	int showReceiveAddrList();

	int mNaviIndex;
	int mCoinChanged;
	int mSignTotal;
	int mCoinType;
	char mCoinUname[COIN_UNAME_BUFFSIZE];
	char mAddressUname[COIN_UNAME_BUFFSIZE];
	BITMAP *mBitmapLogo;

	int originalCoinType;
	char originalCoinUname[COIN_UNAME_BUFFSIZE];
	bool singleAddress;

	HDNode *mHDNode;
};


#endif
