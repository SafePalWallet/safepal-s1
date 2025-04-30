#ifndef _QR_GUIDE_WIN_H
#define _QR_GUIDE_WIN_H

#include "CommonWindow.h"
#include "wallet_util.h"

class GuideWin : public CommonWindow {
public:
	GuideWin();

	~GuideWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);


	int onResume();

	int onPause();

private:
	int isResume;

	int setupLang(void);

	int activeDevice();

	int selectMnemonicCnt(void);

	int enterRecoveryWord(char *mnemonics, int size, int mlen);

	int showMnemonicsWillChangeAlert();

	int enterPassword(unsigned char passhash[PASSWD_HASHED_LEN]);

	int saveWalletName(void);

	int showPasswdTips(void);

	int generateMnemonic(uint16_t *indexes, int len, unsigned char *randomBuf);

	int showMnemonic(const uint16_t *indexes, int len);

	int confirmMnemonic(const uint16_t *mnIndexes, int mlen);

	int checkSechip(int type);

	int clean_data();

	int startGuide(void);
};

#endif
