#define LOG_TAG "ActionSheet"

#include "ActionSheetDialog.h"

#include <minigui_comm.h>
#include <stdlib.h>
#include "resource.h"
#include "debug.h"
#include "key_event.h"
#include "ListView.h"
#include "common.h"
#include "NavBar.h"

#define IDC_TITLE 650
#define IDC_MSG   651
#define IDC_LV_PARENT 652
#define IDC_ITEM_BG_INDEX_0 670
#define IDC_ITEM_TXT_INDEX_0 680

#define ITEM_IMG_ID(i) (IDC_ITEM_BG_INDEX_0 + (i))
#define ITEM_TXT_ID(i) (IDC_ITEM_TXT_INDEX_0 + (i))

#define ITEM_MAX_CNT 6

typedef struct {
	int dataSelectedIndex;
	BITMAP *itemsBgImg;
	BITMAP *itemSelBgImg;
	ActionSheetDialogConfig_t *config;
	ListView *listView;
	NavBar *navBar;
	label_set_param titleSet;
	label_set_param msgSet;
	WIN_RECT screenRect;
	int navBarHeight;
	HWND lvParent; 
} ActionSheetDialogState;

static int initState(ActionSheetDialogState *state, ActionSheetDialogConfig_t *config) {
	memset(state, 0, sizeof(ActionSheetDialogState));
	state->config = config;
	state->dataSelectedIndex = config->initIndex;
	db_msg("initSelectIdx:%d", state->dataSelectedIndex);
	return 0;
}

static int getItemID(int index, int img) {
	if (img) {
		return ITEM_IMG_ID(index * 2);
	} else {
		return ITEM_TXT_ID(index * 2 + 1);
	}
}

static int initImg(HWND hDlg, int id, BITMAP *bitmap) {
	if (NULL == bitmap) {
		db_error("invalid bitmap:%p", bitmap);
		return -1;
	}
	HWND hwnd = HWND_INVALID;
	hwnd = GetDlgItem(hDlg, id);
	if (!IS_VALID_HWND(hwnd)) {
		db_msg("invlaid hwnd:%d id:%d", hwnd, id);
		return -1;
	}
	db_msg("id:%d bipmap:%p", id, bitmap);
	SendMessage(hwnd, STM_SETIMAGE, (WPARAM) bitmap, 0);
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;
}

static int updateWindowState(HWND hDlg, int id, int show) {
	HWND hwnd = HWND_INVALID;
	hwnd = GetDlgItem(hDlg, id);
	if (!IS_VALID_HWND(hwnd)) {
		db_msg("invlaid hwnd:%d id:%d", hwnd, id);
		return -1;
	}
	db_msg("id:%d show:%d", id, show);
	ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
	return 0;
}

static int createListView(HWND hDlg, ActionSheetDialogState *state) {
	ActionSheetDialogConfig_t *config = state->config;
	int icon_mk[] = {MK_ac_dlg_img_item};
	int text_mk[] = {MK_ac_dlg_label_item};
	int ret = 0;
	int y = 0;
	int h = 0;
	int maxShowItemCnt = 0;
	WIN_RECT rect;
	HWND retWnd = HWND_INVALID;
	y = state->titleSet.y + state->titleSet.h;
	if (config->msg.data) {
		y = state->msgSet.y + state->msgSet.h;
	}

	res_getRect(MK_ac_dlg_list_containter, "pos", &rect);
	db_msg("title x:%d y:%d w:%d h:%d", state->titleSet.x, state->titleSet.y, state->titleSet.w, state->titleSet.h);
	h = state->screenRect.h - y - state->navBarHeight;
	maxShowItemCnt = h / rect.h;
	if (state->config->itemsCnt <= maxShowItemCnt) {
		y = y + (h - config->itemsCnt * rect.h) / 2;
	}

	retWnd = CreateWindowEx(CTRL_STATIC, "",
	                        WS_CHILD | WS_VISIBLE,
	                        WS_EX_USEPARENTFONT,
	                        IDC_LV_PARENT,
	                        0, y, state->screenRect.w, h,
	                        hDlg, (DWORD) state);
	if (!IS_VALID_HWND(retWnd)) {
		return -1;
	}
	SetWindowBkColor(retWnd, res_getBGColor());
	state->lvParent = retWnd;
	state->listView = new ListView(retWnd, MK_ac_dlg_list_containter);
	ret = state->listView->init(config->itemsCnt, ARRAY_SIZE(icon_mk), icon_mk, ARRAY_SIZE(text_mk), text_mk);
	db_msg("init ListView ret:%d", ret);
	for (int i = 0; i < config->itemsCnt; ++i) {
		state->listView->setLabelText(i, 0, (const char *) config->items[i]);
		if (config->initIndex == i) {
			state->listView->setIconImage(i, 0, state->itemSelBgImg);
		} else {
			state->listView->setIconImage(i, 0, state->itemsBgImg);
		}
	}
	state->listView->select(config->initIndex);
	return 0;
}

