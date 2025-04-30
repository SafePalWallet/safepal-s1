#define LOG_TAG "PicDialog"

#include "PicDialog.h"
#include "minigui_comm.h"
#include "debug.h"
#include "widgets.h"
#include "resource.h"
#include "key_event.h"
#include "global.h"

#define IDC_PIC        651
#define IDC_BTN        700

typedef struct {
	PicDialogConfig_t *config;
	HWND picHwnd;
	HWND *btnHwnds;
	BITMAP **picImgs;
	BITMAP btnNormalImg;
	BITMAP btnHighlighImg;
	int curIndex;
	int curPicIndex;
	int curBtnIndex;
	int showBtn;
} PicDiaglogState_t;

static int loadImgWithState(PicDiaglogState_t *state, BITMAP *bitmap, int index) {
	if (NULL == bitmap || NULL == state) {
		db_error("invalid bitmap:%p state:%p", bitmap, state);
		return -1;
	}
	PicDialogConfig_t *config = state->config;
	char path[128] = {0};
	const char *lang = settings_get_lang_suffix();
	snprintf(path, sizeof(path), "%s/img/%s/%s%d.png", get_system_res_point(), lang, config->name, index);
	db_msg("img path:%s", path);
	if (access(path, F_OK) != 0) {
		int code = -1;
		if (strcmp(lang, "en") != 0) {
			snprintf(path, sizeof(path), "%s/img/%s/%s%d.png", get_system_res_point(), "en", config->name, index);
			if (access(path, F_OK) == 0) {
				db_msg("use en img path:%s", path);
				code = 0;
			}
		}
		if (code) {
			db_msg("img path:%s not exist", path);
			return code;
		}
	}
	res_loadBmp(path, bitmap);
	return 0;
}

