#define LOG_TAG "AboutWin"

#include "AboutWin.h"
#include "minigui_comm.h"
#include "common.h"
#include "GuiMain.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include "device.h"
#include "wallet_manager.h"


enum {
	ABOUT_ITEM_WALLET_NAME,
	ABOUT_ITEM_PRODUCT_TYPE,
	ABOUT_ITEM_FIREWARE_VERSION,
	ABOUT_ITEM_SN,
	ABOUT_ITEM_ACTIVE_TIME,
	ABOUT_ITEM_ACTIVE_TIME2,
	ABOUT_ITEM_MAXID
};

enum {
	ABOUT_LABEL_TITLE = 0,
	ABOUT_LABEL_MAXID,
};

#define IDC_LIST_PARENT_CONTAINER 650

AboutWin::AboutWin() {
	mListView = NULL;
	mContainer = HWND_INVALID;
	int label_mk_map[ABOUT_LABEL_MAXID] = {
			MK_about_wallet_title,
	};
	initLayout(0, NULL, ABOUT_LABEL_MAXID, label_mk_map);
}

int AboutWin::onCreate(HWND hWnd) {
	createLayoutWidgets(hWnd);
	int text_mk[] = {MK_about_item_value};

	setLabelText(ABOUT_LABEL_TITLE, res_getLabel(LANG_LABEL_SET_ITEM_ABOUT), true);

	WIN_RECT rect;
	res_getPos(MK_about_list_parent_container, &rect);
	mContainer = createWidgetWindow(hWnd, 0, rect.x, rect.y, rect.w, rect.h, IDC_LIST_PARENT_CONTAINER, WIDGET_TYPE_NORMAL, 0, -1);
	if (IS_VALID_HWND(mContainer)) {
		SetWindowBkColor(mContainer, res_getBGColor());
		mListView = new ListView(mContainer, MK_about_wallet_container);
	}
	mListView->init(ABOUT_ITEM_MAXID, 0, NULL, ARRAY_SIZE(text_mk), text_mk);

	return 0;
}

int AboutWin::onResume() {
	char str[128] = {0};
	char device_name[32] = {0};
	const char *format = "%s:  %s";
	setLabelText(ABOUT_LABEL_TITLE, res_getLabel(LANG_LABEL_SET_ITEM_ABOUT), true);
	int atime = device_get_active_time();
	const char *actime_title = res_getLabel(LANG_LABEL_DEVICE_ACTIVE_TIME);
	int actime2 = atime > 100 && strlen(actime_title) > 15;
	for (int i = 0; i < ABOUT_ITEM_MAXID; ++i) {
		switch (i) {
			case ABOUT_ITEM_WALLET_NAME: {
				get_device_name(device_name, 32, 1);
				snprintf(str, sizeof(str), format, res_getLabel(LANG_LABEL_WALLET_NAME), device_name);
			}
				break;
			case ABOUT_ITEM_PRODUCT_TYPE: {
				snprintf(str, sizeof(str), "%s:  %s %s", res_getLabel(LANG_LABEL_PRODUCT_TYPE), PRODUCT_BRAND_VALUE, PRODUCT_NAME_VALUE);
			}
				break;
			case ABOUT_ITEM_FIREWARE_VERSION: {
				const sec_base_info *info = wallet_getBaseInfo();
				if (info) {
					snprintf(str, sizeof(str), "%s:  %s-%d", res_getLabel(LANG_LABEL_FIRMWARE_VERSION), DEVICE_APP_VERSION, info->app_version);
				} else {
					snprintf(str, sizeof(str), "%s:  %s-0", res_getLabel(LANG_LABEL_FIRMWARE_VERSION), DEVICE_APP_VERSION);
				}
			}
				break;
			case ABOUT_ITEM_SN: {
				char sn[64] = {0};
				device_get_sn(sn, sizeof(sn));
				snprintf(str, sizeof(str), "%s:  %s", res_getLabel(LANG_LABEL_DEVICE_SN), sn);
				db_msg("sn:%s str:%s", sn, str);
			}
				break;
			case ABOUT_ITEM_ACTIVE_TIME: {
				if (atime > 100) {
					snprintf(str, sizeof(str), "%s:  ", actime_title);
					if (!actime2) {
						int slen = strlen(str);
						format_time(str + slen, sizeof(str) - slen, atime, 0, 2);
					}
				} else {
					str[0] = 0;
				}
			}
				break;
			case ABOUT_ITEM_ACTIVE_TIME2:
				if (atime > 100 && actime2) {
					format_time(str, sizeof(str), atime, 0, 2);
				} else {
					str[0] = 0;
				}
				break;
			default:
				str[0] = 0;
				break;
		}
		mListView->setLabelText(i, 0, str);
	}
	mListView->select(0);

	return 0;
}

int AboutWin::onPause() {
	return 0;
}

AboutWin::~AboutWin() {
	if (mListView) {
		mListView->clean();
		delete mListView;
		mListView = NULL;
	}
}

PROC_RET AboutWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_INITDIALOG:
			break;
		default:
			break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int AboutWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d isLongPress:%d", keyCode, isLongPress);
	int selectindex;
	switch (keyCode) {
		case INPUT_KEY_LEFT:
		case INPUT_KEY_OK:
			return WINDOWID_SETTING;
			break;
		case INPUT_KEY_RIGHT:
			break;
		case INPUT_KEY_DOWN:
			break;
		case INPUT_KEY_UP:
			break;
		default:
			break;
	}
	return 0;
}

