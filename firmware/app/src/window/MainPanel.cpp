#define LOG_TAG "MainPanel"

#include <common.h>
#include "MainPanel.h"
#include "GuiMain.h"
#include "fileutil.h"
#include "secure_api.h"
#include "wallet_util.h"

enum {
	NAVI_INDEX_SCAN = 0,
	NAVI_INDEX_COIN_MANAGER,
	NAVI_INDEX_SIGN_HISTORY,
	NAVI_INDEX_SETTING,
	NAVI_INDEX_MAXID,
};

#define NaviIndex2IconId(i)  ((i) + MPAN_ICON_SCAN)
#define IconId2NaviIndex(i)  ((i) - MPAN_ICON_SCAN)

#define BATTERY_MAX_ICON_LEVEL 5
#define BATTERY_CHARGING_ICON_LEVEL (BATTERY_MAX_ICON_LEVEL+1)

enum {
	MPAN_ICON_BATTERY = 0,
	MPAN_ICON_CHARGING,
	MPAN_ICON_ENTER,
	MPAN_ICON_SCAN,
	MPAN_ICON_COIN_MANAGER,
	MPAN_ICON_SIGN_HISTORY,
	MPAN_ICON_SETTING,
	MPAN_ICON_MAXID,
};

enum {
	MPAN_LABEL_ENTER_NAME = 0,
	MPAN_LABEL_WALLET_NAME,
	MPAN_LABEL_MAXID,
};

static unsigned int ENTRANCE_LANG_IDS[] = {
		LANG_LABEL_SCAN_QRCODE,
		LANG_LABEL_COIN_MANAGER,
		LANG_LABEL_SIGN_HISTORY,
		LANG_LABEL_SYSTEM_SETTING,
};

static int batteryCapacity2Level(int cap) {
	return (int) (cap / (100 / BATTERY_MAX_ICON_LEVEL) + 0.5);
}

MainPanel::MainPanel() {
	mNaviIndex = NAVI_INDEX_SCAN;
	mResume = 0;
	mBatteryChanged = -1;
	mBatteryCharging = -1;
	mLastDeviceAccountIndex = -1;
	int icon_mk_map[MPAN_ICON_MAXID] = {
			MK_mpan_icon_battery,
			MK_mpan_icon_charging,
			MK_mpan_icon_enter,
			MK_mpan_icon_scan,
			MK_mpan_icon_coin_manager,
			MK_mpan_icon_sign_history,
			MK_mpan_icon_setting,
	};

	int label_mk_map[MPAN_LABEL_MAXID] = {
			MK_mpan_label_enter_name,
			MK_mpan_lable_wallet_name,
	};
	initLayout(MPAN_ICON_MAXID, icon_mk_map, MPAN_LABEL_MAXID, label_mk_map);
}

MainPanel::~MainPanel() {

}

PROC_RET MainPanel::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case STBM_TIME_CLICK:
			//onTimeClick();
			break;
		case MSG_BATTERY_CHANGED:
			if (mResume) {
				updateBattery(wParam);
			} else {
				mBatteryChanged = wParam;
			}
			break;
		case MSG_BATTERY_CHARGING:
			if (mResume) {
				updateIcon(MPAN_ICON_CHARGING, wParam ? 1 : 0);
			} else {
				mBatteryCharging = wParam;
			}
			break;
		case MSG_SECHIP_INITED:
			updateWalletName();
			mLastDeviceAccountIndex = Global_Device_Account_Index;
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int MainPanel::onCreate(HWND hWnd) {
	db_msg("hwnd:%d", hWnd);
	createLayoutWidgets(hWnd);
	entranceChanged(0);
	flushWinWidget();
	updateBattery(0);
	return 0;
}

int MainPanel::onResume() {
	db_msg("hwnd:%d", mHwnd);
	// update enter label name, change langue
	unsigned int langid = ENTRANCE_LANG_IDS[mNaviIndex];
	const char *name = res_getLabel(langid);
	mResume = 1;
	setLabelText(MPAN_LABEL_ENTER_NAME, name);
	if (mLastDeviceAccountIndex == -1 || mLastDeviceAccountIndex != Global_Device_Account_Index) {
		updateWalletName();
		mLastDeviceAccountIndex = Global_Device_Account_Index;
	}
	if (mBatteryChanged != -1) {
		updateBattery(mBatteryChanged);
	}
	if (mBatteryCharging != -1) {
		updateIcon(MPAN_ICON_CHARGING, mBatteryCharging ? 1 : 0);
	}
	mBatteryChanged = -1;
	mBatteryCharging = -1;
	return 0;
}