static int showBtn(PicDiaglogState_t *state, int show, int forceRefresh) {
	if (NULL == state) {
		db_msg("invalid state:%p", state);
		return -1;
	}
	db_msg("show btn:%d", show);
	HWND hwnd = HWND_INVALID;
	PicDialogConfig_t *config = state->config;
	if (!state->config->buttonCount) {
		db_msg("no button");
		return 0;
	}
	if (!forceRefresh && (state->showBtn == show)) {
		return 0;
	}

	for (int i = 0; i < config->buttonCount; ++i) {
		hwnd = state->btnHwnds[2 * i];
		db_msg("btn bg hwnd:%d", hwnd);
		ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
		InvalidateRect(hwnd, NULL, TRUE);

		hwnd = state->btnHwnds[2 * i + 1];
		db_msg("btn text hwnd:%d", hwnd);
		ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	state->showBtn = show;

	return 0;
}

static int updatePictureUI(HWND hDlg, PicDiaglogState_t *state, int oldIndex, int newIndex) {
	if (NULL == state) {
		db_msg("invalid state:%p", state);
		return -1;
	}
	if (oldIndex == newIndex) {
		return 0;
	}

	HWND hwnd = HWND_INVALID;
	BITMAP *bitmap = NULL;

	hwnd = state->picHwnd;
	bitmap = state->picImgs[newIndex];
	if (!bitmap) {
		bitmap = (BITMAP *) malloc(sizeof(BITMAP));
		if (bitmap) {
			memset(bitmap, 0, sizeof(BITMAP));
			loadImgWithState(state, bitmap, newIndex);
		}
		state->picImgs[newIndex] = bitmap;
	}
	SendMessage(hwnd, STM_SETIMAGE, (WPARAM) bitmap, 0);
	InvalidateRect(hwnd, NULL, TRUE);
	state->curPicIndex = newIndex;

	return 0;
}

static int updateBtnUI(HWND hDlg, PicDiaglogState_t *state, int oldIndex, int newIndex) {
	if (NULL == state) {
		db_msg("invalid state:%p", state);
		return -1;
	}
	if (!state->config->buttonCount) {
		return 0;
	}

	HWND hwnd = HWND_INVALID;
	BITMAP *bitmap = NULL;
	PicDialogConfig_t *config = state->config;
	if (newIndex == oldIndex) {
		return 0;
	}

	hwnd = state->btnHwnds[2 * newIndex];
	SendMessage(hwnd, STM_SETIMAGE, (WPARAM)(&state->btnHighlighImg), 0);
	InvalidateRect(hwnd, NULL, TRUE);

	hwnd = state->btnHwnds[2 * oldIndex];
	SendMessage(hwnd, STM_SETIMAGE, (WPARAM)(&state->btnNormalImg), 0);
	InvalidateRect(hwnd, NULL, TRUE);

	state->curBtnIndex = newIndex;

	return 0;
}

static void show_dync_text_cb(HWND hParent, const PicDialogConfig_t *config) {
	label_set_param set;
	res_getLabelSetParam(&set, MK_pic_dialog, config->dync_label);
	HWND hwnd = createWidgetWindow(hParent, 0, set.x, set.y, set.w, set.h, 0, WIDGET_TYPE_TEXT, set.style, set.font);
	SetWindowMText(hwnd, config->dync_text);
}

static int onInitDialog(PicDiaglogState_t *state, HWND hDlg) {
	PicDialogConfig_t *config = state->config;
	db_msg("onInitDialog state:%p config:%p initIndex:%d, initBtnIndex:%d", state, config, config->initIndex, config->initBtnIndex);
	HWND hwnd = HWND_INVALID;
	BITMAP *bitmap = NULL;
	label_set_param label_set;
	WIN_RECT screenRect;
	res_getPos(MK_system, &screenRect);
	res_getLabelSetParam(&label_set, MK_pic_dialog, "btn_config");
	db_msg("btn config style:%d font:%d", label_set.style, label_set.font);
	int y = 0;
	int space = res_getInt(MK_pic_dialog, "btn_space", 0);
	int bottomOffset = res_getInt(MK_pic_dialog, "bottom_offset", 0);
	int offsetY = screenRect.h - (label_set.h * config->buttonCount + (config->buttonCount - 1) * space + bottomOffset);


	hwnd = createWidgetWindow(hDlg, 0, screenRect.x, screenRect.y, screenRect.w, screenRect.h, IDC_PIC, WIDGET_TYPE_IMG, 0, 0);
	if (IS_VALID_HWND(hwnd)) {
		state->picHwnd = hwnd;
		db_msg("pic imgs:%p, init_index:%d", state->picImgs, config->initIndex);
		bitmap = state->picImgs[config->initIndex];
		db_msg("bitmap:%p", bitmap);
		if (!bitmap) {
			bitmap = (BITMAP *) malloc(sizeof(BITMAP));
			if (bitmap) {
				memset(bitmap, 0, sizeof(BITMAP));
				loadImgWithState(state, bitmap, config->initIndex);
			}
			state->picImgs[config->initIndex] = bitmap;
		}
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM)(bitmap), 0);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	int font=-1,pos=0;
	for (int i = 0; i < config->buttonCount; ++i) {
		y = offsetY + (label_set.h + space) * i;
		hwnd = createWidgetWindow(hDlg, 0, label_set.x, y, label_set.w, label_set.h, IDC_BTN + 2 * i, WIDGET_TYPE_IMG, 0, 0);
		state->btnHwnds[2 * i] = hwnd;

		if (state->curBtnIndex == i) {
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) & (state->btnHighlighImg), 0);
		} else {
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) & (state->btnNormalImg), 0);
		}
		InvalidateRect(hwnd, NULL, TRUE);

		font = strGetFont(config->btnTitles[i], &pos);
		if((font==FONT_18) || (font==FONT_16)|| (font==FONT_14) || (font==FONT_12)){
			label_set.font = font;
			db_msg("font=%d,label_set.x=%d,y=%d,label_set.w=%d,label_set.h=%d",font,label_set.x,y,label_set.w,label_set.h);
			//label_set.x=15,y=191,label_set.w=210,label_set.h=34
			switch(font){
				case FONT_18:
					y = y + (label_set.h/2-18/2) - 5;
					break;
				case FONT_16:
					y = y + (label_set.h/2-16/2) - 5;
					break;
				case FONT_14:
					y = y + (label_set.h/2-14/2) - 5;
					break;
				case FONT_12:
					y = y + (label_set.h/2-12/2) - 5;
					break;
				default:
					//y = y + 3;
					break;
			}
		}
		hwnd = createWidgetWindow(hDlg, 0, label_set.x, y, label_set.w, label_set.h, IDC_BTN + 2 * i + 1, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
		state->btnHwnds[2 * i + 1] = hwnd;
		SetWindowMText(hwnd, config->btnTitles[i]);
	}

	showBtn(state, state->curPicIndex == (config->total - 1), 1);

	if (config->dync_label && config->dync_text) {
		show_dync_text_cb(hDlg, config);
	}
	if (config->onInitCallback) {
		config->onInitCallback(hDlg);
	}
	return 0;
}

