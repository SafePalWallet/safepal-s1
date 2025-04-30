#define LOG_TAG "SettingWin"

#include "SettingWin.h"

#include "SettingWin.h"
#include "minigui_comm.h"
#include "GuiMain.h"
#include "debug.h"
#include "wallet_util.h"
#include "secure_api.h"
#include "Dialog.h"
#include "ActionSheetDialog.h"
#include "AppMain.h"
#include "common.h"
#include "wallet_manager.h"
#include "PicDialog.h"
#include "upgrade_helper.h"
#include "loading_win.h"
#include "device.h"

static int gScreenTimes[SCREEN_SAVER_TIMER_MAX] = {15, 30, 45, 60};
static const char *gScreenTimeStr[SCREEN_SAVER_TIMER_MAX] = {"15S", "30S", "45S", "60S"};
static int gBrightnessLevel[BRIGHTNESS_LEVEL_MAX] = {1, 2, 3, 4};
static const char *gBrightnessLevelStr[BRIGHTNESS_LEVEL_MAX] = {"1", "2", "3", "4"};
static int gRandPinKeypad[RANDOM_PIN_KEYPAD_MAX] = {0, 1};
static const char *gRandPinKeypadStr[RANDOM_PIN_KEYPAD_MAX] = {"ON", "OFF"};
static int gBtcMultiAddress[BTC_MULTI_ADDRESS_MAX] = {0, 1};

enum {
	PINCODE_SETUP_STEP_VERIFY = 0,
	PINCODE_SETUP_STEP_ENTER_NEW,
	PINCODE_SETUP_STEP_CONFIRM,
	PINCODE_SETUP_STEP_STORE
};

enum {
	SETTING_LABEL_TITLE = 0,
	SETTING_LABEL_MAXID,
};


#define IDC_LIST_PARENT_CONTAINER 650

MENU_SET_CFG SETTING_LABEL_CFG[SETTING_ITEM_MAXID] = {
        {LANG_LABEL_ITEM_PASSPHRASE,        0, 0},
        {LANG_LABEL_ITEM_VERIFY,            0, 0},
        {LANG_LABEL_SET_ITEM_SCREEN_OFF,    1, 1},
        {LANG_LABEL_SET_ITEM_AUTO_SHUTDOWN, 1, 1},
        {LANG_LABEL_SET_BRIGHTNESS_LEVEL,   1, 1},
        {LANG_LABEL_SET_ITEM_CHANGE_PASSWD, 0, 0},
        {LANG_LABEL_COIN_TYPE_MANAGER,		1, 1},
        {LANG_LABEL_SET_ITEM_CHANGE_LANG,   1, 1},
        {LANG_LABEL_SET_RANDOM_PIN_KEYPAD,  0, 1},
        {LANG_LABEL_BTC_MULTI_ADDRESS,      0, 1},
        {LANG_LABEL_UPDATE_FIRMWARE,        0, 0},
        {LANG_LABEL_MENU_FRESET,            0, 0},
        {LANG_LABEL_SET_ITEM_DOWNLOAD_APP,  0, 0},
        {LANG_LABEL_SET_ITEM_ABOUT,         0, 0},
};

SettingWin::SettingWin() {
	mListView = NULL;
	mContainer = HWND_INVALID;
	mLastListId = 0;
	memset(&mNaviIcon, 0, sizeof(mNaviIcon));
    mBrightnessEnable = 0;
    memset(mSupportCfg, 0x0, sizeof(MENU_SET_CFG));
    mSupportCnt = SETTING_ITEM_MAXID;

}

SettingWin::~SettingWin() {
	if (mListView) {
		mListView->clean();
		delete mListView;
		mListView = NULL;
	}
	if (IS_VALID_HWND(mContainer)) {
		DestroyWindow(mContainer);
		mContainer = HWND_INVALID;
	}
	if (mNaviIcon.bmBits) {
		UnloadBitmap(&(mNaviIcon));
	}
}

