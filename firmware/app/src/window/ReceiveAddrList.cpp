#define LOG_TAG "ReceiveAddrList"

#include <string.h>
#include "ReceiveAddrList.h"
#include "NavBar.h"
#include "ListView.h"
#include "resource.h"
#include "common.h"
#include "minigui_comm.h"
#include "ConfigKey.h"
#include "key_event.h"
#include "resource.h"

#define LIST_ITEM_COUNT 3
#define LIST_ITEM_SELECT_INDEX 1

#define IDC_TITLE 650

#define IDC_CONTAINER(i) (1000+(i))
#define IDC_ITEM_TITLE(i) (2000+(i))
#define IDC_ADDR(i) (3000+(i))
#define IDC_BG(i) (4000+(i))

#define ADDRESS_BUFFER_SIZE 130
typedef struct {
	uint32_t index;
	char address[ADDRESS_BUFFER_SIZE];
} ReceiveIndexAddrData;

typedef struct {
	gen_index_address_func gen_func;
	void *user;
	int maxIndex;
	NavBar *navBar;
	int *outIndex;
	char *outAddress;
	ReceiveIndexAddrData address[LIST_ITEM_COUNT];
	HWND containerHwnds[LIST_ITEM_COUNT];
	HWND addrHwnds[LIST_ITEM_COUNT];
	HWND itemTitleHwnds[LIST_ITEM_COUNT];
	HWND bgHwnds[LIST_ITEM_COUNT];
	HWND titleHwnd;
	int curIndex;
	int newIndex;
	BITMAP shadowUpBitmap;
	BITMAP shadowDownBitmap;
	BITMAP middleBitmap;
} ReceiveAddrListState_t;


static int genNewAddress(ReceiveAddrListState_t *state, int pos, int index) {
	if (!state || !state->gen_func) {
		db_error("invalid state");
		return -1;
	}
	db_msg("pos:%d index:%d maxindex:%d", pos, index, state->maxIndex);
	if (pos < 0 || pos >= LIST_ITEM_COUNT) {
		return -1;
	}
	if (index < 0) {
		index = state->maxIndex;
	} else if (index > state->maxIndex) {
		index = 0;
	}

	int ret = state->gen_func(state->user, state->address[pos].address, ADDRESS_BUFFER_SIZE, (uint32_t) index);
	if (ret > 0) {
		state->address[pos].index = (uint32_t) index;
	} else {
		state->address[pos].index = 0;
		memset(state->address[pos].address, 0, ADDRESS_BUFFER_SIZE);
	}
	db_msg("ret:%d pos:%d index:%d addr:%s", ret, pos, state->address[pos].index, state->address[pos].address);
	return 0;
}

static int createNavBar(HWND hDlg, ReceiveAddrListState_t *state) {
	NavBar *nav = new NavBar(hDlg);
	int op = 0;
	op = NAV_BAR_ITEM_SHOW_OK | NAV_BAR_ITEM_SHOW_BACK;
	nav->init(res_getLabel(LANG_LABEL_BACK), NULL, NULL, op, 0);
	state->navBar = nav;
	return 0;
}

static int updateUI(ReceiveAddrListState_t *state, HWND hDlg) {
	HWND hwnd;
	char str[64] = {0};
	ReceiveIndexAddrData *it;
	const char *addr;
	for (int i = 0; i < LIST_ITEM_COUNT; ++i) {
		it = &state->address[i];
		if ((int) it->index >= state->newIndex) {
			snprintf(str, sizeof(str), res_getLabel(LANG_LABEL_NEW_ADDRESS), (int) (it->index + 1));
		} else {
			snprintf(str, sizeof(str), res_getLabel(LANG_LABEL_ADDRESS_NUM), (int) (it->index + 1));
		}
		setDCLabelText(state->itemTitleHwnds[i], str, (i != LIST_ITEM_SELECT_INDEX));

		hwnd = state->addrHwnds[i];
		addr = state->address[i].address;
		if (i == LIST_ITEM_SELECT_INDEX) {
			setDCLabelText(hwnd, addr, false);
		} else {
			if (strlen(addr) <= 22) {
				setDCLabelText(hwnd, addr, true);
			} else {
				memset(str, 0, sizeof(str));
				strncpy(str, addr, 8);
				strcat(str, "......");
				strncpy(str + 8 + 6, addr + strlen(addr) - 8, 8);
				setDCLabelText(hwnd, str, true);
			}
		}
	}
	return 0;
}

