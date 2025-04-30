#define LOG_TAG "Dialog.cpp"

#include <Dialog.h>
#include <minigui_comm.h>
#include "resource.h"
#include "Dialog.h"
#include "debug.h"
#include "key_event.h"
#include "global.h"

#define IDC_TITLE      650
#define IDC_MSG        651
#define IDC_ICON       652
#define IDC_BUTTON_IMG_0    670
#define IDC_BUTTON_TXT_0    680

#define BUTTON_IMG_ID(i) (IDC_BUTTON_IMG_0 + (i))
#define BUTTON_TXT_ID(i) (IDC_BUTTON_TXT_0 + (i))

typedef struct {
	int navi_index;
	DialogConfig_t *config;
	BITMAP *icon_img;
	BITMAP *btn_img;
	int msg_index;
	int multiPage; 
	HWND parentHwnd;
	int totalHeight;
	int curOffset;
	int screenHeight;
} DialogState;

static int buttonType(DialogConfig_t *config) {
	if (config == NULL) {
		db_error("invalid config:%p", config);
		return -1;
	}
	int btnType = config->button_style;
	if (btnType == DIALOG_BUTTON_ALIGN_CENTER || btnType == DIALOG_BUTTON_ALIGN_VERT) {
		return DIALOG_BUTTON_ALIGN_VERT;
	} else {
		return DIALOG_BUTTON_ALIGN_HORI;
	}
}

static int initDialogState(DialogState *state, DialogConfig_t *config) {
	memset(state, 0, sizeof(DialogState));
	state->config = config;
	if (config->init_btn_index > 0 && config->init_btn_index < config->button_count) {
		state->navi_index = config->init_btn_index;
	}
	return 0;
}

static BITMAP *loadButtonBitMap(DialogState *state, int index, int active) {
	static char configkey[32];
	int btnType = buttonType(state->config);
	const char *file = NULL;
	BITMAP *img = state->btn_img + index;
	if (img->bmBits) {
		UnloadBitmap(img);
	}
	//snprintf(configkey, sizeof(configkey), "btn%d_img_%d_%d", index, state->config->button_style, active);
	//file = rm->getConfigString2(MK_dialog, configkey);
	if (!file) {
		snprintf(configkey, sizeof(configkey), "btn_img_%d_%d", btnType, active);
		file = res_getString2(MK_dialog, configkey);
	}
	db_msg("index:%d active:%d path:%s", index, active, file);
	if (file) {
		int ret = LoadBitmapFromFile(HDC_SCREEN, img, file);
		if (ret != ERR_BMP_OK) {
			db_error("load bitmap path:%s failed", file);
		}
	}
	return img;
}

static int onInitDialog(DialogState *state, HWND hDlg) {
	DialogConfig_t *config = state->config;
	db_msg("msg:%s tilte:%s font:%d style:%d", config->msg.data, config->title.data, config->title.font, config->title.style);
	HWND hwnd;
	setDialogLabelStyle(hDlg, IDC_TITLE, &config->title);
	setDialogLabelStyle(hDlg, IDC_MSG, &config->msg);
	if (config->icon_style) {
		hwnd = GetDlgItem(hDlg, IDC_ICON);
		if (IS_VALID_HWND(hwnd) && state->icon_img) {
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) state->icon_img, 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
	}
	for (int i = 0; i < config->button_count; i++) {
		setDialogLabelStyle(hDlg, BUTTON_TXT_ID(i), &config->buttons[i]);
		hwnd = GetDlgItem(hDlg, BUTTON_IMG_ID(i));
		if (IS_VALID_HWND(hwnd)) {
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) loadButtonBitMap(state, i, i == state->navi_index ? 1 : 0), 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
	}
	return 0;
}

static void initDialogConfig(DialogConfig_t *config, const char *title, dialogIconStyle_t icon_style, const char *msg, dialogButtonAlign_t align, const char *button0, const char *button1) {
	memset(config, 0, sizeof(DialogConfig_t));
	config->win_style = 1;
	config->msg_style = 1;
	config->msg.data = msg;
	config->icon_style = icon_style;

	if (title) {
		config->title.data = title;
		config->title_style = 1;
	}

	int c = 0;
	if (button0) {
		config->buttons[c++].data = button0;
	}
	if (button1) {
		config->buttons[c++].data = button1;
	}
	config->button_style = align;
	config->button_count = c;
}