int SettingWin::onCreate(HWND hWnd) {
	createLayoutWidgets(hWnd);

	WIN_RECT rect;
	res_getPos(MK_set_list_parent_container, &rect);
	mContainer = createWidgetWindow(hWnd, 0, rect.x, rect.y, rect.w, rect.h, IDC_LIST_PARENT_CONTAINER, WIDGET_TYPE_NORMAL, 0, -1);
	if (IS_VALID_HWND(mContainer)) {
		SetWindowBkColor(mContainer, res_getBGColor());
		mListView = new ListView(mContainer, MK_set_list_container);
		res_getBmpByKey(ICON_STATE_KEY(MK_setting_menu_navi, 0), &mNaviIcon);
	}

	return 0;
}

int SettingWin::onResume() {
	db_debug("onResume");
    int ret = device_can_brightness_adjust();
    if (ret == 0) {
        mBrightnessEnable = 1;
    }
	flushSettingList();
	return 0;
}

int SettingWin::flushSettingList() {
	int icon_mk[] = {MK_set_icon_navi};
	int text_mk[] = {MK_set_label_item, MK_set_label_value};

    mSupportCnt = settings_get_all_items(mSupportCfg);
    db_msg("setting mSupportCnt=%d\n", mSupportCnt);
    mListView->init(mSupportCnt, ARRAY_SIZE(icon_mk), icon_mk, ARRAY_SIZE(text_mk), text_mk);
	mListView->setMoveCyclic(true);
	char value[20];
	MENU_SET_CFG *cfg;
	for (int id = 0; id < mSupportCnt; id++) {
		cfg = &mSupportCfg[id];
		mListView->setLabelText(id, 0, res_getLabel(cfg->id));
		if (cfg->has_val && getListSetValue(id, value, sizeof(value)) > 0) {
			mListView->setLabelText(id, 1, value);
		}
		if (cfg->has_sub) {
			mListView->setIconImage(id, 0, &mNaviIcon);
		}
	}
    if (Global_Guide_abort) {
        if (!mBrightnessEnable) {
            mListView->select(SETTING_ITEM_UPDATE_FIRMWARE - 1);
        } else {
            mListView->select(SETTING_ITEM_UPDATE_FIRMWARE);
        }
    } else {
		if (mLastListId) {
			mListView->select(mLastListId);
		} else {
			mListView->select(0);
		}
	}
	return 0;
}

int SettingWin::getListSetValue(int id, char *value, int size) {
	int v;
	switch (id) {
		case SETTING_ITEM_SCREEN_OFF:
			v = gSettings->mScreenSaver;
			if (v == 0) {
				strcpy(value, "Off");
			} else {
				snprintf(value, size, "%dS", v);
			}
			return 1;
			break;
		case SETTING_ITEM_AUTO_SHUTDOWN:
			v = gSettings->mAutoShutdownTime;
			if (v == 0) {
				strcpy(value, "OFF");
			} else {
				snprintf(value, size, "%dS", v);
			}
			return 1;
			break;
        case SETTING_ITEM_BRIGHTNESS_LEVEL:
            v = gSettings->mBrightness;
            if (v == 0) {
                strcpy(value, "OFF");
            } else {
                snprintf(value, size, "%d", v);
            }
            return 1;
            break;
        case SETTING_ITEM_RANDOM_PIN_KEYPAD:
            v = gSettings->mRandPinKeypad;
            if ((v == 0) || (v == 1)) {
                strcpy(value, gRandPinKeypadStr[v]);
            } else {
                return 0;
            }
            return 1;
            break;
        default:
			return 0;
	}
}

int SettingWin::onPause() {
	db_debug("onPause");
	mListView->clean();
	return 0;
}

PROC_RET SettingWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_INITDIALOG:
			break;
		default:
			break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int SettingWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d", keyCode);
	switch (keyCode) {
		case INPUT_KEY_OK:
			onClickOK();
			break;
		case INPUT_KEY_UP:
			mListView->move(-1);
			break;
		case INPUT_KEY_DOWN:
			mListView->move(1);
			break;
		case INPUT_KEY_LEFT:
			return WINDOWID_MAINPANEL;
			break;
		case INPUT_KEY_RIGHT: {
			int id = mListView->getSelectIndex();
			if (id >= 0 && id < mSupportCnt && mSupportCfg[id].has_sub) {
				onClickOK();
			}
		}
			break;
		default:
			break;
	}
	return 0;
}

