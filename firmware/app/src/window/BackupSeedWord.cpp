#define LOG_TAG "BackupSeedWord"

#include "BackupSeedWord.h"

#include <minigui_comm.h>
#include <stdlib.h>
#include "resource.h"
#include "debug.h"
#include "key_event.h"
#include "bip39_english.h"
#include "NavBar.h"
#include "global.h"

#define IDC_TITLE      650
#define IDC_ITEM_BG_INDEX_0 670
#define IDC_ITEM_TXT_INDEX_0 680

#define ITEM_IMG_ID(i) (IDC_ITEM_BG_INDEX_0 + (i))
#define ITEM_TXT_ID(i) (IDC_ITEM_TXT_INDEX_0 + (i))

#define ITEM_MAX_CNT 4

#define ITEM_PREFIX_TXT  "     %d"

typedef struct {
	int page_index;
	BackupSeedWordConfig_t *config;
	BITMAP *itemsBgImg;
	NavBar *navBar;
	label_string prePageTxt;
	label_string nextPageTxt;
	label_set_param item_label_set;
} BackupSeedWordState;

static int createNavBar(HWND hDlg, BackupSeedWordState *state) {
	NavBar *nav = new NavBar(hDlg);
	int op = 0;
	op = NAV_BAR_ITEM_SHOW_UP_DOWN;
	nav->init(state->prePageTxt.data, state->nextPageTxt.data, NULL, op, 0);
	state->navBar = nav;
	return 0;
}

static int initBackupSeedWordState(BackupSeedWordState *state, BackupSeedWordConfig_t *config) {
	memset(state, 0, sizeof(BackupSeedWordState));
	state->config = config;
	state->config->title.data = "";
//	txt = (char *)rm->getLabel(LANG_LABEL_NEXT_PAGE);
//	db_msg("next page:%s", txt);
	state->nextPageTxt.data = NULL;
	state->prePageTxt.data = res_getLabel(LANG_LABEL_BACK);
	return 0;
}