static int initState(PicDialogConfig_t *config, PicDiaglogState_t *state) {
	if (NULL == state || NULL == config) {
		db_error("invalid paras config:%p state:%p", config, state);
		return -1;
	}
	const char *confstr = NULL;
	state->curPicIndex = config->initIndex;
	state->curBtnIndex = config->initBtnIndex;
	state->config = config;

	if (config->buttonCount) {
		state->btnHwnds = (HWND *) malloc(sizeof(HWND) * 2 * config->buttonCount);
		if (NULL == state->btnHwnds) {
			db_error("new memroy btn hwnds failed");
			return -1;
		}
		memset(state->btnHwnds, 0, sizeof(HWND) * config->buttonCount);
	}
	state->picImgs = (BITMAP **) malloc(sizeof(BITMAP * ) * config->total);
	if (!state->picImgs) {
		if (state->btnHwnds) {
			free(state->btnHwnds);
		}
		db_error("new memory bitmap failed");
		return -1;
	}
	memset(state->picImgs, 0, sizeof(BITMAP * ) * config->total);

	confstr = res_getString2(MK_system_icon, "btn_long_sel_bg_img");
	if (confstr) {
		res_loadBmp(confstr, &(state->btnHighlighImg));
	}
	confstr = res_getString2(MK_system_icon, "btn_long_bg_img");
	if (confstr) {
		res_loadBmp(confstr, &(state->btnNormalImg));
	}
	return 0;
}