int SettingWin::updateFirmware() {
	int ret = 0;
	int state;	
	int flag = 0;

    if (device_battery_capacity() < BATTERY_MIDDLE_LEVEL) {
        PicDialogConfig_t config;
        const char *btnTitles[1] = {0};
        memset(&config, 0, sizeof(PicDialogConfig_t));
        config.name = "upgrade_battery_tips";
        config.total = 1;
        btnTitles[0] = res_getLabel(LANG_LABEL_TXT_OK);
        config.btnTitles = btnTitles;
        config.buttonCount = 1;
        config.initIndex = 0;
        config.initBtnIndex = 0;
        config.flag = 0;

        do {
            ret = showPicDialog(mHwnd, &config);
            if (ret == KEY_EVENT_OK || !ret) {
                flag = 1;
            }
        } while (!flag);

        return ret;
    } else {
        if (device_usb_online() == 0) {
            PicDialogConfig_t config;
            const char *btnTitles[1] = {0};
            memset(&config, 0, sizeof(PicDialogConfig_t));
            config.name = "upgrade_charge_tips";
            config.total = 1;
            btnTitles[0] = res_getLabel(LANG_LABEL_TXT_OK);
            config.btnTitles = btnTitles;
            config.buttonCount = 1;
            config.initIndex = 0;
            config.initBtnIndex = 0;
            config.flag = 0;

            do {
                ret = showPicDialog(mHwnd, &config);
                if (ret == KEY_EVENT_OK || !ret) {
                    flag = 1;
                }
            } while (!flag);

            return ret;
        }
    }

	db_msg("start");
	state = 0;
	do {
		if (state == 0) {
			ret = picDialog(mHwnd, "upgrade_backup_tips", res_getLabel(LANG_LABEL_SUBMENU_CANCEL), res_getLabel(LANG_LABEL_ALREADY_BACKUP), 0);
			if (ret <= 0) {
				return 0;
			}
			state++;
		}
		if (state == 1) {
			ret = picDialog(mHwnd, "download_firmware_tips", res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0);
			if (ret == KEY_EVENT_BACK) {
				state--;
				continue;
			} else if (ret < 0) {
				db_error("download firmware tips failed:%d", ret);
				return -1;
			}
			state++;
		}
		break;
	} while (1);

	storage_syncData2Disk(); //sync force

	int passwd_error = 0;
	upgrade_init();
	state = upgrade_getState();
	db_msg("state:%d", state);
	if (!(state & OTA_DISK_STATE_SHARE)) {
		db_msg("start shareToPC");
		ret = upgrade_shareToPC();
		db_msg("share ret:%d", ret);
	}
	do {
		ret = picDialogFlag(mHwnd, "connect_pc_tips", res_getLabel(LANG_LABEL_UPGRADE), res_getLabel(LANG_LABEL_EXIT_UPGRADE_MODE), 0, PIC_DLG_FLAG_LEFT_NOT_BACK);
		db_msg("ret:%d", ret);
		if (ret == KEY_EVENT_BACK) {
			continue;
		} else if (ret != 0) {
			if (ret < 0) {
				db_error("connect pc tips false ret:%d", ret);
			}
#ifdef BUILD_FOR_DEV
			if (access(DATA_POINT"/not_close_usb.txt", F_OK) == 0) {
				db_msg("dev not_close_usb");
				break;
			}
#endif
			upgrade_clean();
			break;
		}
		Global_Key_Shield = 1;
		for (int t = 0; t < 3; t++) {
			ret = upgrade_prepare(mHwnd, 0);
			if (ret == UPGRADE_FW_VERIFY_DIGEST_FAILED) {
				if (!loading_win_processing) loading_win_start(mHwnd, res_getLabel(LANG_LABEL_CHECKING_FIRMWARE), NULL, 0);
				usleep(100 * 1000);
				loading_win_refresh_timeout(200);
			} else {
				break;
			}
		}
		if (loading_win_processing) {
			loading_win_stop();
		}
		Global_Key_Shield = 0;
		if (ret != 0) {
			db_error("prepare upgrade false ret:%d", ret);
			if (ret == -101) {
				picDialog(mHwnd, "not_found_firmware", res_getLabel(LANG_LABEL_BACK), NULL, 0);
			} else {
				dialog_system_error2(mHwnd, ret, res_getLabel(LANG_LABEL_VERIFIED_FIRMWARE_FAIL), res_getLabel(LANG_LABEL_BACK));
			}
		} else {
			db_msg("prepare upgrade OK");
			ret = picDialog(mHwnd, "found_firmware_tip", res_getLabel(LANG_LABEL_UPGRADE), NULL, 0);
			db_msg("start do ota ret:%d", ret);
			if (ret == 0) {
				if (gHaveSeed) {
					if (checkPasswdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD),  PASSKB_FLAG_RANDOM | PASSKB_FLAG_NOT_SWITCH_GUIDE) != 0) {
						if (!gHaveSeed) {
							passwd_error = 1;
						}
						continue;
					}
				}
				unlink(OTA_SUCCESS_FILE);
				settings_save(SETTING_KEY_OTA_PRE_VERSION, DEVICE_APP_INT_VERSION);
				Global_Key_Shield = 1;
				ret = upgrade_do_upgrade(mHwnd);
				if (ret == 0) {
					ret = AppMain::getInstance()->otaUpdate();
				} else {
					db_error("upgrade ret:%d", ret);
				}
				Global_Key_Shield = 0;
				if (ret != 0) {
					db_error("upgrade ret:%d", ret);
					if (ret == -101) {
						settings_save(SETTING_KEY_OTA_PRE_VERSION, 0);
						picDialog(mHwnd, "not_found_firmware", res_getLabel(LANG_LABEL_BACK), NULL, 0);
					} else if (ret == -102) {
						settings_save(SETTING_KEY_OTA_PRE_VERSION, 0);
						picDialog(mHwnd, "verified_firmware_failed", res_getLabel(LANG_LABEL_BACK), NULL, 0);
					}
				}
				break;
			}
		}
	} while (1);
	if (passwd_error) {
		changeWindow(WINDOWID_GUIDE);
	}
	return 0;
}

