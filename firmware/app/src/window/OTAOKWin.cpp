#define LOG_TAG "OTAOK"

#include "OTAOKWin.h"
#include "minigui_comm.h"
#include "common.h"
#include "GuiMain.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include "device.h"
#include "wallet_manager.h"
#include "fileutil.h"
#include "loading_win.h"

OTAOKWin::OTAOKWin() {
}

int OTAOKWin::onCreate(HWND hWnd) {
	return 0;
}

int OTAOKWin::onResume() {
	char str[128] = {0};
	snprintf(str, sizeof(str), res_getLabel(LANG_LABEL_OTA_SUCCESS), DEVICE_APP_VERSION);
	int ret;
	int ret2;
	unsigned char passhash[PASSWD_HASHED_LEN];
	uint64_t account_id = 0;
	GLobal_IN_OTAOK_Win = 1;
	sec_state_info info;
	do {
		ret = dialog_alert(mHwnd, DIALOG_ICON_STYLE_SUCCESS, str, res_getLabel(LANG_LABEL_OTA_END_START_USE));
		if (ret == 0) {
			if (!wallet_isInited()) {
				db_msg("wallet not inited,wait...");
				continue;
			}

			memset(&info, 0, sizeof(sec_state_info));
			account_id = 0;
			if (gHaveSeed) {
				ret = sapi_get_state_info(&info);
				if (ret == 0 && info.seed_state == 1) {
					account_id = info.account_id;
				}
				if (!account_id) {
					settings_set_have_seed(0);
				}
			}

			if (gHaveSeed && account_id) { //try upgrade DB
				storage_upgrade(account_id);
			}

            if (gHaveSeed && gSettings->mCoinsVersion < COINS_INIT_VERSION) {
                memzero(passhash, sizeof(passhash));
                ret2 = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, PASSKB_FLAG_RANDOM);
                db_secure("check passwd ret:%d", ret2);
                if (ret2 != 0) {
                    continue;
                }
                loading_win_start(mHwnd, res_getLabel(LANG_LABEL_LOADING), NULL, 0);
                wallet_initDefaultCoin(passhash);//init new coin
                memzero(passhash, sizeof(passhash));
                loading_win_stop();
            }
            if (!gHaveSeed) {
                changeWindow(WINDOWID_GUIDE);
            } else {
                changeWindow(WINDOWID_MAINPANEL);
            }
            break;
		}
	} while (1);
	settings_save(SETTING_KEY_OTA_PRE_VERSION, 0);
	unlink(OTA_SUCCESS_FILE);
	GLobal_IN_OTAOK_Win = 0;
	device_set_boot_otp();
	return 0;
}

int OTAOKWin::onPause() {
	return 0;
}

OTAOKWin::~OTAOKWin() {

}

PROC_RET OTAOKWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int OTAOKWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d isLongPress:%d", keyCode, isLongPress);
	return 0;
}