static int onKeyDown(PicDiaglogState_t *state, HWND hDlg, WPARAM code) {
	PicDialogConfig_t *config = state->config;
	int index;
	HWND hwnd;
	int newPicIndex = state->curPicIndex;
	int newBtnIndex = state->curBtnIndex;
	switch (code) {
		case INPUT_KEY_OK: {
			if (state->curPicIndex == (config->total - 1)) {
				db_msg("INPUT_KEY_OK end dialog");
				EndDialog(hDlg, state->curBtnIndex);
			}
		}
			break;
		case INPUT_KEY_ESC:
			break;
		case INPUT_KEY_LEFT: {
			if (!(state->config->flag & PIC_DLG_FLAG_LEFT_NOT_BACK)) {
				db_msg("INPUT_KEY_LEFT end dialog");
				EndDialog(hDlg, KEY_EVENT_BACK);
			}
		}
			break;
		case INPUT_KEY_RIGHT: {
			if (state->config->flag & PIC_DLG_FLAG_RIGHT_AS_OK) {
				db_msg("INPUT_KEY_RIGHT as OK");
				EndDialog(hDlg, state->curBtnIndex);
			}
			if (!config->buttonCount) {
				db_msg("INPUT_KEY_RIGHT end dialog");
				EndDialog(hDlg, KEY_EVENT_NEXT);
			}
		}
			break;
		case INPUT_KEY_UP: {
			if (config->total == 1) {
				if (!config->buttonCount) {
					db_msg("INPUT_KEY_UP end dialog");
					EndDialog(hDlg, KEY_EVENT_UP);
					return 0;
				}
				if (state->curBtnIndex > 0) {
					newBtnIndex = state->curBtnIndex - 1;
				} else {
					newBtnIndex = config->buttonCount - 1;
				}
			} else {
				if (state->curPicIndex == (config->total - 1)) {
					if (state->curBtnIndex > 0) {
						newBtnIndex = state->curBtnIndex - 1;
					} else {
						newPicIndex = (state->curPicIndex > 0) ? (state->curPicIndex - 1) : state->curPicIndex;
					}
				} else if (state->curPicIndex > 0) {
					newPicIndex = state->curPicIndex - 1;
				}
			}

			db_msg("up pic old:%d new:%d btn old:%d new:%d", state->curPicIndex, newPicIndex, state->curBtnIndex, newBtnIndex);
			updatePictureUI(hDlg, state, state->curPicIndex, newPicIndex);
			updateBtnUI(hDlg, state, state->curBtnIndex, newBtnIndex);
			showBtn(state, newPicIndex == (config->total - 1), 0);
		}
			break;
		case INPUT_KEY_DOWN: {
			if (config->total == 1) {
				if (!config->buttonCount) {
					db_msg("KEY_EVENT_DOWN end dialog");
					EndDialog(hDlg, KEY_EVENT_DOWN);
					return 0;
				}
				newBtnIndex = (state->curBtnIndex + 1) % config->buttonCount;
			} else {
				if (state->curPicIndex == (config->total - 1)) {
					if (state->curBtnIndex < (config->buttonCount - 1)) {
						newBtnIndex = state->curBtnIndex + 1;
					}
				} else if (state->curPicIndex < (config->total - 1)) {
					newPicIndex = state->curPicIndex + 1;
				}
			}
			db_msg("down pic old:%d new:%d btn old:%d new:%d", state->curPicIndex, newPicIndex, state->curBtnIndex, newBtnIndex);
			updatePictureUI(hDlg, state, state->curPicIndex, newPicIndex);
			updateBtnUI(hDlg, state, state->curBtnIndex, newBtnIndex);
			showBtn(state, newPicIndex == (config->total - 1), 0);
		}
			break;
		default:
			break;
	}
	return 0;
}

static int onKeyUp(PicDiaglogState_t *state, HWND hDlg, WPARAM code) {
	return 0;
}