int SettingWin::resetDevice() {
	int ret = 0;
	ret = picDialog(mHwnd, "disclaimer_alert_delete_wallet", res_getLabel(LANG_LABEL_I_KNOW), NULL, 0);
	if (ret == KEY_EVENT_BACK) {
		return ret;
	} else if (ret < 0) {
		db_msg("disclaimer alert false:%d", ret);
		return -1;
	}
	do {
		ret = picDialog(mHwnd, "delete_wallet_alert", res_getLabel(LANG_LABEL_SUBMENU_CANCEL), res_getLabel(LANG_LABEL_TXT_OK), 0);
		if (ret != 1) {
			return 0;
		}
		loading_win_start(mHwnd, res_getLabel(LANG_LABEL_PROCESSING), NULL, 0);
		ret = wallet_destorySeed(1);
		loading_win_refresh();
		db_msg("destorySeed ret:%d", ret);
		AppMain::getInstance()->doFactoryReset();
		loading_win_refresh();
		ret = device_reformat_data_partition();
		db_msg("reformat data ret:%d", ret);
		if (ret != 0) {
			loading_win_stop();
			dialog_error3(mHwnd, ret, "Reset failed.");
			continue;
		}
		break;
	} while (1);

	if (gSettings->mLang != 0) {
		settings_save(SETTING_KEY_LANGUAGE, settings_get_lang());
	}
	long datasize = device_data_partition_size();
	db_msg("datasize:%ld", datasize);
	if (datasize < 1024 * 1024) {
		loading_win_refresh_timeout(500);
	}
	loading_win_stop();
	picDialog(mHwnd, "wallet_reset_successs_tip", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0);
	changeWindow(WINDOWID_GUIDE);
	return 0;
}

