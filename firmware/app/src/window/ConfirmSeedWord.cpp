#define LOG_TAG "ConfirmSeedWord"

#include <minigui_comm.h>
#include <stdlib.h>
#include "resource.h"
#include "debug.h"
#include "key_event.h"
#include "bip39_english.h"
#include "wallet_util.h"
#include "Dialog.h"
#include "NavBar.h"
#include "PicDialog.h"
#include "rand.h"
#include "global.h"
#include "ConfirmSeedWord.h"

#define IDC_TITLE      650
#define IDC_NUM 654
#define IDC_TIMER 655 
#define IDC_ITEM_BG_INDEX_0 670
#define IDC_ITEM_TXT_INDEX_0 680

#define ITEM_IMG_ID(i) (IDC_ITEM_BG_INDEX_0 + (i))
#define ITEM_TXT_ID(i) (IDC_ITEM_TXT_INDEX_0 + (i))

#define ITEM_SELECT_POOL_MAX 8
#define ITEM_MAX_CNT 5
#define NOT_SELECTED -1

#define TIMER_DURATION_MS 400 // unit ms

typedef enum {
	ITEM_CTRLDATA_TYPE_BG = 0,
	ITEM_CTRLDATA_TYPE_TXT, 
	ITEM_CTRLDATA_TYPE_MAX
} ITEM_CTRLDATA_TYPE;

typedef enum {
	ITEM_STATE_NORMAL = 0,
	ITEM_STATE_HIGH_LIGHT,
	ITEM_STATE_RIGHT,
	ITEM_STATE_ERR,
	ITEM_STATE_MAX
} ITEM_STATE;

typedef struct {
	int curConfirmIdx; 
	int selectedIdx; 
	uint16_t items[ITEM_MAX_CNT];
	ConfirmSeedWordConfig_t *config;
	NavBar *navBar;
	BITMAP *itemsBgImg;
	BITMAP *itemSelImg;
	BITMAP *tickImg;
	BITMAP *errImg;
	BITMAP *numImg;
	ITEM_STATE itemState;
	label_string itemTxt;
	label_string backTxt;
	HWND hDlg;
	int timer;
} ConfirmSeedWordState;

static ConfirmSeedWordState *sState = NULL;

static int initSeedWordState(ConfirmSeedWordState *state, ConfirmSeedWordConfig_t *config) {
	memset(state, 0, sizeof(ConfirmSeedWordState));
	state->config = config;
	state->backTxt.data = res_getLabel(LANG_LABEL_BACK);
	state->itemTxt.data = "";
	state->curConfirmIdx = 0;
	return 0;
}

static void killTimer(HWND hDlg, int id, ConfirmSeedWordState *state) {
	if (IsTimerInstalled(hDlg, id)) {
		KillTimer(hDlg, id);
		db_msg("killTimer");
	}
	state->timer = 0;
}

static int loadBitmap(BITMAP *bitmap, int id, const char *key) {
	int ret = -1;
	if (NULL == bitmap || NULL == key) {
		db_error("invalid paras bitmap:%p key:%s", bitmap, key);
		return -1;
	}
	const char *confstr = res_getString2(id, key);
	if (confstr) {
		ret = res_loadBmp(confstr, bitmap);
	} else {
		db_error("not found path key:%s, id:%d", key, id);
	}
	return ret;
}

static int getItemID(int index, ITEM_CTRLDATA_TYPE type) {
	switch (type) {
		case ITEM_CTRLDATA_TYPE_BG:
			return ITEM_CTRLDATA_TYPE_MAX * index;
			break;
		case ITEM_CTRLDATA_TYPE_TXT:
			return ITEM_CTRLDATA_TYPE_MAX * index + 1;
			break;
		default:
			db_error("error type:%d", ITEM_CTRLDATA_TYPE_MAX);
			return -1;
			break;
	}
}