static int onInit(ReceiveAddrListState_t *state, HWND hDlg) {
	HWND hwnd;
	createNavBar(hDlg, state);
	SetWindowBkColor(hDlg, res_getBGColor());

	WIN_RECT listRect;
	res_getRect(MK_addr_list, "list_pos", &listRect);
	WIN_RECT rect;
	label_set_param label_set;
	int selFont = res_getInt(MK_addr_list, "sel_font", 3);
	int norFont = res_getInt(MK_addr_list, "nor_font", 5);
	BITMAP bitmap;
	memset(&bitmap, 0, sizeof(BITMAP));

	res_getLabelSetParam(&label_set, MK_addr_list, "title_config");
	hwnd = createWidgetWindow(hDlg, 0, label_set.x, label_set.y, label_set.w, label_set.h, IDC_TITLE, WIDGET_TYPE_TEXT, label_set.style, label_set.font);
	if (IS_VALID_HWND(hwnd)) {
		state->titleHwnd = hwnd;
		SetWindowMText(hwnd, res_getLabel(LANG_LABEL_SELECT_ADDR));
	}

	const char *itemsPos[LIST_ITEM_COUNT] = {"shadow_up_pos", "middle_item_pos", "shadow_down_pos"};
	int offsetX = 5;
	for (int i = 0; i < LIST_ITEM_COUNT; ++i) {
		res_getRect(MK_addr_list, itemsPos[i], &rect);
		if (i == LIST_ITEM_SELECT_INDEX) {
			hwnd = createWidgetWindow(hDlg, 0, rect.x, rect.y, rect.w, rect.h, IDC_CONTAINER(i), WIDGET_TYPE_IMG, 0, -1);
			if (IS_VALID_HWND(hwnd)) {
				SendMessage(hwnd, STM_SETIMAGE, (WPARAM) &state->middleBitmap, 0);
				InvalidateRect(hwnd, NULL, TRUE);
				state->containerHwnds[i] = hwnd;
			}
			hwnd = createWidgetWindow(hDlg, 0, rect.x + offsetX, rect.y, rect.w - 2 * offsetX, 24, IDC_ITEM_TITLE(i), WIDGET_TYPE_TEXT, 2, selFont);
			state->itemTitleHwnds[i] = hwnd;
			setLabelTextColor(hwnd, res_getTextColor(0));

			hwnd = createWidgetWindow(hDlg, 0, rect.x + offsetX, rect.y + 20, rect.w - 2 * offsetX, 50, IDC_ADDR(i), WIDGET_TYPE_TEXT, 2, selFont);
			state->addrHwnds[i] = hwnd;
			setLabelTextColor(hwnd, res_getTextColor(0));
		} else {
			hwnd = createWidgetWindow(hDlg, 0, rect.x + offsetX, i == 0 ? rect.y + 10 : rect.y, rect.w - 2 * offsetX, 20, IDC_ITEM_TITLE(i), WIDGET_TYPE_TEXT, 2, norFont);
			state->itemTitleHwnds[i] = hwnd;
			setLabelTextColor(hwnd, res_getDisableColor());
			hwnd = createWidgetWindow(hDlg, 0, rect.x + offsetX, i == 0 ? rect.y + 10 + 20 : rect.y + 18, rect.w - 2 * offsetX, 20, IDC_ADDR(i), WIDGET_TYPE_TEXT, 2, norFont);
			state->addrHwnds[i] = hwnd;
			setLabelTextColor(hwnd, res_getDisableColor());
			hwnd = createWidgetWindow(hDlg, 0, rect.x, rect.y, rect.w, rect.h, IDC_CONTAINER(i), WIDGET_TYPE_IMG, 0, -1);
			if (IS_VALID_HWND(hwnd)) {
				state->containerHwnds[i] = hwnd;
				if (i == 0) {
					SendMessage(hwnd, STM_SETIMAGE, (WPARAM) &state->shadowUpBitmap, 0);
				} else {
					SendMessage(hwnd, STM_SETIMAGE, (WPARAM) &state->shadowDownBitmap, 0);
				}
				InvalidateRect(hwnd, NULL, TRUE);
			}
		}
	}
	return 0;
}

static int onKeyDown(ReceiveAddrListState_t *state, HWND hDlg, WPARAM code) {
	int i;
	switch (code) {
		case INPUT_KEY_OK: {
			if (state->outIndex) {
				*(state->outIndex) = state->address[1].index;
			}
			if (state->outAddress) {
				strlcpy(state->outAddress, state->address[1].address, ADDRESS_BUFFER_SIZE);
			}
			EndDialog(hDlg, KEY_EVENT_OK);
		}
			break;
		case INPUT_KEY_DOWN: {
			for (i = 0; i < (LIST_ITEM_COUNT - 1); i++) {
				memcpy(&state->address[i], &state->address[i + 1], sizeof(ReceiveIndexAddrData));
			}
			genNewAddress(state, i, state->address[i].index + 1);
			SendMessage(hDlg, MSG_PAINT, 0, 0);
		}
			break;
		case INPUT_KEY_UP: {
			for (i = (LIST_ITEM_COUNT - 1); i > 0; i--) {
				memcpy(&state->address[i], &state->address[i - 1], sizeof(ReceiveIndexAddrData));
			}
			genNewAddress(state, 0, state->address[0].index - 1);
			SendMessage(hDlg, MSG_PAINT, 0, 0);
		}
			break;
		case INPUT_KEY_LEFT: {
			EndDialog(hDlg, KEY_EVENT_BACK);
		}
			break;
		default:
			break;
	}
	return 0;
}