int SettingWin::showActionSheet(const char *items[], int itemsCnt, int initIndex, const char *title, const char *msg) {
	if (items == NULL) {
		db_error("invalid items:%p", items);
		return -1;
	}
	if (initIndex >= itemsCnt) {
		db_error("invalid initIndex:%d", initIndex);
		return -1;
	}
	db_msg("itemsCnt:%d initIndex:%d title:%s, msg:%s", itemsCnt, initIndex, title, msg);
	ActionSheetDialogConfig_t _config;
	ActionSheetDialogConfig_t *config = &_config;
	memset(config, 0, sizeof(ActionSheetDialogConfig_t));
	int ret = 0;
	config->title.data = title;
	config->msg.data = msg;
	config->itemsCnt = itemsCnt;
	config->items = items;
	config->initIndex = initIndex;
	ret = showActionSheetDialog(mHwnd, config);
	db_msg("action dialog ret:%d", ret);
	return ret;
}

#define AUTO_OFF_TIME_CNT 4

int SettingWin::changeAutoShutdownTime(void) {
	int count = AUTO_OFF_TIME_CNT;
	static int GAutoOffTime[AUTO_OFF_TIME_CNT] = {30, 45, 60, 120};
	if (app_run_in_dev_mode()) {
		GAutoOffTime[AUTO_OFF_TIME_CNT - 1] = 0;
	}

	const char *items[AUTO_OFF_TIME_CNT * 10] = {0};
	int initIndex = 0;
	int ret = 0;
	char str[32] = {0};

	char datas[AUTO_OFF_TIME_CNT * 10] = {0};

	for (int j = 0; j < count; ++j) {
		if (GAutoOffTime[j] == 0) {
			snprintf(str, sizeof(str), "Off");
		} else {
			snprintf(str, sizeof(str), "%dS", GAutoOffTime[j]);
		}
		memcpy(datas + 10 * j, str, strlen(str));
		items[j] = datas + 10 * j;
	}
	for (int i = 0; i < count; ++i) {
		if (GAutoOffTime[i] == gSettings->mAutoShutdownTime) {
			initIndex = i;
		}
	}
	ret = showActionSheet(items, count, initIndex, res_getLabel(LANG_LABEL_SET_ITEM_AUTO_SHUTDOWN), res_getLabel(LANG_LABEL_AUTO_OFF_TIPS));
	if (ret == KEY_EVENT_BACK || ret < 0) {
		return ret;
	}
	if (ret >= 0 && ret < count) {
		ret = AppMain::getInstance()->uiCallBack(UI_EVENT_SHUTDOWN, GAutoOffTime[ret]);
		db_msg("auto shutdown time:%d ret:%d", GAutoOffTime[ret], ret);
	}
	getListSetValue(SETTING_ITEM_AUTO_SHUTDOWN, str, 32);
	mListView->setLabelText(SETTING_ITEM_AUTO_SHUTDOWN, 1, str);

	return 0;
}

int SettingWin::changeScreenSaverTime(void) {
	int initIndex = 0;
	int ret = 0;
	char str[32] = {0};
	for (int i = SCREEN_SAVER_TIME_15S; i < SCREEN_SAVER_TIMER_MAX; ++i) {
		if (gScreenTimes[i] == (gSettings->mScreenSaver)) {
			initIndex = i;
			break;
		}
	}
	ret = showActionSheet(gScreenTimeStr, SCREEN_SAVER_TIMER_MAX, initIndex, res_getLabel(LANG_LABEL_SET_ITEM_SCREEN_OFF), NULL);
	if (ret == KEY_EVENT_BACK || ret < 0) {
		return ret;
	}
	if (ret >= 0 && ret < SCREEN_SAVER_TIMER_MAX) {
		ret = AppMain::getInstance()->uiCallBack(UI_EVENT_SET_SCREEN_SAVER, gScreenTimes[ret]);
		db_msg("screen saver time:%d ret:%d", gScreenTimes[ret], ret);
	}
	getListSetValue(SETTING_ITEM_SCREEN_OFF, str, 32);
	mListView->setLabelText(SETTING_ITEM_SCREEN_OFF, 1, str);

	return 0;
}

