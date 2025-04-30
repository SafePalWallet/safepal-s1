#define LOG_TAG "CommonQRShow"

#include "CommonQRShow.h"
#include "NavBar.h"
#include "common.h"
#include "minigui_comm.h"
#include "ConfigKey.h"
#include "key_event.h"
#include "resource.h"

#define IDC_MSG 650
#define IDC_QR_CODE 651

typedef struct {
	unsigned char *data;
	int size;
	int style;
	char *msg;
	NavBar *navBar;
	HWND msgHwnd;
	HWND qrHwnd;
	BITMAP qrBitmap;
	BITMAP btnHighlighImg;
} CommonQRState_t;

static int hidden_show_up_down(CommonQRState_t *state) {
	if (state->style & COMMON_QR_STYLE_RIGHT_SINGLE) {
		return true;
	}
	
	return false;
}

static int createNavBar(HWND hDlg, CommonQRState_t *state) {
	NavBar *nav = new NavBar(hDlg);
	int op = 0;
	int hide_state = hidden_show_up_down(state);
	op = hide_state ? NAV_BAR_ITEM_SHOW_BACK : NAV_BAR_ITEM_SHOW_BACK | NAV_BAR_ITEM_SHOW_UP_DOWN;
	const char *next_txt = NULL;
    if ((state->style & COMMON_QR_STYLE_RIGHT_LEGACY) || (state->style & COMMON_QR_STYLE_RIGHT_LEGACY_LTC) || (state->style & COMMON_QR_STYLE_RIGHT_LEGACY_BCH)) {
        next_txt = "Legacy";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if (state->style & COMMON_QR_STYLE_RIGHT_SEGWIT) {
        next_txt = "SegWit";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if ((state->style & COMMON_QR_STYLE_RIGHT_NSEGWIT) || (state->style & COMMON_QR_STYLE_RIGHT_NSEGWIT_LTC)) {
        next_txt = "{11}Native SegWit";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if (state->style & COMMON_QR_STYLE_RIGHT_CASH) {
        next_txt = "CashAddr";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if (state->style & COMMON_QR_STYLE_RIGHT_DEFAULT) {
        next_txt = "Default";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if (state->style & COMMON_QR_STYLE_RIGHT_OPTIONAL) {
        next_txt = "Optional";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    } else if (state->style & COMMON_QR_STYLE_RIGHT_TAPROOT) {
        next_txt = "taproot";
        op |= NAV_BAR_ITEM_SHOW_NEXT;
    }

    const char *current_txt = NULL;
    if (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_LEGACY)) {
        current_txt = "taproot";
        op |= NAV_BAR_ITEM_SHOW_CURRENT;
    } else if ((hide_state && (state->style & COMMON_QR_STYLE_RIGHT_SEGWIT)) || (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_CASH)) || (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_NSEGWIT_LTC))) {
        current_txt = "Legacy";
        op |= NAV_BAR_ITEM_SHOW_CURRENT;
    } else if (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_NSEGWIT)) {
        current_txt = "SegWit";
        op |= NAV_BAR_ITEM_SHOW_CURRENT;
    } else if ((hide_state && (state->style & COMMON_QR_STYLE_RIGHT_TAPROOT)) || (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_LEGACY_LTC))) {
        current_txt = "Native SegWit";
        op |= NAV_BAR_ITEM_SHOW_CURRENT;
    } else if (hide_state && (state->style & COMMON_QR_STYLE_RIGHT_LEGACY_BCH)) {
        current_txt = "CashAddr";
        op |= NAV_BAR_ITEM_SHOW_CURRENT;
    }
	db_msg("state->style:%d next_txt:%s msg:%s", state->style, next_txt, state->msg);
	nav->init(res_getLabel(LANG_LABEL_BACK), next_txt, current_txt, op, 0);
	state->navBar = nav;
	return 0;
}