static int getTotalPageCnt(BackupSeedWordState *state) {
	if (NULL == state) {
		db_error("invalid state:%p", state);
		return 0;
	}
	if (state->config->seedWordCnt % ITEM_MAX_CNT) {
		return state->config->seedWordCnt / ITEM_MAX_CNT + 1;
	} else {
		return state->config->seedWordCnt / ITEM_MAX_CNT;
	}
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

static int updateData(HWND hDlg, BackupSeedWordState *state) {
	if (!IS_VALID_HWND(hDlg)) {
		db_error("invalid hwnd:%d", hDlg);
		return -1;
	}
	if (NULL == state) {
		db_error("invalid state:%p", state);
		return -1;
	}

	uint16_t start = 0;
	uint16_t end = 0;
	char str[32] = {0};
	const char *word = NULL;
	int totalPage = getTotalPageCnt(state);
	int options = 0;
	db_msg("current page index:%d", state->page_index);

//	options = NAV_BAR_ITEM_SHOW_BACK;
	options = 0;
	if (state->page_index == (totalPage - 1)) {
		options |= NAV_BAR_ITEM_SHOW_OK;
	} else if (state->page_index == 0) {
		options |= NAV_BAR_ITEM_SHOW_DOWN;
	} else {
		options |= NAV_BAR_ITEM_SHOW_UP_DOWN;
	}
	state->navBar->updateState(options);

	start = state->page_index * ITEM_MAX_CNT;
	if (state->config->seedWordCnt - start >= ITEM_MAX_CNT) {
		end = start + ITEM_MAX_CNT - 1;
	} else {
		end = state->config->seedWordCnt - 1;
	}
	HWND hwnd = GetDlgItem(hDlg, IDC_TITLE);
	if (!IS_VALID_HWND(hwnd)) {
		db_error("title hwd invalid:%d", hwnd);
		return -1;
	}
	SetWindowMText(hwnd, res_getLabel(LANG_LABEL_WRITE_DOWN_SEED_TITLE));
	for (int i = 0; i < ITEM_MAX_CNT; ++i) {
		if (start + i >= state->config->seedWordCnt) {
			hwnd = GetDlgItem(hDlg, getItemID(i, 1));
			ShowWindow(hwnd, SW_HIDE);
			hwnd = GetDlgItem(hDlg, getItemID(i, 0));
			ShowWindow(hwnd, SW_HIDE);
		} else {
			hwnd = GetDlgItem(hDlg, getItemID(i, 1));
			ShowWindow(hwnd, SW_SHOW);
			hwnd = GetDlgItem(hDlg, getItemID(i, 0));
			ShowWindow(hwnd, SW_SHOW);
			if (!IS_VALID_HWND(hwnd)) {
				db_error("invalid item txt hwd:%d", hwnd);
				return -1;
			}
			word = wordlist[state->config->seeds[start + i]];

			if (start >= 10 || end >= 10) {
				if ((start + 1 + i) >= 10) {
					snprintf(str, sizeof(str), ITEM_PREFIX_TXT"    %s", start + 1 + i, word);
				} else {
					snprintf(str, sizeof(str), ITEM_PREFIX_TXT"      %s", start + 1 + i, word);
				}
			} else {
				snprintf(str, sizeof(str), ITEM_PREFIX_TXT"      %s", start + 1 + i, word);
			}
			db_secure("show word:%s", str);
			SetWindowMText(hwnd, str);
		}
	}

	return 0;
}

static int onInit(BackupSeedWordState *state, HWND hDlg) {
	BackupSeedWordConfig_t *config = state->config;
	db_msg("tilte:%s font:%d style:%d", config->title.data, config->title.font, config->title.style);
	HWND hwnd = HWND_INVALID;
	setDialogLabelStyle(hDlg, IDC_TITLE, &config->title);
	createNavBar(hDlg, state);

	label_string label_str;
	label_str.style = state->item_label_set.style;
	label_str.font = state->item_label_set.font;
	label_str.data = "";
	for (int i = 0; i < ITEM_MAX_CNT; i++) {
		setDialogLabelStyle(hDlg, getItemID(i, 0), &label_str);
		hwnd = GetDlgItem(hDlg, getItemID(i, 0));
		if (!IS_VALID_HWND(hwnd)) {
			db_error("invalid hwnd %d", hwnd);
			return -1;
		}
		ShowWindow(hwnd, SW_SHOW);
		hwnd = GetDlgItem(hDlg, getItemID(i, 1));
		if (!IS_VALID_HWND(hwnd)) {
			db_error("invalid item img:%d", hwnd);
			return -1;
		}
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) state->itemsBgImg, 0);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	updateData(hDlg, state);

	return 0;
}