static int onInit(ActionSheetDialogState *state, HWND hDlg) {
	ActionSheetDialogConfig_t *config = state->config;
	db_msg("tilte:%s font:%d style:%d", config->title.data, config->title.font, config->title.style);
	setDialogLabelStyle(hDlg, IDC_TITLE, &config->title);
	setDialogLabelStyle(hDlg, IDC_MSG, &config->msg);
	createListView(hDlg, state);
	return 0;
}

static int onKeyDown(ActionSheetDialogState *state, HWND hDlg, WPARAM code) {
	ActionSheetDialogConfig_t *config = state->config;
	int index;
	HWND hwnd;
	switch (code) {
		case INPUT_KEY_OK: {
			EndDialog(hDlg, state->dataSelectedIndex);
		}
			break;
		case INPUT_KEY_ESC:
//			EndDialog(hDlg, KEY_EVENT_ABORT);
			break;
		case INPUT_KEY_UP: {
			state->listView->setIconImage(state->dataSelectedIndex, 0, state->itemsBgImg);
			if (state->dataSelectedIndex <= 0) {
				state->dataSelectedIndex = config->itemsCnt - 1;
			} else {
				state->dataSelectedIndex--;
			}
			state->listView->select(state->dataSelectedIndex);
			state->listView->setIconImage(state->dataSelectedIndex, 0, state->itemSelBgImg);
		}
			break;
		case INPUT_KEY_DOWN: {
			state->listView->setIconImage(state->dataSelectedIndex, 0, state->itemsBgImg);
			state->dataSelectedIndex = (state->dataSelectedIndex + 1) % config->itemsCnt;
			state->listView->select(state->dataSelectedIndex);
			state->listView->setIconImage(state->dataSelectedIndex, 0, state->itemSelBgImg);
		}
			break;
		case INPUT_KEY_LEFT: {
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		case INPUT_KEY_RIGHT:
			break;
		default:
			break;
	}
	return 0;
}

static int onKeyUp(ActionSheetDialogState *state, HWND hDlg, WPARAM code) {
	return 0;
}

static PROC_RET ActionSheetDialogProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	ActionSheetDialogState *state;
	db_msg("ActionSheetDialogProc message:%d", message);
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (ActionSheetDialogState *) lParam;
			SetWindowBkColor(hDlg, res_getBGColor());
			onInit(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (ActionSheetDialogState *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (ActionSheetDialogState *) GetWindowAdditionalData(hDlg);
			onKeyUp(state, hDlg, wParam);
		}
			break;
		case MSG_PAINT: {
		}
			break;
		case MSG_DESTROY: {
		}
			break;
		default:
			break;
	}
	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void freeMemory(CTRLDATA *data, ActionSheetDialogState *state) {
	if (NULL != data) {
		free(data);
	}
	if (state->itemsBgImg) {
		UnloadBitmap(state->itemsBgImg);
		free(state->itemsBgImg);
		state->itemsBgImg = NULL;
	}
	if (state->itemSelBgImg) {
		UnloadBitmap(state->itemSelBgImg);
		free(state->itemSelBgImg);
		state->itemSelBgImg = NULL;
	}
	if (state->listView) {
		free(state->listView);
		state->listView = NULL;
	}
	if (state->listView) {
		delete state->listView;
		state->listView = NULL;
	}
	if (state->navBar) {
		delete state->navBar;
		state->navBar = NULL;
	}
	if (IS_VALID_HWND(state->lvParent)) {
		DestroyWindow(state->lvParent);
	}
}

static int loadBitmap(BITMAP *bitmap, int id, const char *key) {
	int ret = -1;
	if (NULL == bitmap || NULL == key) {
		db_error("invalid paras bitmap:%p key:%s", bitmap, key);
		return -1;
	}
	const char *confstr = res_getString2(id, key);
	if (confstr) {
		ret = LoadBitmapFromFile(HDC_SCREEN, bitmap, confstr);
		if (ret) {
			db_error("load bitmap path:%s failed", confstr);
		} else {
			db_msg("load bitmap path:%s OK", confstr);
		}
	} else {
		db_error("not found path key:%s, id:%d", key, id);
	}
	return ret;
}

static BITMAP *newMallocBitMap(void) {
	BITMAP *bitmap = NULL;
	bitmap = (BITMAP *) malloc(sizeof(BITMAP));
	if (!bitmap) {
		db_error("new bitmap memory false");
		return NULL;
	}
	memset(bitmap, 0, sizeof(BITMAP));
	return bitmap;
}

int showActionSheetDialog(HWND hParent, ActionSheetDialogConfig_t *config) {
	ActionSheetDialogState _state;
	ActionSheetDialogState *state = &_state;
	memset(state, 0, sizeof(ActionSheetDialogState));
	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	int controlNrs;
	const char *confstr;
	int retval;
	int msg_index = 0;
	int title_index = 0;
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;

	if (initState(state, config) != 0) {
		db_msg("initBackupSeedWordState false");
		return -1;
	}
	res_getPos(MK_system, &state->screenRect);
	db_msg("screen w:%d h:%d", state->screenRect.w, state->screenRect.h);
	state->navBarHeight = 0;
#if 0
	if (config->flag & ACTION_SHEET_DIALOG_FLAG_NAVI) {
		state->navBarHeight = 0;
	}
#endif
	controlNrs = 0;
	title_index = controlNrs++;
	if (config->msg.data) {
		msg_index = controlNrs++;
		db_msg("msg_index:%d", msg_index);
	}

	CtrlData = (PCTRLDATA) malloc(sizeof(CTRLDATA) * controlNrs);
	if (!CtrlData) {
		db_error("new ctrl data memory false");
		return -1;
	}
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	state->itemsBgImg = newMallocBitMap();
	db_msg("itemsBgImg:%p", state->itemsBgImg);
	if (!state->itemsBgImg) {
		freeMemory(CtrlData, state);
		return -1;
	}
	loadBitmap(state->itemsBgImg, MK_action_sheet_dialog, "item_bg_img");
	state->itemSelBgImg = newMallocBitMap();
	db_msg("itemSelBgImg:%p", state->itemSelBgImg);
	if (!state->itemSelBgImg) {
		freeMemory(CtrlData, state);
		return -1;
	}
	loadBitmap(state->itemSelBgImg, MK_action_sheet_dialog, "item_sel_bg_img");

	// title index
	res_getLabelSetParam(label_set, MK_action_sheet_dialog, "title_config");
	db_msg("action sheet dialog title:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->title.data, label_set->font, label_set->style, label_set->x, label_set->y, label_set->w, label_set->h);
	initStringCtrlData(&CtrlData[title_index], IDC_TITLE, &config->title, label_set);
	memcpy(&(state->titleSet), label_set, sizeof(label_set_param));
	if (is_empty_string(config->title.data)) { //not title
		state->titleSet.h = 0;
	}

	if (msg_index) {
		db_msg("msg_index:%d", msg_index);
		res_getLabelSetParam(label_set, MK_action_sheet_dialog, "msg_config");
		db_msg("action sheet dialog msg:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->msg.data, label_set->font, label_set->style, label_set->x, label_set->y, label_set->w, label_set->h);
		initStringCtrlData(&CtrlData[msg_index], IDC_MSG, &(config->msg), label_set);
		memcpy(&(state->msgSet), label_set, sizeof(label_set_param));
	}
	db_msg("msg_index:%d", msg_index);

	confstr = res_getString2(MK_action_sheet_dialog, "pos");
	db_msg("pos:%s", confstr);
	if (confstr) {
		sscanf(confstr, "%d %d %d %d", &(dlgTemplate.x), &(dlgTemplate.y), &(dlgTemplate.w), &(dlgTemplate.h));
	} else {
		dlgTemplate.x = 0;
		dlgTemplate.y = 0;
		res_screen_info(&dlgTemplate.w, &dlgTemplate.h);
	}

	dlgTemplate.caption = "";
	dlgTemplate.controlnr = controlNrs;
	dlgTemplate.controls = CtrlData;
	db_msg("CtrlData:%p, controlnr:%d state:%p", CtrlData, controlNrs, state);
	retval = DialogBoxIndirectParam(&dlgTemplate, hParent, ActionSheetDialogProc, (LPARAM) state);
	db_msg("ActionSheetDialog ret:%d", retval);
	freeMemory(CtrlData, state);

	return retval;
}