int SettingWin::changeBrightness(void) {
    int initIndex = 0;
    int ret = 0;
    char str[32] = {0};

    for (int i = BRIGHTNESS_LEVEL_1; i < BRIGHTNESS_LEVEL_MAX; ++i) {
        if (gBrightnessLevel[i] == (gSettings->mBrightness)) {
            initIndex = i;
            break;
        }
    }
    ret = showActionSheet(gBrightnessLevelStr, BRIGHTNESS_LEVEL_MAX, initIndex, res_getLabel(LANG_LABEL_SET_BRIGHTNESS_LEVEL), NULL);
    if (ret == KEY_EVENT_BACK || ret < 0) {
        return ret;
    }
    if (ret >= 0 && ret < BRIGHTNESS_LEVEL_MAX) {
        device_set_brightness_level(gBrightnessLevel[ret]);
        db_msg("brightness level:%d ret:%d", gBrightnessLevel[ret], ret);
        ret = AppMain::getInstance()->uiCallBack(UI_EVENT_SET_BRIGHTNESS, gBrightnessLevel[ret]);
    }
    getListSetValue(SETTING_ITEM_BRIGHTNESS_LEVEL, str, 32);
    mListView->setLabelText(SETTING_ITEM_BRIGHTNESS_LEVEL, 1, str);

    return 0;
}

int SettingWin::changeRandomPinKeypad() {
    int initIndex = 0;
    int ret = 0;

    for (int i = RANDOM_PIN_KEYPAD_ON; i < RANDOM_PIN_KEYPAD_MAX; ++i) {
        if (gRandPinKeypad[i] == (gSettings->mRandPinKeypad)) {
            initIndex = i;
            break;
        }
    }
    ret = showActionSheet(gRandPinKeypadStr, RANDOM_PIN_KEYPAD_MAX, initIndex, res_getLabel(LANG_LABEL_SET_RANDOM_PIN_KEYPAD), NULL);
    if (ret == KEY_EVENT_BACK || ret < 0) {
        return ret;
    }
    if (ret >= 0 && ret < RANDOM_PIN_KEYPAD_MAX) {
        ret = AppMain::getInstance()->uiCallBack(UI_EVENT_SET_RAND_PIN_KEYPAD, gRandPinKeypad[ret]);
        db_msg("screen saver time:%d ret:%d", gRandPinKeypad[ret], ret);
    }

    return 0;
}

int SettingWin::changeBtcMultiAddress(void) {
    int ret, state = 0, initIndex = 0;

    do {
        if (state == 0) {
            ret = picDialog(mHwnd, "btc_multi_address_tips", res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0);
            if (ret < 0) {
                return 0;
            }
            state++;
        }

        if (state == 1) {
            for (int i = BTC_MULTI_ADDRESS_OFF; i < BTC_MULTI_ADDRESS_MAX; ++i) {
                if (gBtcMultiAddress[i] == (gSettings->mBtcMultiAddress)) {
                    initIndex = i;
                    break;
                }
            }
            ret = picDialog(mHwnd, "btc_multi_address_select_tips", res_getLabel(LANG_LABEL_BTC_SINGLE_ADDRESS), res_getLabel(LANG_LABEL_BTC_MULTI_ADDRESS), initIndex);
            if (ret == KEY_EVENT_BACK) {
                state--;
                continue;
            } else if (ret < 0) {
                db_error("download firmware tips failed:%d", ret);
                return -1;
            }
            break;
        }
    } while (1);

    if (ret >= 0 && ret < BTC_MULTI_ADDRESS_MAX) {
        db_msg("multi address:%d ret:%d", gBtcMultiAddress[ret], ret);
        AppMain::getInstance()->uiCallBack(UI_EVENT_SET_BTC_MULTI_ADDRESS, gBtcMultiAddress[ret]);
    }

    return 0;
}