static int onKeyDown(BackupSeedWordState *state, HWND hDlg, WPARAM code) {
	int totalPageCnt = getTotalPageCnt(state);
	switch (code) {
		case INPUT_KEY_OK: {
			if (state->page_index == (totalPageCnt - 1)) {
				EndDialog(hDlg, 0);
			}
		}
			break;
		case INPUT_KEY_ESC:
			break;
		case INPUT_KEY_UP: {
			if (state->page_index > 0) {
				state->page_index--;
				updateData(hDlg, state);
			}
			break;
		}
		case INPUT_KEY_LEFT: {
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		case INPUT_KEY_DOWN: {
			if (state->page_index < (totalPageCnt - 1)) {
				state->page_index++;
				updateData(hDlg, state);
			}
		}
			break;
		case INPUT_KEY_RIGHT:
			break;
		default:
			break;
	}
	return 0;
}

static int onKeyUp(BackupSeedWordState *state, HWND hDlg, WPARAM code) {
	return 0;
}

static PROC_RET BackupSeedWordProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	BackupSeedWordState *state;
	db_msg("message:%d", message);
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (BackupSeedWordState *) lParam;
			SetWindowBkColor(hDlg, res_getBGColor());
			onInit(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (BackupSeedWordState *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (BackupSeedWordState *) GetWindowAdditionalData(hDlg);
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

static void freeMemory(CTRLDATA *data, BackupSeedWordState *state) {
	if (NULL != data) {
		free(data);
	}
	if (state->navBar) {
		delete state->navBar;
		state->navBar = NULL;
	}
	if (state->itemsBgImg) {
		free(state->itemsBgImg);
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
		ret = res_loadBmp(confstr, bitmap);
	} else {
		db_error("not found path key:%s, id:%d", key, id);
	}
	return ret;
}

int showBackupSeedWord(HWND hParent, BackupSeedWordConfig_t *config) {
	//db_msg("showBackupSeedWord hParent:%d config:%p", hParent, config);
	BackupSeedWordState _state;
	BackupSeedWordState *state = &_state;
	memset(state, 0, sizeof(BackupSeedWordState));

	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	unsigned int controlNrs;

	const char *confstr;
	int retval;
	int title_index = 0;
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;
	WIN_RECT screenRect;

	res_getPos(MK_system, &screenRect);
	db_msg("screen w:%d h:%d", screenRect.w, screenRect.h);

	if (initBackupSeedWordState(state, config) != 0) {
		db_msg("initBackupSeedWordState false");
		return -1;
	}

	controlNrs = 0;
	controlNrs = controlNrs + ITEM_MAX_CNT * 2; //image & labe
	title_index = controlNrs++;
	CtrlData = (PCTRLDATA) malloc(sizeof(CTRLDATA) * controlNrs);
	if (!CtrlData) {
		db_error("new ctrl data memory false");
		return -1;
	}
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	if (ITEM_MAX_CNT) {
		state->itemsBgImg = newMallocBitMap();
		if (!state->itemsBgImg) {
			db_error("new itemsBgImg memory false");
			freeMemory(CtrlData, state);
			return -1;
		}
		loadBitmap(state->itemsBgImg, MK_backup_seed_word, "item_bg_img");
	}
	if (title_index) {
		res_getLabelSetParam(label_set, MK_backup_seed_word, "title_config");
		db_msg("backup seed word title:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->title.data, label_set->font, label_set->style, label_set->x, label_set->y, label_set->w, label_set->h);
		initStringCtrlData(&CtrlData[title_index], IDC_TITLE, &config->title, label_set);
	}

	WIN_RECT rect;
	int itemSpace = 6;
	int firstItemOffsetY = 30;
	if (ITEM_MAX_CNT) {
		itemSpace = res_getInt(MK_backup_seed_word, "item_space", itemSpace);
		res_getRect(MK_backup_seed_word, "item_frame", &rect);
		db_msg("item_frame x:%d y:%d w:%d h:%d item_space:%d", rect.x, rect.y, rect.w, rect.h, itemSpace);
		firstItemOffsetY = (screenRect.h - (rect.h * ITEM_MAX_CNT + (ITEM_MAX_CNT - 1) * itemSpace)) / 2 + 6;
		for (int i = 0; i < ITEM_MAX_CNT; ++i) {
			rect.y = firstItemOffsetY + i * (itemSpace + rect.h);
			initImageCtrlDataWithRect(&CtrlData[2 * i], getItemID(i, 1), rect);

			res_getLabelSetParam(&(state->item_label_set), MK_backup_seed_word, "item_text_config");

			initStringCtrlData(&CtrlData[2 * i + 1], getItemID(i, 0), NULL, &(state->item_label_set));
			rect.y = rect.y - 2;
			updateCtrlDataRect(&CtrlData[2 * i + 1], rect);
		}
	}

	confstr = res_getString2(MK_backup_seed_word, "pos");
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
	retval = DialogBoxIndirectParam(&dlgTemplate, hParent, BackupSeedWordProc, (LPARAM) state);
	db_msg("backup seed word ret:%d", retval);

	if (state->itemsBgImg) {
		UnloadBitmap(state->itemsBgImg);
		free(state->itemsBgImg);
	}
	free(CtrlData);
	return retval;
}

