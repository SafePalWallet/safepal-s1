#ifndef _WIN_VERIFYWIN_H
#define _WIN_VERIFYWIN_H

#include "CommonWindow.h"
#include "wallet_util.h"

class VerifyWin : public CommonWindow {
public:
	VerifyWin();

	~VerifyWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);


	int onResume();

	int onPause();

private:
	int isResume;
	int mChangeWin;

	int selectMnemonicCnt();

	int verifyMnemonic(const unsigned char *passwd, char *mnemonics, int size, int mlen);

	int enterPassphrase(char *passphrase, int size);

	int guide();
};

#endif