static PROC_RET DialogProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	PicDiaglogState_t *state;
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (PicDiaglogState_t *) lParam;
			SetWindowBkColor(hDlg, res_getBGColor());
			db_msg("MSG_INITDIALOG state:%p", state);
			onInitDialog(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (PicDiaglogState_t *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (PicDiaglogState_t *) GetWindowAdditionalData(hDlg);
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

static int freeMemory(PicDiaglogState_t *state) {
	if (IS_VALID_HWND(state->picHwnd)) {
		db_msg("free picHwnd");
		DestroyWindow(state->picHwnd);
		state->picHwnd = HWND_INVALID;
	}
	if (state->btnHwnds) {
		for (int i = 0; i < state->config->buttonCount; ++i) {
			HWND hwnd = state->btnHwnds[2 * i];
			DestroyWindow(hwnd);
			hwnd = state->btnHwnds[2 * i + 1];
			DestroyWindow(hwnd);
		}
		free(state->btnHwnds);
	}
	if (state->btnHighlighImg.bmBits) {
		db_msg("free btnHighlighImg");
		UnloadBitmap(&(state->btnHighlighImg));
	}
	if (state->btnNormalImg.bmBits) {
		db_msg("free btnNormalImg");
		UnloadBitmap(&(state->btnNormalImg));
	}

	BITMAP *bitmap = NULL;
	if (state->picImgs) {
		db_msg("free pic imgs total:%d", state->config->total);
		for (int j = 0; j < state->config->total; ++j) {
			bitmap = state->picImgs[j];
			if (bitmap && bitmap->bmBits) {
				UnloadBitmap(bitmap);
			}
			if (bitmap) {
				free(bitmap);
			}
		}
		free(state->picImgs);
	}
	db_msg("end");
	return 0;
}

int showPicDialog(HWND hParent, PicDialogConfig_t *config) {
	if (NULL == config) {
		db_error("invalid config:%p", config);
		return -1;
	}

	if (config->buttonCount) {
		if (config->initBtnIndex >= config->buttonCount || config->initBtnIndex < 0) {
			db_error("invalid init btn index:%d btn count:%d", config->initBtnIndex, config->buttonCount);
			return -1;
		}
		if (config->initIndex >= config->total || config->initIndex < 0) {
			db_error("invalid init index:%d pic count:%d", config->initIndex, config->total);
			return -1;
		}
	}
	db_msg("initIndex:%d indexBtnIndex:%d", config->initIndex, config->initBtnIndex);

	PicDiaglogState_t _state;
	PicDiaglogState_t *state = &_state;
	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	int controlNrs = 0;

	const char *confstr;
	int retval;
	label_set_param label_set;
	WIN_RECT screenRect;
	memset(&label_set, 0, sizeof(label_set_param));
	memset(state, 0, sizeof(PicDiaglogState_t));

	if (initState(config, state) < 0) {
		db_error("init state false");
		return -1;
	}

	res_getPos(MK_system, &screenRect);
	db_msg("screen w:%d h:%d", screenRect.w, screenRect.h);

	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	confstr = res_getString2(MK_guide_dlg, "pos");
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
	retval = DialogBoxIndirectParam(&dlgTemplate, hParent, DialogProc, (LPARAM) state);
	db_msg("pic dialog ret:%d", retval);
	freeMemory(state);
	if (CtrlData) {
		free(CtrlData);
	}
	return retval;
}

static int getPicCount(const char *name) {
	int lang;
	if (NULL == name) {
		return -1;
	}
	if (!strcmp(name, "what_is_pin")) {
		return 4;
	}
	if (!strcmp(name, "what_is_passphrase")) {
		return 2;
	}
	if (!strcmp(name, "what_is_mnemonic_phase")) {
		lang = settings_get_lang();
		if (lang == LANG_CN || lang == LANG_TW) {
			return 4;
		}
		return 5;
	}
	return 1;
}

int picDialogFlag(HWND hParent, const char *name, const char *btn0, const char *btn1, int btnInitIndex, int flag) {
	PicDialogConfig_t _config;
	PicDialogConfig_t *config = &_config;
	const char *btnTitles[2] = {0, 0};
	memset(config, 0, sizeof(PicDialogConfig_t));
	config->total = (flag & PIC_DLG_FLAG_MULTI_PIC) ? getPicCount(name) : 1;
	config->name = name;
	int index = 0;
	int btnCnt = 0;
	if (btn0) {
		btnCnt++;
	}
	if (btn1) {
		btnCnt++;
	}
	if (btn0) {
		btnTitles[index] = (char *) btn0;
		index++;
	}
	if (btn1) {
		btnTitles[index] = (char *) btn1;
		index++;
	}
	config->btnTitles = btnTitles;
	config->buttonCount = btnCnt;
	config->initBtnIndex = btnInitIndex;
	config->flag = flag;
	int ret = showPicDialog(hParent, config);
	return ret;
}

int picDialog(HWND hParent, const char *name, const char *btn0, const char *btn1, int btnInitIndex) {
	return picDialogFlag(hParent, name, btn0, btn1, btnInitIndex, 0);
}