static void destroyHwnd(ReceiveAddrListState_t *state) {
	HWND hwnd;
	db_msg("destroyHwnd");
	if (state->navBar) {
		delete state->navBar;
		state->navBar = NULL;
	}
	for (int i = 0; i < LIST_ITEM_COUNT; ++i) {
		hwnd = state->addrHwnds[i];
		if (IS_VALID_HWND(hwnd)) {
			DestroyWindow(hwnd);
			state->addrHwnds[i] = HWND_INVALID;
		}
		hwnd = state->containerHwnds[i];
		if (IS_VALID_HWND(hwnd)) {
			DestroyWindow(hwnd);
			state->containerHwnds[i] = HWND_INVALID;
		}
		hwnd = state->itemTitleHwnds[i];
		if (IS_VALID_HWND(hwnd)) {
			DestroyWindow(hwnd);
			state->itemTitleHwnds[i] = HWND_INVALID;
		}
		hwnd = state->bgHwnds[i];
		if (IS_VALID_HWND(hwnd)) {
			DestroyWindow(hwnd);
			state->bgHwnds[i] = HWND_INVALID;
		}
	}
	if (IS_VALID_HWND(state->titleHwnd)) {
		DestroyWindow(state->titleHwnd);
		state->titleHwnd = HWND_INVALID;
	}
}

static PROC_RET DialogProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	ReceiveAddrListState_t *state;
	switch (message) {
		case MSG_INITDIALOG: {
			db_msg("MSG_INITDIALOG");
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			state = (ReceiveAddrListState_t *) lParam;
			onInit(state, hDlg);
		}
			break;
		case MSG_PAINT: {
			state = (ReceiveAddrListState_t *) GetWindowAdditionalData(hDlg);
			db_msg("MSG_PAINT hDlg:%d", hDlg);
			updateUI(state, hDlg);
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("MSG_KEYDOWN:%u", wParam);
			state = (ReceiveAddrListState_t *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;
		case MSG_DESTROY: {
			db_msg("MSG_DESTROY");
			state = (ReceiveAddrListState_t *) GetWindowAdditionalData(hDlg);
			destroyHwnd(state);
		}
			break;
		default:
			break;
	}
	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void freeMemory(ReceiveAddrListState_t *state) {
	if (state->shadowDownBitmap.bmBits) {
		UnloadBitmap(&state->shadowDownBitmap);
	}
	if (state->shadowUpBitmap.bmBits) {
		UnloadBitmap(&state->shadowUpBitmap);
	}
	if (state->middleBitmap.bmBits) {
		UnloadBitmap(&state->middleBitmap);
	}
}

int showAddrList(HWND parent, int maxIndex, int newIndex, char *inoutAddress, int *inoutIndex, gen_index_address_func gen_func, void *user) {
	ReceiveAddrListState_t _state;
	ReceiveAddrListState_t *state = &_state;
	DLGTEMPLATE dlgTemplate;

	db_msg("mParent:%d maxindex:%d init index:%d address:%s", parent, maxIndex, *inoutIndex, inoutAddress);

	int index = *inoutIndex;
	if (maxIndex > 0x7FFFFFF0) maxIndex = 0x7FFFFFF0;
	if (maxIndex < LIST_ITEM_COUNT || index < 0 || index > maxIndex) {
		db_error("invalid index:%d maxindex:%d", index, maxIndex);
		return -1;
	}

	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	memset(state, 0, sizeof(ReceiveAddrListState_t));

	state->gen_func = gen_func;
	state->user = user;
	state->maxIndex = maxIndex;
	state->curIndex = index;
	state->newIndex = newIndex;
	state->outAddress = inoutAddress;
	state->outIndex = inoutIndex;

	strlcpy(state->address[LIST_ITEM_SELECT_INDEX].address, inoutAddress, ADDRESS_BUFFER_SIZE);
	state->address[LIST_ITEM_SELECT_INDEX].index = (uint32_t) index;
	int i;
	for (i = 0; i < LIST_ITEM_SELECT_INDEX; i++) {
		genNewAddress(state, i, index - LIST_ITEM_SELECT_INDEX + i);
	}
	for (i = LIST_ITEM_SELECT_INDEX + 1; i < LIST_ITEM_COUNT; i++) {
		genNewAddress(state, i, index - LIST_ITEM_SELECT_INDEX + i);
	}

	const char *confstr;
	int retval;
	res_getBmp(MK_addr_list, "shadow_up_img", &state->shadowUpBitmap);
	res_getBmp(MK_addr_list, "middle_item_bg_img", &state->middleBitmap);
	res_getBmp(MK_addr_list, "shadow_down_img", &state->shadowDownBitmap);

	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	confstr = res_getString2(MK_addr_list, "pos");
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
	retval = DialogBoxIndirectParam(&dlgTemplate, parent, DialogProc, (LPARAM) state);
	db_msg("add list ret:%d", retval);
	freeMemory(state);
	return retval;
}
