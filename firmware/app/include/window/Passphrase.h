#ifndef _WIN_PASSPHRASE_H
#define _WIN_PASSPHRASE_H

#include "CommonWindow.h"
#include "wallet_util.h"

class Passphrase : public CommonWindow {
public:
	Passphrase();

	~Passphrase();

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

	int saveWalletName(const char *passphrase);

	int guide();
};

#endif