int SettingWin::changeLanguage(int *change) {
	int support[LANG_MAXID] = {0};
	const char *langs[LANG_MAXID] = {0};
	int supportCnt = settings_get_all_langs(support);
	for (int j = 0; j < supportCnt; ++j) {
		langs[j] = res_getLangName(support[j]);
	}
	int curLang = settings_get_lang();
	int init_index = 0;
	for (int i = 0; i < supportCnt; ++i) {
		if (curLang == support[i]) {
			init_index = i;
			break;
		}
	}

	ActionSheetDialogConfig_t _config;
	ActionSheetDialogConfig_t *config = &_config;
	memset(config, 0, sizeof(ActionSheetDialogConfig_t));
	config->itemsCnt = supportCnt;
	config->items = langs;
	config->initIndex = init_index;
	config->title.data = "Language";
	//config->flag = ACTION_SHEET_DIALOG_FLAG_NAVI;

	int ret = showActionSheetDialog(mHwnd, config);
	if (ret == KEY_EVENT_BACK) {
		return 0;
	} else if (ret < 0 || ret >= supportCnt) {
		db_error("setup lang action sheet false %d", ret);
		return -1;
	}
	int newLang = support[ret];
	db_msg("lang new:%d current:%d ret:%d", newLang, curLang, ret);
	if (newLang != curLang && (IS_VALID_LANG_ID(newLang))) {
		if (settings_save(SETTING_KEY_LANGUAGE, newLang)) {
			db_msg("lang:%d save false", newLang);
			return -1;
		}
		if (change) {
			*change = 1;
		}
		res_updateLangAndFont(newLang);
	}
	return 0;
}

int SettingWin::changePassword() {
	int ret = 0;
	unsigned char oldPasswd[PASSWD_HASHED_LEN] = {0};
	unsigned char newPasswd[PASSWD_HASHED_LEN] = {0};
	unsigned char confirmPasswd[PASSWD_HASHED_LEN] = {0};
	int step = PINCODE_SETUP_STEP_VERIFY;
	int err = 0;
	do {
		switch (step) {
			case PINCODE_SETUP_STEP_VERIFY: {
				ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_OLD_PASSWD), PIN_CODE_VERITY, oldPasswd, 1);
				if (!ret) {
					step++;
				} else {
					db_error("check old passwd error ret:%d", ret);
					err = 1;
				}
			}
				break;
			case PINCODE_SETUP_STEP_ENTER_NEW: {
				ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_NEW_PASSWD), PIN_CODE_CHECK, newPasswd, 0);
				if (!ret) {
					step++;
				} else {
					db_error("enter passwd error ret:%d", ret);
					err = 1;
				}
			}
				break;
			case PINCODE_SETUP_STEP_CONFIRM: {
				ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_CONFIRM_PASSWD), PIN_CODE_NONE, confirmPasswd, 0);
				if (!ret) {
					if (memcmp(newPasswd, confirmPasswd, PASSWD_HASHED_LEN) == 0) {
						step++;
					} else {
						picDialog(mHwnd, "inconsistent_pin_code", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
						step = PINCODE_SETUP_STEP_ENTER_NEW;
					}
				} else {
					err = 1;
				}
			}
				break;
			case PINCODE_SETUP_STEP_STORE: {
				ret = sapi_change_passwd((const unsigned char *) oldPasswd, PASSWD_HASHED_LEN, (const unsigned char *) newPasswd, PASSWD_HASHED_LEN);
				if (!ret) {
					memzero(oldPasswd, PASSWD_HASHED_LEN);
					memzero(newPasswd, PASSWD_HASHED_LEN);
					memzero(confirmPasswd, PASSWD_HASHED_LEN);
					picDialog(mHwnd, "change_pin_code_success", res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
					return 0;
				} else {
					char errTips[128] = {0};
					snprintf(errTips, sizeof(errTips), res_getLabel(LANG_LABEL_SYSTEM_ERR), ret);
					picDialog(mHwnd, errTips, res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
					err = 1;
				}
			}
				break;
			default:
				break;
		}

	} while (!err);
	memzero(oldPasswd, PASSWD_HASHED_LEN);
	memzero(newPasswd, PASSWD_HASHED_LEN);
	memzero(confirmPasswd, PASSWD_HASHED_LEN);

	return err ? -1 : 0;
}