static int updateImg(HWND hDlg, int id, BITMAP *bitmap) {
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

static int reloadData(ConfirmSeedWordState *state) {
	if (NULL == state) {
		db_error("invalid state:%p", state);
		return -1;
	}
	int i;
	ConfirmSeedWordConfig_t *config = state->config;
	db_msg("current confirm index:%d", state->curConfirmIdx);
	uint16_t cur_value = state->config->seeds[state->curConfirmIdx];
	int startPool = 0;
	uint16_t pools[ITEM_SELECT_POOL_MAX];
	if ((config->seedWordCnt - state->curConfirmIdx) > ITEM_SELECT_POOL_MAX) {
		startPool = state->curConfirmIdx;
	} else {
		startPool = config->seedWordCnt - ITEM_SELECT_POOL_MAX;
	}
	for (i = 0; i < ITEM_SELECT_POOL_MAX; i++) {
		pools[i] = state->config->seeds[startPool + i];
	}
	upset_array(pools, ITEM_SELECT_POOL_MAX);

	bool find = false;
	for (i = 0; i < ITEM_MAX_CNT; ++i) {
		state->items[i] = pools[i];
		if (pools[i] == cur_value) {
			find = true;
		}
	}
	if (!find) {
		i = random32() % ITEM_MAX_CNT;
		state->items[i] = cur_value;
	}
	state->selectedIdx = 0;
	return 0;
}

static int reloadUI(HWND hDlg, ConfirmSeedWordState *state) {
	db_msg("reloadUI hwnd:%d state:%p", hDlg, state);
	if (!IS_VALID_HWND(hDlg)) {
		db_error("invalid hwnd:%d", hDlg);
		return -1;
	}
	if (NULL == state) {
		db_error("invalid state:%p", state);
		return -1;
	}
	char str[48] = {0};
	const char *word = NULL;
	const char *confstr = NULL;
	int ret = 0;

	HWND hwnd = GetDlgItem(hDlg, IDC_TITLE);
	if (!IS_VALID_HWND(hwnd)) {
		db_error("title hwd invalid:%d", hwnd);
		return -1;
	}
	confstr = res_getString2(MK_confirm_seed_word, "num_path");
	snprintf(str, sizeof(str), "%s/%d.png", confstr, state->curConfirmIdx + 1);
	db_msg("num_path:%s", str);
	if (state->numImg) {
		UnloadBitmap(state->numImg);
	}
	if (confstr) {
		ret = res_loadBmp(str, state->numImg);
		if (ret == 0) {
			updateImg(hDlg, IDC_NUM, state->numImg);
		} else {
			db_error("load bitmap failed ret:%d", ret);
		}
	}
	for (int i = 0; i < ITEM_MAX_CNT; ++i) {
		hwnd = GetDlgItem(hDlg, getItemID(i, ITEM_CTRLDATA_TYPE_TXT));
		if (!IS_VALID_HWND(hwnd)) {
			db_error("invalid item txt hwd:%d", hwnd);
			return -1;
		}
		word = wordlist[state->items[i]];
		snprintf(str, sizeof(str), "       %s", word);
		SetWindowMText(hwnd, (const char *) str);

		if (state->selectedIdx != i) {
			updateImg(hDlg, getItemID(i, ITEM_CTRLDATA_TYPE_BG), state->itemsBgImg);
		} else {
			updateImg(hDlg, getItemID(i, ITEM_CTRLDATA_TYPE_BG), state->itemSelImg);
		}
	}
	return 0;
}

static int onInit(ConfirmSeedWordState *state, HWND hDlg) {
	ConfirmSeedWordConfig_t *config = state->config;
	db_msg("tilte:%s font:%d style:%d", config->title.data, config->title.font, config->title.style);
	setDialogLabelStyle(hDlg, IDC_TITLE, &config->title);
	updateWindowState(hDlg, IDC_TITLE, 0);
	int id = 0;
	for (int i = 0; i < ITEM_MAX_CNT; i++) {
		id = getItemID(i, ITEM_CTRLDATA_TYPE_TXT);
		setDialogLabelStyle(hDlg, id, &(state->itemTxt));
	}
	reloadData(state);
	reloadUI(hDlg, state);
	return 0;
}

static void updateItemStateImg(HWND hDlg, ConfirmSeedWordState *state) {
	HWND hwnd = GetDlgItem(hDlg, getItemID(state->selectedIdx, ITEM_CTRLDATA_TYPE_BG));
	BITMAP *bitmap = NULL;
	if (IS_VALID_HWND(hwnd)) {
		switch (state->itemState) {
			case ITEM_STATE_ERR:
				bitmap = state->errImg;
				break;
			case ITEM_STATE_RIGHT:
				bitmap = state->tickImg;
				break;
			case ITEM_STATE_HIGH_LIGHT:
				bitmap = state->itemSelImg;
				break;
			case ITEM_STATE_NORMAL:
				bitmap = state->itemsBgImg;
				break;
			default:
				db_error("unkown ITEM_STATE:%d", state->itemState);
				return;
				break;
		}
		updateImg(hDlg, getItemID(state->selectedIdx, ITEM_CTRLDATA_TYPE_BG), bitmap);
	}
}

static void freeMemory(CTRLDATA *data, ConfirmSeedWordState *state) {
	if (NULL != data) {
		free(data);
	}
	if (state->itemsBgImg) {
		free(state->itemsBgImg);
	}
	if (state->itemSelImg) {
		free(state->itemSelImg);
	}
	if (state->tickImg) {
		free(state->tickImg);
	}
	if (state->errImg) {
		free(state->errImg);
	}
	if (state->numImg) {
		free(state->numImg);
	}
	if (state->navBar) {
		delete state->navBar;
		state->navBar = NULL;
	}
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

static BOOL TimerProc(HWND hWnd, int message, DWORD wParam) {
	db_msg("hwnd:%d message：%d wParam:%lu", hWnd, message, (long unsigned int) wParam);
	ConfirmSeedWordConfig_t *config = sState->config;
	ConfirmSeedWordState *state = sState;
	if (message == IDC_TIMER) {
		if (state == NULL) {
			db_error("sState is NULL");
		} else {
			if (!state->timer) {
				return TRUE;
			}
			killTimer(state->hDlg, IDC_TIMER, state);
			if (config->seeds[state->curConfirmIdx] == state->items[state->selectedIdx]) {
				state->curConfirmIdx++;
				db_msg("current confirm index:%d", state->curConfirmIdx);
				if (state->curConfirmIdx == state->config->seedWordCnt) {
					EndDialog(state->hDlg, 0);
				} else {
					reloadData(state);
					reloadUI(state->hDlg, state);
				}
			} else {
				reloadData(state);
				reloadUI(state->hDlg, state);
			}
		}
	}
	return TRUE;
}

static int onKeyDown(ConfirmSeedWordState *state, HWND hDlg, WPARAM code) {
	ConfirmSeedWordConfig_t *config = state->config;
	int ret = 0;
	switch (code) {
		case INPUT_KEY_OK: {
			db_msg("INPUT_KEY_OK");
			if (!state->timer) {
				if (config->seeds[state->curConfirmIdx] == state->items[state->selectedIdx]) {
					state->itemState = ITEM_STATE_RIGHT;
					updateItemStateImg(hDlg, state);
				} else {
					state->itemState = ITEM_STATE_ERR;
					updateItemStateImg(hDlg, state);
				}
				db_msg("start Timer hDlg:%d IDC_TIMER:%d", hDlg, IDC_TIMER);
				ret = SetTimerEx(hDlg, IDC_TIMER, TIMER_DURATION_MS / 10, TimerProc);
				state->timer = 1;
				db_msg("end Timer ret:%d", ret);
			}
		}
			break;
		case INPUT_KEY_ESC:
			break;
		case INPUT_KEY_LEFT: {
			if (state->timer) {
				KillTimer(hDlg, IDC_TIMER);
				state->timer = 0;
			}
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		case INPUT_KEY_RIGHT:
			break;
		case INPUT_KEY_UP:
		case INPUT_KEY_DOWN: {
			if (!state->timer) {
				updateItemStateImg(hDlg, state);
				updateImg(hDlg, getItemID(state->selectedIdx, ITEM_CTRLDATA_TYPE_BG), state->itemsBgImg);
				if (code == INPUT_KEY_UP) {
					state->selectedIdx = state->selectedIdx > 0 ? (state->selectedIdx - 1) : (ITEM_MAX_CNT - 1);
				} else {
					state->selectedIdx = state->selectedIdx < (ITEM_MAX_CNT - 1) ? (state->selectedIdx + 1) : 0;
				}
				updateImg(hDlg, getItemID(state->selectedIdx, ITEM_CTRLDATA_TYPE_BG), state->itemSelImg);
			}
//			killTimer(hDlg, IDC_TIMER, state);
		}
			break;
		default:
			break;
	}
	return 0;
}

static int onKeyUp(ConfirmSeedWordState *state, HWND hDlg, WPARAM code) {
	return 0;
}

static PROC_RET ConfirmSeedWordProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	ConfirmSeedWordState *state;
	db_msg("message:%d", message);
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (ConfirmSeedWordState *) lParam;
			state->hDlg = hDlg;
			SetWindowBkColor(hDlg, res_getBGColor());
			onInit(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (ConfirmSeedWordState *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (ConfirmSeedWordState *) GetWindowAdditionalData(hDlg);
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

int showConfirmSeedWord(HWND hParent, ConfirmSeedWordConfig_t *config) {
	db_msg("showConfirmSeedWord hParent:%d config:%p", hParent, config);
	ConfirmSeedWordState _state;
	ConfirmSeedWordState *state = &_state;
	sState = state;

	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	unsigned int controlNrs;

	const char *confstr;
	int retval;
	int title_index = 0;
	int num_img_index = 0;
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;
	label_set_param title_set;
	WIN_RECT screenRect;
	res_getPos(MK_system, &screenRect);
	db_msg("screen w:%d h:%d", screenRect.w, screenRect.h);

	if (initSeedWordState(state, config) != 0) {
		db_msg("initSeedWordState false");
		return -1;
	}
	controlNrs = 0;
	controlNrs = controlNrs + ITEM_MAX_CNT * ITEM_CTRLDATA_TYPE_MAX; //image & labe & state
	title_index = controlNrs++;
	num_img_index = controlNrs++;
	CtrlData = (PCTRLDATA) malloc(sizeof(CTRLDATA) * controlNrs);
	if (!CtrlData) {
		db_error("new ctrl data memory false");
		return -1;
	}
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	db_msg("new ctr data");
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	if (num_img_index) {
		state->numImg = newMallocBitMap();
		if (!state->numImg) {
			freeMemory(CtrlData, state);
			return -1;
		}
	}
	if (ITEM_MAX_CNT) {
		state->itemsBgImg = newMallocBitMap();
		if (!state->itemsBgImg) {
			freeMemory(CtrlData, state);
			return -1;
		}
		loadBitmap(state->itemsBgImg, MK_confirm_seed_word, "item_bg_img");

		state->itemSelImg = newMallocBitMap();
		if (!state->itemSelImg) {
			freeMemory(CtrlData, state);
			return -1;
		}
		loadBitmap(state->itemSelImg, MK_confirm_seed_word, "item_bg_sel_img");

		state->tickImg = newMallocBitMap();
		if (!state->tickImg) {
			freeMemory(CtrlData, state);
			return -1;
		}
		loadBitmap(state->tickImg, MK_confirm_seed_word, "item_state_right_img");
		state->errImg = newMallocBitMap();
		if (!state->errImg) {
			freeMemory(CtrlData, state);
			return -1;
		}
		loadBitmap(state->errImg, MK_confirm_seed_word, "item_state_wrong_img");
	}

	db_msg("bitmap init finish");
	if (title_index) {
		res_getLabelSetParam(&title_set, MK_confirm_seed_word, "title_config");
		db_msg("confirm seed word title:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->title.data, title_set.font, title_set.style, title_set.x, title_set.y, title_set.w, title_set.h);
		initStringCtrlData(&CtrlData[title_index], IDC_TITLE, &config->title, &title_set);
	}
	WIN_RECT numImgRect;
	res_getRect(MK_confirm_seed_word, "num_img_pos", &numImgRect);
	if (num_img_index) {
		confstr = res_getString2(MK_confirm_seed_word, "num_img_pos");
		db_msg("num_img_pos:%s", confstr);
		initImageCtrlData(&CtrlData[num_img_index], IDC_NUM, confstr);
	}

	WIN_RECT rect;
	WIN_RECT stateIconRect;
	int itemSpace = 6;
	int id = 0;
	int offsetY = numImgRect.y + numImgRect.h;
	int totalItemHeight = 0;
	if (ITEM_MAX_CNT) {
		db_msg("layout item");
		itemSpace = res_getInt(MK_confirm_seed_word, "item_space", itemSpace);
		res_getRect(MK_confirm_seed_word, "item_state_pos", &stateIconRect);
		res_getRect(MK_confirm_seed_word, "item_frame", &rect);
		totalItemHeight = (ITEM_MAX_CNT - 1) * itemSpace + ITEM_MAX_CNT * rect.h;
		offsetY = offsetY + ((screenRect.h - offsetY) - totalItemHeight) / 2;
		db_msg("item_frame x:%d y:%d w:%d h:%d item_space:%d state_width:%d", rect.x, rect.y, rect.w, rect.h, itemSpace, stateIconRect.w);
		res_getLabelSetParam(label_set, MK_confirm_seed_word, "item_text_config");

		for (int i = 0; i < ITEM_MAX_CNT; ++i) {
			rect.y = offsetY + i * (itemSpace + rect.h);
			id = getItemID(i, ITEM_CTRLDATA_TYPE_BG);
			initImageCtrlDataWithRect(&CtrlData[id], id, rect);

			id = getItemID(i, ITEM_CTRLDATA_TYPE_TXT);
			initStringCtrlData(&CtrlData[id], id, &(state->itemTxt), label_set);
			rect.y -= 3;
			updateCtrlDataRect(&CtrlData[id], rect);
			db_msg("item font:%d style:%d", state->itemTxt.font, state->itemTxt.style);
		}
	}

	confstr = res_getString2(MK_confirm_seed_word, "pos");
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
	int pkeyesc = use_powerkey_as_esc(1);
	retval = DialogBoxIndirectParam(&dlgTemplate, hParent, ConfirmSeedWordProc, (LPARAM) state);
	use_powerkey_as_esc(pkeyesc);
	db_msg("confirm seed word ret:%d", retval);
	sState = NULL;
	if (state->itemsBgImg) {
		UnloadBitmap(state->itemsBgImg);
	}
	if (state->itemSelImg) {
		UnloadBitmap(state->itemSelImg);
	}
	if (state->tickImg) {
		UnloadBitmap(state->tickImg);
	}
	if (state->errImg) {
		UnloadBitmap(state->errImg);
	}
	if (state->numImg) {
		UnloadBitmap(state->numImg);
	}
	freeMemory(CtrlData, state);
	db_msg("freeMemory");
	return retval;
}