int MainPanel::onPause() {
	db_msg("hwnd:%d", mHwnd);
	mResume = 0;
	mBatteryChanged = -1;
	mBatteryCharging = -1;
	return 0;
}

int MainPanel::entranceChanged(int action) {
	db_msg("action:%d oldindex:%d", action, mNaviIndex);
	int newidex = mNaviIndex + action;
	if (newidex < 0) {
		newidex = (NAVI_INDEX_MAXID - 1);
	} else if (newidex >= NAVI_INDEX_MAXID) {
		newidex = 0;
	}
	int oldicon = NaviIndex2IconId(mNaviIndex);
	int newicon = NaviIndex2IconId(newidex);
	mNaviIndex = newidex;
	updateIcon(oldicon);
	updateIcon(newicon);
	updateIcon(MPAN_ICON_ENTER);
	unsigned int langid = ENTRANCE_LANG_IDS[mNaviIndex];
	const char *name = res_getLabel(langid);
	db_msg("oldicon:%d newicon:%d action:%d newindex:%d langid:%d name:%s", oldicon, newicon, action, newidex, langid, name);
	setLabelText(MPAN_LABEL_ENTER_NAME, name);

	return 0;
}

int MainPanel::onEntranceClicked() {
	db_msg("nav index:%d", mNaviIndex);
	switch (mNaviIndex) {
		case NAVI_INDEX_SCAN:
			return WINDOWID_SCAN;
			break;
		case NAVI_INDEX_SETTING:
			return WINDOWID_SETTING;
			break;
		case NAVI_INDEX_COIN_MANAGER:
			if (gHaveSeed) {
				GLobal_CoinsWin_EditMode = 0;
				return WINDOWID_COINS_MANAGER;
			}
			break;
		case NAVI_INDEX_SIGN_HISTORY:
			if (gHaveSeed) {
				GuiMain::getInstance()->sendMessage(WINDOWID_SIGN_HISTORY, MSG_SET_COIN_INFO, 0, 0);
				return WINDOWID_SIGN_HISTORY;
			}
			break;
		default:
			return 0;
	}
	return 0;
}

int MainPanel::keyProc(int keyCode, int isLongPress) {
	//db_debug("key event %d", keyCode);
	switch (keyCode) {
		case INPUT_KEY_LEFT:
			entranceChanged(-1);
			break;
		case INPUT_KEY_RIGHT:
			entranceChanged(1);
			break;
		case INPUT_KEY_OK: {
			return onEntranceClicked();
		}
			break;
	}
	return 0;
}

int MainPanel::getIconState(int id) {
	switch (id) {
		case MPAN_ICON_BATTERY:
			return batteryCapacity2Level(device_battery_capacity());

		case MPAN_ICON_CHARGING:
			return device_battery_is_charging();
			break;
		case MPAN_ICON_ENTER:
			return mNaviIndex;
		case MPAN_ICON_SCAN:
			return (mNaviIndex == NAVI_INDEX_SCAN) ? 1 : 0;
		case MPAN_ICON_COIN_MANAGER:
			return (mNaviIndex == NAVI_INDEX_COIN_MANAGER) ? 1 : 0;
		case MPAN_ICON_SIGN_HISTORY:
			return (mNaviIndex == NAVI_INDEX_SIGN_HISTORY) ? 1 : 0;
		case MPAN_ICON_SETTING:
			return (mNaviIndex == NAVI_INDEX_SETTING) ? 1 : 0;
		default:
			return -1;
	}
}

int MainPanel::onChangeFont() {
	return 0;
}

void MainPanel::flushWinWidget() {
	db_msg("flushWinWidget");
	flushLayoutWidget(1);
}

void MainPanel::updateBattery(int cap) {
	char buf[32];
	if (cap <= 0) {
		cap = device_battery_capacity();
	}
	snprintf(buf, sizeof(buf), "%d%%", cap);
	db_msg("cap:%d str:%s", cap, buf);
	updateIcon(MPAN_ICON_BATTERY, batteryCapacity2Level(cap));
	if (cap <= 0) {
		AppMain::getInstance()->deleteMessage(CALLBACK_MSG_REFRESH_BATTERY);
		AppMain::getInstance()->postMessageDelay(1000, CALLBACK_MSG_REFRESH_BATTERY);
	}
}

void MainPanel::updateWalletName() {
	char name[32] = {0};
	get_device_name(name, sizeof(name), 1);
	db_msg("name:%s", name);
	setLabelText(MPAN_LABEL_WALLET_NAME, name, 1);
}