int SettingWin::showDownloadApp() {
	picDialog(mHwnd, "download_app_qr", NULL, NULL, 0);
	return 0;
}

int SettingWin::showAboutWallet() {
	changeWindow(WINDOWID_ABOUT);
	return 0;
}

int SettingWin::onClickOK() {
	int id = mListView->getSelectIndex();
    int changed = 0;
    int tmpId = 0;
    mLastListId = id;

    if ((!mBrightnessEnable) && (id >= SETTING_ITEM_BRIGHTNESS_LEVEL)) {
        tmpId = id + 1;
    } else {
        tmpId = id;
    }

    db_msg("tmpId=%d\n", tmpId);

    switch (tmpId) {
        case SETTING_ITEM_CHANGE_PASSWD: {
			changePassword();
		}
			break;
		case SETTING_ITEM_AUTO_SHUTDOWN: {
			changeAutoShutdownTime();
		}
			break;
		case SETTING_ITEM_SCREEN_OFF: {
			db_msg("SETTING_ITEM_SCREEN_OFF");
			changeScreenSaverTime();
		}
			break;
		case SETTING_ITEM_CHANGE_LANG: {
			changeLanguage(&changed);
			if (changed) {
				flushSettingList();
				setLabelText(SETTING_LABEL_TITLE, res_getLabel(LANG_LABEL_SET_TITLE), true);
			}
		}
			break;
		case SETTING_ITEM_UPDATE_FIRMWARE: {
			set_temp_screen_time(-1);
			if (!gProcessing) {
				gProcessing = 1;
				updateFirmware();
				gProcessing = 0;
			}
			set_temp_screen_time(0);
		}
			break;
		case SETTING_ITEM_FRESET: {
			resetDevice();
		}
			break;

		case SETTING_ITEM_DOWNLOAD_APP: {
			showDownloadApp();
		}
			break;
		case SETTING_ITEM_ABOUT: {
			showAboutWallet();
		}
			break;
		case SETTING_ITEM_PASSPHRASE: {
			changeWindow(WINDOWID_PASSPHRASE);
		}
			break;
		case SETTING_ITEM_VERIFY: {
			changeWindow(WINDOWID_VERIFY);
		}
			break;
		case SETTING_ITEM_COINS_SHOW_MANAGER: {
			GLobal_CoinsWin_EditMode = 1;
			changeWindow(WINDOWID_COINS_MANAGER);
		}
            break;
        case SETTING_ITEM_BRIGHTNESS_LEVEL: {
            changeBrightness();
        }
            break;
        case SETTING_ITEM_RANDOM_PIN_KEYPAD:{
            changeRandomPinKeypad();
        }
            break;
        case SETTING_ITEM_BTC_MULTI_ADDRESS:{
            changeBtcMultiAddress();
        }
            break;
		default:
			break;
	}
	return 0;
}

int SettingWin::settings_get_all_items(MENU_SET_CFG settings[SETTING_ITEM_MAXID]) {
    int n = 0;
	
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_PASSPHRASE];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_VERIFY];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_SCREEN_OFF];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_AUTO_SHUTDOWN];
    if (mBrightnessEnable) {
        settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_BRIGHTNESS_LEVEL];
    }
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_CHANGE_PASSWD];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_COINS_SHOW_MANAGER];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_CHANGE_LANG];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_RANDOM_PIN_KEYPAD];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_BTC_MULTI_ADDRESS];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_UPDATE_FIRMWARE];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_FRESET];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_DOWNLOAD_APP];
    settings[n++] = SETTING_LABEL_CFG[SETTING_ITEM_ABOUT];

    return n;
}