static int scrollWindowForKeyDown(DialogState *state, HWND hDlg, WPARAM code) {
	const int screenHeight = 240;
	if (state->totalHeight <= screenHeight) {
		return 0;
	}
	DialogConfig_t *config = state->config;
	int nextPage = -1;
	int move = 0;
	RECT rect;
	int showMaxY = state->curOffset + screenHeight;
	switch (code) {
		case INPUT_KEY_UP: {
			if (state->curOffset <= 0) {
				move = 0;
			} else if (state->curOffset >= screenHeight) {
				move = screenHeight;
			} else {
				move = state->curOffset;
			}
		}
			break;
		case INPUT_KEY_DOWN: {
			if (showMaxY == state->totalHeight) {
				move = 0;
			} else if (showMaxY + screenHeight < state->totalHeight) {
				move = -screenHeight;
			} else {
				move = -(state->totalHeight - screenHeight - state->curOffset);
			}
		}
			break;
		default:
			db_msg("");
			break;
	}
	db_msg("move:%d curOffset:%d", move, state->curOffset);

	if (move != 0) {
		ScrollWindow(hDlg, 0, move, NULL, NULL);
		InvalidateRect(hDlg, NULL, TRUE);
		state->curOffset -= move;
	}
	return 0;
}

static int onKeyDown(DialogState *state, HWND hDlg, WPARAM code) {
	DialogConfig_t *config = state->config;
	int index;
	HWND hwnd;
	int btnType = buttonType(config);
	switch (code) {
		case INPUT_KEY_OK:
			if (!state->multiPage || ((state->curOffset + state->screenHeight) == state->totalHeight)) {
				EndDialog(hDlg, state->navi_index);
			}
			break;
		case INPUT_KEY_ESC:
//			EndDialog(hDlg, KEY_EVENT_ABORT);
			break;
		case INPUT_KEY_LEFT: {
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		case INPUT_KEY_RIGHT:
			break;
		case INPUT_KEY_UP:
		case INPUT_KEY_DOWN: {
			db_msg("curOffset:%d totalHeight:%d", state->curOffset, state->totalHeight);
			if (!state->multiPage || ((state->curOffset + state->screenHeight) == state->totalHeight)) {
				if (config->button_count > 1) {
					if (code == INPUT_KEY_UP) {
						index = (config->button_count + state->navi_index - 1) % config->button_count;
					} else {
						index = (state->navi_index + 1) % config->button_count;
					}
					hwnd = GetDlgItem(hDlg, BUTTON_IMG_ID(state->navi_index));
					if (IS_VALID_HWND(hwnd)) {
						SendMessage(hwnd, STM_SETIMAGE, (WPARAM) loadButtonBitMap(state, state->navi_index, 0), 0);
						InvalidateRect(hwnd, NULL, TRUE);
					}
					hwnd = GetDlgItem(hDlg, BUTTON_IMG_ID(index));
					if (IS_VALID_HWND(hwnd)) {
						SendMessage(hwnd, STM_SETIMAGE, (WPARAM) loadButtonBitMap(state, index, 1), 0);
						InvalidateRect(hwnd, NULL, TRUE);
					}
					state->navi_index = index;
					return 0;
				}
			}
			if (state->multiPage) {
				scrollWindowForKeyDown(state, hDlg, code);
				return 0;
			}
		}
		default:
			break;
	}
	return 0;
}

static int onKeyUp(DialogState *state, HWND hDlg, WPARAM code) {
	return 0;
}

static PROC_RET DialogProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	DialogState *state;
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (DialogState *) lParam;
			SetWindowBkColor(hDlg, res_getBGColor());
			onInitDialog(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (DialogState *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (DialogState *) GetWindowAdditionalData(hDlg);
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

int showDialog(HWND hParent, DialogConfig_t *config) {
	DialogState _state;
	DialogState *state = &_state;

	SPEC_TEXT_ALIGN spec_align = SPEC_TEXT_ALIGN_DEFAULT;
	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	CTRLDATA *c;
	unsigned int controlNrs;

	const char *confstr;
	char configkey[32];
	int retval;
	int msg_index = 0;
	int title_index = 0;
	int icon_index = 0;
	int button_index = 0;
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;
	SIZE msgSize;

	label_set_param icon_set;
	label_set_param title_set;
	label_set_param _msg_set;
	label_set_param *msg_set = &_msg_set;
	WIN_RECT screenRect;
	int ret = 0;

	memset(&icon_set, 0, sizeof(label_set_param));
	memset(&title_set, 0, sizeof(label_set_param));
	memset(msg_set, 0, sizeof(label_set_param));

	res_getPos(MK_system, &screenRect);
	db_msg("screen w:%d h:%d", screenRect.w, screenRect.h);

	if (initDialogState(state, config) != 0) {
		db_msg("init state false");
		return -1;
	}
	db_msg("init state icon_style:%d button_count:%d", config->icon_style, config->button_count);

	controlNrs = 0;
	msg_index = controlNrs++;

	db_msg("title:%s", config->title.data);
	if (config->title.data && config->title_style) {
		title_index = controlNrs++;
	}
	if (config->icon_style) {
		icon_index = controlNrs++;
	}
	if (config->button_count) {
		button_index = controlNrs;
		controlNrs += config->button_count * 2; //image & label
	}
	db_msg("title_index:%d icon_index:%d button_index:%d", title_index, icon_index, button_index);
	CtrlData = (PCTRLDATA) malloc(sizeof(CTRLDATA) * controlNrs);
	if (!CtrlData) {
		db_error("new ctrl data memory false");
		return -1;
	}
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));

	if (icon_index) {
		state->icon_img = (BITMAP *) malloc(sizeof(BITMAP));
		if (!state->icon_img) {
			db_error("new icon_img memory false");
			free(CtrlData);
			return -1;
		}
		memset(state->icon_img, 0, sizeof(BITMAP));
	}

	if (config->button_count) {
		state->btn_img = (BITMAP *) malloc(sizeof(BITMAP) * config->button_count);
		if (!state->btn_img) {
			db_error("new btn_img memory false");
			free(CtrlData);
			if (state->icon_img) {
				free(state->icon_img);
			}
			return -1;
		}
		memset(state->btn_img, 0, sizeof(BITMAP) * config->button_count);
	}

	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	if (title_index) {
		snprintf(configkey, sizeof(configkey), "title_%d_pos", config->title_style);
		db_msg("title config:%s", configkey);
		res_getLabelSetParam(&title_set, MK_dialog, configkey);
		db_msg("dialog title:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->title.data, title_set.font, title_set.style, title_set.x, title_set.y, title_set.w, title_set.h);
		initStringCtrlData(&CtrlData[title_index], IDC_TITLE, &config->title, &title_set);
	}
	snprintf(configkey, sizeof(configkey), "msg_%d_pos", config->msg_style);
	res_getLabelSetParam(msg_set, MK_dialog, configkey);
	int style = msg_set->style;
	if (config->msg.data) {
		if (strlen(config->msg.data) > 80) { //change
			msg_set->font = 3;
		}
		spec_align = (SPEC_TEXT_ALIGN) getSpecAlignFromStr((const char *) (config->msg.data));
		style = style & 0x01;
		switch (spec_align) {
			case SPEC_TEXT_ALIGN_LEFT:
				msg_set->style = style | LABEL_STYLE_LEFT;
				config->msg.data = filterSpecTextFromStr(config->msg.data);
				break;
			case SPEC_TEXT_ALIGN_CENTER:
				msg_set->style = style | LABEL_STYLE_CENTER;
				config->msg.data = filterSpecTextFromStr(config->msg.data);
				db_msg("fix msg:%s", config->msg.data);
				break;
			case SPEC_TEXT_ALIGN_RIGHT:
				msg_set->style = style | LABEL_STYLE_RIGHT;
				config->msg.data = filterSpecTextFromStr(config->msg.data);
				break;
			default:
				break;
		}
	}
	res_getLabelSetParam(&icon_set, MK_dialog, "icon_pos");
	if (config->icon_style == DIALOG_ICON_STYLE_NONE) {
		if (title_index) {
			msg_set->y += title_set.h + 2 * title_set.y;
		}
	} else if ((config->icon_style == DIALOG_ICON_STYLE_ERR) || (config->icon_style == DIALOG_ICON_STYLE_SUCCESS || (config->icon_style == DIALOG_ICON_STYLE_ALTER))) {
		msg_set->y += icon_set.h + icon_set.y;
	}
	state->msg_index = msg_index;

	if (config->msg.data) {

	} else {
		msg_set->h = 0;
		msg_set->w = 0;
		db_msg("no msg");
	}

	if (icon_index) {
		snprintf(configkey, sizeof(configkey), "icon_pos");
		initImageCtrlData(&CtrlData[icon_index], IDC_ICON, res_getString2(MK_dialog, configkey));
		snprintf(configkey, sizeof(configkey), "icon_img_%d", config->icon_style);
		confstr = res_getString2(MK_dialog, configkey);
		if (confstr) {
			ret = LoadBitmapFromFile(HDC_SCREEN, state->icon_img, confstr);
			if (ret) {
				db_error("load bitmap path:%s failed", confstr);
			} else {
				db_msg("load bitmap path:%s OK", confstr);
			}
		}
	}
	WIN_RECT rect;
	int btnType = buttonType(config);
	int allButtonTotalHeight = 0;
	int btnHeight = res_getInt(MK_dialog, "btn_height", 34);
	int btnVeriSpace = res_getInt(MK_dialog, "long_btn_veri_space", 6);
	int longBtnWidth = res_getInt(MK_dialog, "long_btn_width", 210);
	int shortBtnWidth = res_getInt(MK_dialog, "short_btn_width", 100);
	int btnHoriSpace = res_getInt(MK_dialog, "short_btn_hori_space", 10);
	if (config->button_style == DIALOG_BUTTON_ALIGN_HORI) {
		allButtonTotalHeight = btnHeight;
	} else {
		allButtonTotalHeight = config->button_count * btnHeight + (config->button_count - 1) * btnVeriSpace;
	}

	int height = msg_set->y + allButtonTotalHeight + 2 * btnVeriSpace;
	msg_set->h = screenRect.h - height;
	state->screenHeight = screenRect.h;
	state->totalHeight = screenRect.h;
	state->multiPage = 0;

	initStringCtrlData(&CtrlData[msg_index], IDC_MSG, &config->msg, msg_set);
	CtrlData[msg_index].dwExStyle |= SS_LEFTNOWORDWRAP;
	db_msg("msg x:%d y:%d w:%d h:%d", msg_set->x, msg_set->y, msg_set->w, msg_set->h);
	int btnCnt = config->button_count;
	if (button_index) {
		rect.h = btnHeight;
		for (int j = 0; j < btnCnt; ++j) {
			if (config->button_style == DIALOG_BUTTON_ALIGN_CENTER) {
				rect.w = longBtnWidth;
				rect.x = (screenRect.w - longBtnWidth) / 2;
				rect.y = (screenRect.h - allButtonTotalHeight) / 2 + (btnCnt - 1 - j) * (btnHeight + btnVeriSpace);
			} else if (config->button_style == DIALOG_BUTTON_ALIGN_HORI) {
				rect.w = shortBtnWidth;
				if (state->multiPage) {
					rect.y = msg_set->y + msg_set->h + btnVeriSpace;
				} else {
					rect.y = screenRect.h - allButtonTotalHeight - btnVeriSpace;
				}
				rect.x = (screenRect.w - btnHoriSpace - 2 * shortBtnWidth) / 2 + (btnCnt - 1 - j) * (shortBtnWidth + btnHoriSpace);
			} else if (config->button_style == DIALOG_BUTTON_ALIGN_VERT) {
				rect.w = longBtnWidth;
				rect.x = (screenRect.w - longBtnWidth) / 2;
				if (state->multiPage) {
					rect.y = state->totalHeight - allButtonTotalHeight - btnVeriSpace + (btnCnt - 1 - j) * (btnHeight + btnVeriSpace);
				} else {
					rect.y = screenRect.h - allButtonTotalHeight - btnVeriSpace + (btnCnt - 1 - j) * (btnHeight + btnVeriSpace);
				}
			}
			initImageCtrlDataWithRect(&CtrlData[button_index + 2 * j], BUTTON_IMG_ID(j), rect);
			res_getLabelSetParam(label_set, MK_dialog, "btn_txt_config");
			label_set->y = rect.y;
			label_set->x = rect.x;
			label_set->w = rect.w;
			label_set->h = rect.h;
			initStringCtrlData(&CtrlData[button_index + 2 * j + 1], BUTTON_TXT_ID(j), &config->buttons[j], label_set);
		}
	}

	snprintf(configkey, sizeof(configkey), "win_%d_pos", config->win_style);
	confstr = res_getString2(MK_dialog, configkey);
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
	//int pkeyesc = use_powerkey_as_esc(1);
	retval = DialogBoxIndirectParam(&dlgTemplate, hParent, DialogProc, (LPARAM) state);
	//use_powerkey_as_esc(pkeyesc);
	db_msg("dialog result:%d", retval);

	if (state->icon_img) {
		UnloadBitmap(state->icon_img);
		free(state->icon_img);
	}
	if (state->btn_img) {
		for (int i = 0; i < config->button_count; i++) {
			UnloadBitmap(state->btn_img + i);
		}
		free(state->btn_img);
	}
	free(CtrlData);
	return retval;
}

int dialog(HWND hParent, const char *title, dialogIconStyle_t icon_style, const char *msg, dialogButtonAlign_t align, const char *btn1, const char *btn2, int init_state) {
	DialogConfig_t _config;
	initDialogConfig(&_config, title, icon_style, msg, align, btn1, btn2);
	db_msg("title:%s msg:%s icon_style:%d align:%d btn1:%s btn2:%s count:%d", title, msg, icon_style, align, btn1, btn2, _config.button_count);
	_config.init_btn_index = init_state;
	return showDialog(hParent, &_config);
}

int dialog_l(HWND hParent, const char *title, dialogIconStyle_t icon_style, const char *msg, dialogButtonAlign_t align, const char *btn1, const char *btn2, int init_state) {
	DialogConfig_t _config;
	initDialogConfig(&_config, title, icon_style, msg, align, btn1, btn2);
	db_msg("title:%s msg:%s icon_style:%d align:%d btn1:%s btn2:%s count:%d", title, msg, icon_style, align, btn1, btn2, _config.button_count);
	_config.init_btn_index = init_state;
	_config.msg_style = 2;
	return showDialog(hParent, &_config);
}

int dialog_alert(HWND hParent, dialogIconStyle_t icon_style, const char *msg, const char *button_msg) {
	return dialog(hParent, NULL, icon_style, msg, DIALOG_BUTTON_ALIGN_VERT, button_msg, NULL, 0);
}

int dialog_error(HWND hParent, const char *msg) {
	return dialog(hParent, NULL, DIALOG_ICON_STYLE_ERR, msg, DIALOG_BUTTON_ALIGN_VERT, res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
}

int dialog_error2(HWND hParent, const char *msg, const char *btn_msg) {
	return dialog(hParent, NULL, DIALOG_ICON_STYLE_ERR, msg, DIALOG_BUTTON_ALIGN_VERT, btn_msg, NULL, 0);
}

int dialog_error3(HWND hParent, int code, const char *msg) {
    char str[256] = {0};
    snprintf(str, sizeof(str), "(%d)%s", code, msg);
    return dialog_error(hParent, str);
}

extern int dialog_system_error(HWND hParent, int code) {
	char str[64] = {0};
	snprintf(str, sizeof(str), res_getLabel(LANG_LABEL_SYSTEM_ERR), code);
	return dialog_error(hParent, str);
}

extern int dialog_system_error2(HWND hParent, int code, const char *tips_msg, const char *btn_msg) {
	char str[128] = {0};
	snprintf(str, sizeof(str), "%s(%d)", tips_msg, code);
	if (!btn_msg) btn_msg = res_getLabel(LANG_LABEL_SUBMENU_OK);
	return dialog_error2(hParent, str, btn_msg);
}

int dialog_confirm(HWND hParent, const char *title, const char *msg, dialogButtonAlign_t align, const char *cancel_msg, const char *ok_msg, int init_state) {
	return dialog(hParent, title, DIALOG_ICON_STYLE_NONE, msg, align, cancel_msg, ok_msg, init_state);
}