static int onInit(CommonQRState_t *state, HWND hDlg) {
	db_msg("hDlg:%d", hDlg);
	HWND hwnd;
	SetWindowBkColor(hDlg, res_getBGColor());
	if (state->style & COMMON_QR_STYLE_NAVBAR) {
		createNavBar(hDlg, state);
	}

	label_set_param label_set;
	if (state->style & COMMON_QR_STYLE_FULL) {
		res_getLabelSetParam(&label_set, MK_common_qr_show, "qr_full_pos");
    } else if (state->style & COMMON_QR_STYLE_NAVBAR || state->style & COMMON_QR_STYLE_NAVBAR_NO_BAR) {
		res_getLabelSetParam(&label_set, MK_common_qr_show, "qr_navbar_pos");
	} else {
		res_getLabelSetParam(&label_set, MK_common_qr_show, "qr_pos");
	}
	genQrcodeBitmap(&state->qrBitmap, state->data, state->size, label_set.w, label_set.h, 1, -1);

	hwnd = createWidgetWindow(hDlg, 0, label_set.x, label_set.y, label_set.w, label_set.h, IDC_QR_CODE, WIDGET_TYPE_IMG, 0, -1);
	if (IS_VALID_HWND(hwnd)) {
		state->qrHwnd = hwnd;
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) &state->qrBitmap, 0);
		InvalidateRect(hwnd, NULL, TRUE);
		ShowWindow(hwnd, SW_SHOW);
	}

	if (state->msg && !(state->style & COMMON_QR_STYLE_FULL)) {
        if (state->style & COMMON_QR_STYLE_NAVBAR) {
            res_getLabelSetParam(&label_set, MK_common_qr_show, "msg_navbar_config");
        } else if (state->style & COMMON_QR_STYLE_BUTTON) {
            res_getLabelSetParam(&label_set, MK_common_qr_show, "msg_button_config");
        } else if (state->style & COMMON_QR_STYLE_NAVBAR_NO_BAR) {
            res_getLabelSetParam(&label_set, MK_common_qr_show, "msg_navbar_config_no_bar");
        } else {
            res_getLabelSetParam(&label_set, MK_common_qr_show, "msg_config");
        }
        if (state->style & COMMON_QR_STYLE_FONT12) {
            label_set.font = 11;
        }
		if (state->style & COMMON_QR_STYLE_BUTTON) {
			hwnd = createWidgetWindow(hDlg, 0, label_set.x, label_set.y, label_set.w, label_set.h, IDC_MSG + 1, WIDGET_TYPE_IMG, 0, 0);
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) &(state->btnHighlighImg), 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		hwnd = createWidgetWindow(hDlg, 0, label_set.x, label_set.y, label_set.w, label_set.h, IDC_MSG, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
		state->msgHwnd = hwnd;
		setLabelTextColor(hwnd, res_getTextColor(0));
		SetWindowText(hwnd, state->msg);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	return 0;
}

static int onKeyDown(CommonQRState_t *state, HWND hDlg, WPARAM code) {
	switch (code) {
		case INPUT_KEY_OK: {
			EndDialog(hDlg, KEY_EVENT_OK);
		}
			break;
		case INPUT_KEY_UP: {
			int hide_state = hidden_show_up_down(state);
			if (hide_state) {
				return 0;
			}
			if (state->style & COMMON_QR_STYLE_UPDOWN_KEY) {
				EndDialog(hDlg, KEY_EVENT_UP);
			}
		}
			break;
		case INPUT_KEY_DOWN: {
			int hide_state = hidden_show_up_down(state);
			if (hide_state) {
				return 0;
			}

			if (state->style & COMMON_QR_STYLE_UPDOWN_KEY) {
				EndDialog(hDlg, KEY_EVENT_DOWN);
			}
		}
			break;
		case INPUT_KEY_LEFT: {
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		case INPUT_KEY_RIGHT: {
            if (state->style & (COMMON_QR_STYLE_RIGHT_LEGACY | COMMON_QR_STYLE_RIGHT_SEGWIT | COMMON_QR_STYLE_RIGHT_NSEGWIT
                                | COMMON_QR_STYLE_RIGHT_CASH | COMMON_QR_STYLE_RIGHT_DEFAULT | COMMON_QR_STYLE_RIGHT_OPTIONAL
                                | COMMON_QR_STYLE_RIGHT_TAPROOT | COMMON_QR_STYLE_RIGHT_LEGACY_LTC | COMMON_QR_STYLE_RIGHT_NSEGWIT_LTC
                                | COMMON_QR_STYLE_RIGHT_LEGACY_BCH)) {
				EndDialog(hDlg, KEY_EVENT_NEXT);
			} else if (state->style & COMMON_QR_STYLE_BUTTON) {
				EndDialog(hDlg, KEY_EVENT_OK);
			}
		}
			break;
		default:
			break;
	}
	return 0;
}

static PROC_RET CommonQRShowDialogProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	CommonQRState_t *state;
	db_msg("CommonQRShowDialogProc message:%d", message);
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (CommonQRState_t *) lParam;
			onInit(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (CommonQRState_t *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
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

static void freeMemory(CommonQRState_t *state) {
	if (state->navBar) {
		delete state->navBar;
		state->navBar = NULL;
	}

	if (IS_VALID_HWND(state->msgHwnd)) {
		DestroyWindow(state->msgHwnd);
		state->msgHwnd = HWND_INVALID;
	}
	if (IS_VALID_HWND(state->qrHwnd)) {
		DestroyWindow(state->qrHwnd);
		state->qrHwnd = HWND_INVALID;
	}
	if (state->qrBitmap.bmBits) {
		UnloadBitmap(&state->qrBitmap);
	}
	if (state->btnHighlighImg.bmBits) {
		db_msg("free btnHighlighImg");
		UnloadBitmap(&(state->btnHighlighImg));
	}
}

int showCommonQR(HWND parent, const unsigned char *data, int size, const char *msg, int style) {
	CommonQRState_t _state;
	CommonQRState_t *state = &_state;
	memset(state, 0, sizeof(CommonQRState_t));

	DLGTEMPLATE dlgTemplate;
	state->data = (unsigned char *) data;
	state->size = size;
	state->msg = (char *) msg;
	state->style = style;

	if (style & COMMON_QR_STYLE_BUTTON) {
		const char *confstr = res_getString2(MK_system_icon, "btn_long_sel_bg_img");
		if (confstr) {
			res_loadBmp(confstr, &(state->btnHighlighImg));
		}
	}
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	const char *confstr = res_getString2(MK_common_qr_show, "pos");
	if (confstr) {
		sscanf(confstr, "%d %d %d %d", &(dlgTemplate.x), &(dlgTemplate.y), &(dlgTemplate.w), &(dlgTemplate.h));
	} else {
		dlgTemplate.x = 0;
		dlgTemplate.y = 0;
		res_screen_info(&dlgTemplate.w, &dlgTemplate.h);
	}

	dlgTemplate.caption = "";
	dlgTemplate.controlnr = 0;
	dlgTemplate.controls = NULL;
	int retval = DialogBoxIndirectParam(&dlgTemplate, parent, CommonQRShowDialogProc, (LPARAM) state);
	db_msg("ret:%d", retval);
	freeMemory(state);
	return retval;
}
