#ifndef WALLET_SETTINGWIN_H
#define WALLET_SETTINGWIN_H

#include "CommonWindow.h"
#include "ListView.h"
#include "settings.h"

typedef enum {
	SCREEN_SAVER_TIME_15S = 0,
	SCREEN_SAVER_TIME_30S,
	SCREEN_SAVER_TIME_45S,
	SCREEN_SAVER_TIME_60S,
	SCREEN_SAVER_TIMER_MAX
} SCREEN_SAVER_TIME;

typedef enum {
	BRIGHTNESS_LEVEL_1 = 0,
	BRIGHTNESS_LEVEL_2,
	BRIGHTNESS_LEVEL_3,
	BRIGHTNESS_LEVEL_4,
	BRIGHTNESS_LEVEL_MAX
} BRIGHTNESS_LEVEL;

typedef enum {
    RANDOM_PIN_KEYPAD_ON = 0,
    RANDOM_PIN_KEYPAD_OFF,
	RANDOM_PIN_KEYPAD_MAX
} RANDOM_PIN_KEYPAD;

typedef enum {
    BTC_MULTI_ADDRESS_OFF = 0,
    BTC_MULTI_ADDRESS_ON,
    BTC_MULTI_ADDRESS_MAX
} BTC_MULTI_ADDRESS;

enum {
    SETTING_ITEM_PASSPHRASE = 0,
    SETTING_ITEM_VERIFY,
    SETTING_ITEM_SCREEN_OFF,
    SETTING_ITEM_AUTO_SHUTDOWN,
    SETTING_ITEM_BRIGHTNESS_LEVEL,
    SETTING_ITEM_CHANGE_PASSWD,
    SETTING_ITEM_COINS_SHOW_MANAGER,
    SETTING_ITEM_CHANGE_LANG,
	SETTING_ITEM_RANDOM_PIN_KEYPAD,
	SETTING_ITEM_BTC_MULTI_ADDRESS,
    SETTING_ITEM_UPDATE_FIRMWARE,
    SETTING_ITEM_FRESET,
    SETTING_ITEM_DOWNLOAD_APP,
    SETTING_ITEM_ABOUT,
    SETTING_ITEM_MAXID
};

typedef struct {
    int id;
    char has_val;
    char has_sub;
} MENU_SET_CFG;

class SettingWin : public CommonWindow {
public:
	SettingWin();

	~SettingWin();

	PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

	int keyProc(int keyCode, int isLongPress);

private:
	int onCreate(HWND hWnd);

	int onResume();

	int onPause();

	int updateFirmware();

	int reformatDataPartition();

	int resetDevice();

	int changeAutoShutdownTime(void);

	int changeScreenSaverTime(void);

	int changeBrightness(void);

	int changeRandomPinKeypad(void);

    int changeBtcMultiAddress(void);

    int changeLanguage(int *change);

	int changePassword();

	int showDownloadApp();

	int showAboutWallet();

	int showActionSheet(const char *items[], int itemsCnt, int initIndex, const char *title, const char *msg);

	int flushSettingList();

	int getListSetValue(int id, char *value, int size);

	int onClickOK();

	int settings_get_all_items(MENU_SET_CFG settings[SETTING_ITEM_MAXID]);

    ListView *mListView;
	int mLastListId;
	BITMAP mNaviIcon;
	HWND mContainer;
	int mBrightnessEnable;
    MENU_SET_CFG mSupportCfg[SETTING_ITEM_MAXID];
    int mSupportCnt;

};

#endif
