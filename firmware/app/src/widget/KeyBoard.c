#define LOG_TAG "KeyBoard"

#include <string.h>
#include <widgets.h>
#include <minigui_comm.h>
#include "common.h"
#include "minigui_comm.h"
#include "resource.h"
#include "key_event.h"
#include "words_util.h"
#include "bip39_english.h"
#include "Dialog.h"
#include "PicDialog.h"
#include "rand.h"

#define KEYID_INDEX_0 600
#define IDC_TITLE        700
#define IDC_REMIND_TEXT 701
#define IDC_HIDE_CODE 702
#define IDC_TEXT_FIELD 703
#define IDC_TEXT_FIELD_LINE 704

#define FULL_KEYBOARK_ITEM_SIZE 33
#define NUMBER_KEYBOARK_ITEM_SIZE 12
#define CHAR_KEYBOARK_ITEM_SIZE 29
#define ASS_KB_ITEMS_SIZE_DEFAULT 10
#define ASS_KB_COLUMN_CNT_MAX 2

#define KEYBOARD_TYPE_LOWERCASE 0
#define KEYBOARD_TYPE_CAP 1
#define KEYBOARD_TYPE_NUMANDPUNC 2
#define KEYBOARD_TYPE_PUNC 3

#define CODE_CAPS 0x1
#define CODE_TYPE 0x2
#define CODE_OK 0x4
#define CODE_DEL 0x5
#define CODE_BACK 0x6
#define CODE_TYPE1 0x7
#define IS_CTRL_KEY(c) ((c)<0x20)

typedef enum {
	KEY_DIRECTOR_LEFT = 0,
	KEY_DIRECTOR_RIGHT = 1,
	KEY_DIRECTOR_UP = 2,
	KEY_DIRECTOR_DOWN = 3,
} KEY_DIRECTOR;

typedef struct {
	int ass_index; 
	int cur_show_style; 
	int itemSize; 
	int rowCnt;  
	int columnCnt; 
	int fillStartIndex; 
	int fillEndIndex; 
	int fillStartRow;  
	Range range; 
	const char *prefix; 
	label_set_param item_set;
	BITMAP *bgImg;
	BITMAP *highlightImg;
	BITMAP *backImg;
	BITMAP *backHighlightImg;
} AssKeyboardState;

typedef struct {
	int style; 
	int type; 
	int index;
	int itemsize;
	const char (*neighbor)[4];
	BITMAP *images;
	BITMAP *images_active;
	BITMAP *hidecode_img;
	BITMAP *textfield_line_img;
	const char *item_code;
	KeyBoardConfig_t *config;
	char *result;
	int result_limit;
	int result_len;
	int caps;
	AssKeyboardState *ass_state;
} keyBoardState;

#define N ((char)-1)

// {left, right, up, down}
static const char gItemStyleNeighbor_Full[FULL_KEYBOARK_ITEM_SIZE][4] = {
		{9,  1,  28, 10}, //0 q
		{0,  2,  29, 11}, //1 w
		{1,  3,  29, 12}, //2 e
		{2,  4,  30, 13}, //3 r
		{3,  5,  30, 14}, //4 t
		{4,  6,  30, 15}, //5 y
		{5,  7,  31, 16}, //6 u
		{6,  8,  31, 17}, //7 i
		{7,  9,  32, 18}, //8 o
		{8,  0,  32, 18}, //9 p

		{18, 11, 0,  19},//10 a
		{10, 12, 1,  20},//11 s
		{11, 13, 2,  21},//12 d
		{12, 14, 3,  22},//13 f
		{13, 15, 4,  23},//14 g
		{14, 16, 5,  24},//15 h
		{15, 17, 6,  25},//16 j
		{16, 18, 7,  26},//17 k
		{17, 10, 8,  27},//18 l

		{27, 20, 10, 28},//19 caps
		{19, 21, 11, 29},//20 z
		{20, 22, 12, 30},//21 x
		{21, 23, 13, 30},//22 c
		{22, 24, 14, 30},//23 v
		{23, 25, 15, 30},//24 b
		{24, 26, 16, 31},//25 n
		{25, 27, 17, 32},//26 m
		{26, 19, 18, 32}, //27 <-

		{32, 29, 19, 0}, //28 123
		{28, 30, 20, 1}, //29 ,
		{29, 31, 22, 4}, //30 space
		{30, 32, 25, 6}, //31 .
		{31, 28, 27, 9}, //32 Enter
};

static const char gItemCode_Full_0[FULL_KEYBOARK_ITEM_SIZE] = {
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
		'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
		CODE_CAPS, 'z', 'x', 'c', 'v', 'b', 'n', 'm', CODE_DEL,
		CODE_TYPE, ',', ' ', '.', CODE_OK
};

static const char gItemCode_Full_1[FULL_KEYBOARK_ITEM_SIZE] = {
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'~', '*', '@', '[', ']', '(', ')', '{', '}',
		CODE_TYPE1, '?', '!', ':', ';', '\'', '"', '/', CODE_DEL,
		CODE_TYPE, '%', ' ', '&', CODE_OK
};

static const char gItemCode_Full_2[FULL_KEYBOARK_ITEM_SIZE] = {
		'#', '+', '=', '_', '-', '$', '<', '>', '\\', '|',
		'^', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		CODE_TYPE1, ' ', ' ', ' ', ' ', ' ', ' ', ' ', CODE_DEL,
		CODE_TYPE, '%', ' ', '&', CODE_OK
};

static const char gItemStyleNeighbor_Number[NUMBER_KEYBOARK_ITEM_SIZE][4] = {
		{2,  1,  9,  3}, //0 1
		{0,  2,  10, 4}, //1 2
		{1,  0,  11, 5}, //2 3

		{5,  4,  0,  6}, //3 4
		{3,  5,  1,  7}, //4 5
		{4,  3,  2,  8}, //5 6

		{8,  7,  3,  9}, //6 7
		{6,  8,  4,  10}, //7 8
		{7,  6,  5,  11}, //8 9

		{11, 10, 6,  0}, //9 <-
		{9,  11, 7,  1},//10 0
		{10, 9,  8,  2},//11 ok
};


static const char gItemCode_NUMBER_DEFAULT0[NUMBER_KEYBOARK_ITEM_SIZE] = {
		'1', '2', '3',
		'4', '5', '6',
		'7', '8', '9',
		CODE_DEL, '0', CODE_OK
};

static char gItemCode_NUMBER_DEFAULT[NUMBER_KEYBOARK_ITEM_SIZE] = {
		'1', '2', '3',
		'4', '5', '6',
		'7', '8', '9',
		CODE_DEL, '0', CODE_OK
};

static const char gImgIndexMap_NUMBER0[NUMBER_KEYBOARK_ITEM_SIZE] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9,
		10, 0, 11};

static char gImgIndexMap_NUMBER[NUMBER_KEYBOARK_ITEM_SIZE] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9,
		10, 0, 11};


static const char gItemCode_CHAR[CHAR_KEYBOARK_ITEM_SIZE] = {
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
		'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
		'z', 'x', 'c', 'v', 'b', 'n', 'm',
		CODE_BACK, CODE_OK, CODE_DEL
};

// {left, right, up, down}
static const char gItemStyleNeighbor_CHAR[CHAR_KEYBOARK_ITEM_SIZE][4] = {
		{9,  1,  26, 10}, //0 q
		{0,  2,  26, 11}, //1 w
		{1,  3,  26, 12}, //2 e
		{2,  4,  27, 13}, //3 r
		{3,  5,  27, 14}, //4 t
		{4,  6,  27, 15}, //5 y
		{5,  7,  28, 16}, //6 u
		{6,  8,  28, 17}, //7 i
		{7,  9,  28, 18}, //8 o
		{8,  0,  28, 18}, //9 p

		{18, 11, 0,  19},//10 a
		{10, 12, 1,  19},//11 s
		{11, 13, 2,  20},//12 d
		{12, 14, 3,  21},//13 f
		{13, 15, 4,  22},//14 g
		{14, 16, 5,  23},//15 h
		{15, 17, 6,  24},//16 j
		{16, 18, 7,  25},//17 k
		{17, 10, 8,  25},//18 l

		{25, 20, 11, 26},//19 z
		{19, 21, 12, 26},//20 x
		{20, 22, 13, 27},//21 c
		{21, 23, 14, 27},//22 v
		{22, 24, 15, 27},//23 b
		{23, 25, 16, 28},//24 n
		{24, 19, 17, 28},//25 m

		{28, 27, 19, 1},//26 back
		{26, 28, 22, 4}, //27 ok
		{27, 26, 25, 8}, //28 <-
};

#undef N

static char key_img_path[64];
static const char *key_img_path_prefix = NULL;

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

static int getAssKeyboardItemID(int index, keyBoardState *state, int img) {
	if (img) {
		return (state->itemsize + 2 * index);
	} else {
		return (state->itemsize + 2 * index + 1);
	}
}

static void freeAssKeyboardBitmap(AssKeyboardState *state) {
	if (state->backImg) {
		if (state->backImg->bmBits) {
			UnloadBitmap(state->backImg);
		}
		free(state->backImg);
	}
	if (state->backHighlightImg) {
		if (state->backHighlightImg->bmBits) {
			UnloadBitmap(state->backHighlightImg);
		}
		free(state->backHighlightImg);
	}
	if (state->bgImg) {
		if (state->bgImg->bmBits) {
			UnloadBitmap(state->bgImg);
		}
		free(state->bgImg);
	}
	if (state->highlightImg) {
		if (state->highlightImg->bmBits) {
			UnloadBitmap(state->highlightImg);
		}
		free(state->highlightImg);
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

static int updateFullKeyboardState(HWND hDlg, keyBoardState *state, int show) {
	int id = 0;
	HWND hwnd = HWND_INVALID;
	for (int i = 0; i < state->itemsize; ++i) {
		id = i + KEYID_INDEX_0;
		hwnd = GetDlgItem(hDlg, id);
		if (IS_VALID_HWND(hwnd)) {
			ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
		}
	}
	if (show) {
		hwnd = GetDlgItem(hDlg, IDC_TEXT_FIELD);
		if (IS_VALID_HWND(hwnd)) {
			SetWindowText(hwnd, state->result);
		}
	}
	return 0;
}

static int getStyleFromState(keyBoardState *state) {
	if (state->style == KEYBOARD_STYLE_ASSN) {
		return KEYBOARD_STYLE_CHAR;
	}
	return state->style;
}

static int showAssKeyboard(HWND hDlg, const char *prefix, keyBoardState *state) {
	int count = getCntOfRange(&(state->ass_state->range));
	db_msg("search result count:%d startIndex:%d endIndex:%d", count, state->ass_state->range.startIndex, state->ass_state->range.endIndex);
	int id = 0;
	HWND hwnd = HWND_INVALID;
	int showRowCnt = 0;
	showRowCnt = (count + 1) / state->ass_state->columnCnt;
	if ((count + 1) % state->ass_state->columnCnt) {
		showRowCnt++;
	}
	int invisibleRowCnt = state->ass_state->rowCnt - showRowCnt;
	int fillStartIndex = invisibleRowCnt * state->ass_state->columnCnt;
	state->ass_state->fillStartIndex = fillStartIndex;
	state->ass_state->fillEndIndex = fillStartIndex + count - 1;
	if (count <= 0) {
		state->ass_state->ass_index = state->ass_state->itemSize - 1;
	} else {
		state->ass_state->ass_index = fillStartIndex + count / 2;
	}
	state->ass_state->fillStartRow = state->ass_state->rowCnt - showRowCnt;
	db_msg("wordCnt:%d showRowCnt:%d invisibleRowCnt:%d startIndex:%d", count, showRowCnt, invisibleRowCnt, fillStartIndex);
	for (int i = 0; i < state->ass_state->itemSize; ++i) {
		id = getAssKeyboardItemID(i, state, 1) + KEYID_INDEX_0;
		hwnd = GetDlgItem(hDlg, id);
		if (IS_VALID_HWND(hwnd)) {
			if ((i / state->ass_state->columnCnt) < invisibleRowCnt) {
				ShowWindow(hwnd, SW_HIDE);
			} else {
				ShowWindow(hwnd, SW_SHOW);
			}

			BITMAP *img = NULL;
			if (state->ass_state->ass_index == i) {
				img = (i == state->ass_state->itemSize - 1) ? state->ass_state->backHighlightImg : state->ass_state->highlightImg;
			} else {
				img = (i == state->ass_state->itemSize - 1) ? state->ass_state->backImg : state->ass_state->bgImg;
			}
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) img, 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}

		id = getAssKeyboardItemID(i, state, 0) + KEYID_INDEX_0;
		hwnd = GetDlgItem(hDlg, id);
		if (IS_VALID_HWND(hwnd)) {
			if ((i / state->ass_state->columnCnt) < invisibleRowCnt) {
				ShowWindow(hwnd, SW_HIDE);
			} else {
				ShowWindow(hwnd, SW_SHOW);
				if ((i == (state->ass_state->itemSize - 1)) ||
				    ((state->ass_state->range.startIndex + i - fillStartIndex) > state->ass_state->range.endIndex)) {
					SetWindowText(hwnd, "");
				} else {
					SetWindowText(hwnd, wordlist[state->ass_state->range.startIndex + (i - fillStartIndex)]);
				}
			}
		}
	}

	return 0;
}

static int hideAssKeyboard(HWND hDlg, keyBoardState *state) {
	HWND hwnd = HWND_INVALID;
	for (int i = 0; i < state->ass_state->itemSize; ++i) {
		hwnd = GetDlgItem(hDlg, getAssKeyboardItemID(i, state, 1) + KEYID_INDEX_0);
		if (IS_VALID_HWND(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}

		hwnd = GetDlgItem(hDlg, getAssKeyboardItemID(i, state, 0) + KEYID_INDEX_0);
		if (IS_VALID_HWND(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
	}
	return 0;
}

static BITMAP *loadKeyBitMap(keyBoardState *state, int index, int active, int reload) {
	if (key_img_path_prefix == NULL) {
		key_img_path_prefix = res_getString2(MK_kb, "img_path_prefix");
	}
	BITMAP *img = NULL;
	if (active == 1) {
		img = &state->images_active[index];
	} else {
		img = &state->images[index];
	}

	if (reload) {
		if (state->images[index].bmBits != NULL) {
			UnloadBitmap(&state->images[index]);
		}
		if (state->images_active[index].bmBits != NULL) {
			UnloadBitmap(&state->images_active[index]);
		}
	} else if (img->bmBits) {
		db_msg("index:%d active:%d loaded", index, active);
		return img;
	}

	int style = getStyleFromState(state);
	if (active == 1) {
		snprintf(key_img_path, sizeof(key_img_path), "%s/k%d%d%02d_active.png", key_img_path_prefix, style, state->type, index);
	} else {
		snprintf(key_img_path, sizeof(key_img_path), "%s/k%d%d%02d.png", key_img_path_prefix, style, state->type, index);
	}
	db_msg("index:%d path:%s", index, key_img_path);
	int ret = LoadBitmapFromFile(HDC_SCREEN, img, key_img_path);
	if (ret != ERR_BMP_OK) {
		db_error("load bitmap path:%s failed", key_img_path);
	}
	return img;
}

static int setKeyItemBitMap(keyBoardState *state, HWND hDlg, int index, int active, int reload) {
	HWND hwnd = GetDlgItem(hDlg, KEYID_INDEX_0 + index);
	if (IS_VALID_HWND(hwnd)) {
		if (active == -1) {
			active = state->index == index ? 1 : 0;
		}

		if (state->config->keyStyle == KEYBOARD_STYLE_PIN) {
			if (index < 0 || index >= NUMBER_KEYBOARK_ITEM_SIZE) {
				db_error("number keyboard out range index:%d", index);
				return -1;
			}
			int des_index = 0;
			des_index = gImgIndexMap_NUMBER[index];
			db_debug("src index:%d, des index:%d", index, des_index);
			index = des_index;
		}

		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) loadKeyBitMap(state, index, active, reload), 0);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	return 0;
}

static int updateImg(keyBoardState *state, HWND hDlg, int index) {
	HWND hwnd = GetDlgItem(hDlg, index);
	if (!IS_VALID_HWND(hwnd)) {
		db_error("unvalid hwnd:%d id:%d", hDlg, index);
		return -1;
	}
	BITMAP *img = NULL;
	int hc = 0;
	switch (index) {
		case IDC_HIDE_CODE: {
			img = state->hidecode_img;
			if (state->result_len < 0) {
				hc = 0;
			} else if (state->result_len > state->result_limit) {
				hc = state->result_limit;
			} else {
				hc = state->result_len;
			}
			snprintf(key_img_path, sizeof(key_img_path), "%s/hc%d.png", key_img_path_prefix, hc);
			db_msg("hide code len:%d path:%s", state->result_len, key_img_path);
			break;
		}
		case IDC_TEXT_FIELD_LINE: {
			img = state->textfield_line_img;
			snprintf(key_img_path, sizeof(key_img_path), "%s/%s.png", key_img_path_prefix, "text_field_line");
			db_msg("text field line img path:%s", key_img_path);
			break;
		}
		default:
			break;
	}
	if (!img) {
		db_debug("state img false index:%d", index);
		return -1;
	}

	int ret = LoadBitmapFromFile(HDC_SCREEN, img, key_img_path);
	if (ret != ERR_BMP_OK) {
		db_error("load bitmap path:%s failed", key_img_path);
		ShowWindow(hwnd, SW_HIDE);
	} else {
		db_msg("load bitmap path:%s OK", key_img_path);
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) img, 0);
		InvalidateRect(hwnd, NULL, TRUE);
		ShowWindow(hwnd, SW_SHOW);
	}
	return 0;
}

static int getRowRange(keyBoardState *state, int columnIndex, int *minRow, int *maxRow) {
	if (NULL == minRow || NULL == maxRow) {
		db_error("invalid paras minRow:%p maxRow:%p", minRow, maxRow);
		return -1;
	}
	int count = getCntOfRange(&(state->ass_state->range));
	if (count <= 0) {
		*maxRow = state->ass_state->rowCnt - 1;
		*minRow = state->ass_state->rowCnt - 1;
		return 0;
	}
	//
	db_msg("count:%d totalRowCnt:%d", count, state->ass_state->rowCnt);
	if (count % 2) {
		*maxRow = state->ass_state->rowCnt - 1;
	} else {
		if (columnIndex == 0) {
			*maxRow = state->ass_state->rowCnt - 2;
		} else {
			*maxRow = state->ass_state->rowCnt - 1;
		}
	}
	*minRow = state->ass_state->fillStartRow;
	db_msg("minRow:%d maxRow:%d", *minRow, *maxRow);

	return 0;
}

static int getColumnRange(keyBoardState *state, int rowIndex, int *minCol, int *maxCol) {
	if (NULL == minCol || NULL == maxCol) {
		db_error("invalid paras minCol:%p maxCol:%p", minCol, maxCol);
		return -1;
	}
	int count = getCntOfRange(&(state->ass_state->range));
	if (count <= 0) {
		*minCol = state->ass_state->columnCnt - 1;
		*maxCol = state->ass_state->columnCnt - 1;
		return 0;
	}
	if (count % 2) {
		*minCol = 0;
	} else {
		if (rowIndex == (state->ass_state->rowCnt - 1)) {
			*minCol = 1;
		} else {
			*minCol = 0;
		}
	}
	*maxCol = state->ass_state->columnCnt - 1;

	return 0;
}

static int onKeyMove(keyBoardState *state, HWND hDlg, int act, int direction) {
	int newindex = 0;
	int showStyle;
	HWND hwnd = HWND_INVALID;
	int curIndex = 0;
	int count = 0;
	int starIndex = -1;
	int endIndex = -1;
	int nextRow = 0;
	int nextColumn = 0;
	BITMAP *img = NULL;
	int rowIndex = 0;
	if (state->style == KEYBOARD_STYLE_ASSN && state->ass_state->cur_show_style == KEYBOARD_STYLE_ASSN) {
		if (act == 1) { //
			return 0;
		}
		count = getCntOfRange(&(state->ass_state->range));
		if (count <= 0) {
			return 0;
		}

		curIndex = state->ass_state->ass_index;
		rowIndex = state->ass_state->ass_index / state->ass_state->columnCnt;

		db_msg("onKeyMove curIndex:%d rowIndex:%d", curIndex, rowIndex);
		switch (direction) {
			case KEY_DIRECTOR_LEFT: {
				db_msg("KEY_DIRECTOR_LEFT");
				nextColumn = (curIndex % state->ass_state->columnCnt) - 1;
				getColumnRange(state, curIndex / state->ass_state->columnCnt, &starIndex, &endIndex);
				if (nextColumn < starIndex) {
					nextColumn = endIndex;
				}
				newindex = rowIndex * state->ass_state->columnCnt + nextColumn;
			}
				break;
			case KEY_DIRECTOR_RIGHT: {
				db_msg("KEY_DIRECTOR_RIGHT");
				nextColumn = (curIndex % state->ass_state->columnCnt) + 1;
				getColumnRange(state, curIndex / state->ass_state->columnCnt, &starIndex, &endIndex);
				if (nextColumn > endIndex) {
					nextColumn = starIndex;
				}
				newindex = rowIndex * state->ass_state->columnCnt + nextColumn;
			}
				break;
			case KEY_DIRECTOR_UP: {
				db_msg("KEY_DIRECTOR_UP");
				nextRow = rowIndex - 1;
				getRowRange(state, curIndex % state->ass_state->columnCnt, &starIndex, &endIndex);
				db_msg("maxRow:%d minRow:%d", endIndex, starIndex);
				if (nextRow < starIndex) {
					nextRow = endIndex;
				}
				newindex = nextRow * state->ass_state->columnCnt + curIndex % state->ass_state->columnCnt;
			}
				break;
			case KEY_DIRECTOR_DOWN: {
				db_msg("KEY_DIRECTOR_DOWN");
				nextRow = rowIndex + 1;
				getRowRange(state, curIndex % state->ass_state->columnCnt, &starIndex, &endIndex);
				db_msg("maxRow:%d minRow:%d", starIndex, endIndex);
				if (nextRow > endIndex) {
					nextRow = starIndex;
				}
				newindex = nextRow * state->ass_state->columnCnt + curIndex % state->ass_state->columnCnt;
			}
				break;
			default:
				break;
		}
		db_msg("newIndex:%d", newindex);
		if (curIndex < state->ass_state->itemSize - 1) {
			img = state->ass_state->bgImg;
		} else {
			img = state->ass_state->backImg;
		}
		hwnd = GetDlgItem(hDlg, getAssKeyboardItemID(curIndex, state, 1) + KEYID_INDEX_0);
		if (IS_VALID_HWND(hwnd)) {
			db_msg("update img current img:%p hwnd:%d", img, hwnd);
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) img, 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		state->ass_state->ass_index = newindex;
		if (newindex < state->ass_state->itemSize - 1) {
			img = state->ass_state->highlightImg;
		} else {
			img = state->ass_state->backHighlightImg;
		}
		hwnd = GetDlgItem(hDlg, getAssKeyboardItemID(newindex, state, 1) + KEYID_INDEX_0);
		if (IS_VALID_HWND(hwnd)) {
			db_msg("update img newIndex img:%p hwnd:%d", img, hwnd);
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) img, 0);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		return 0;
	}

	if (act == 0) { //down
		setKeyItemBitMap(state, hDlg, state->index, 0, 0);
		db_msg("neighbor index:%d direction:%d", state->index, direction);
		newindex = state->neighbor[state->index][direction];
		db_msg("new index：%d", newindex);
		if (newindex >= 0 && newindex < state->itemsize) {
			state->index = newindex;
		}
	} else {
		db_msg("direction:%d up index:%d", direction, state->index);
		setKeyItemBitMap(state, hDlg, state->index, 1, 0);
	}

	return 0;
}

static int set_hide_more_result(const keyBoardState *state, HWND hwnd) {
	char txtbuf[28];
	int hidelen = 0;
	int len = strlen(state->result);
	db_msg("show flag:%d result:%s", state->config->flag, state->result);
	if (state->config->flag & KEYBOARD_FLAG_HIDE_MORE) {
		hidelen = 11;
		if (state->config->flag & KEYBOARD_FLAG_MULTI_LINE) {
			hidelen = 23;
		}
	}
	if (!hidelen || len <= hidelen) {
		SetWindowText(hwnd, state->result);
	} else {
		txtbuf[0] = state->result[0];
		txtbuf[1] = '.';
		txtbuf[2] = '.';
		txtbuf[3] = '.';
		strcpy(txtbuf + 4, state->result + len - (hidelen - 4));
		SetWindowText(hwnd, txtbuf);
	}
	return 0;
}

static int onClickKey(keyBoardState *state, HWND hDlg) {
	char code = state->item_code[state->index];
	if (state->caps && code >= 'a' && code <= 'z') {
		code = code - ('a' - 'A');
	}
	int reload = 0;
	int flushresult = 0;
	int ret = 0;
	int assEnable = 0;
	const char *name = NULL;
	const char *str = NULL;
	HWND hwnd;
	if (state->style == KEYBOARD_STYLE_ASSN && state->ass_state->cur_show_style == KEYBOARD_STYLE_ASSN) {
		if (state->ass_state->ass_index == (state->ass_state->itemSize - 1)) {
			state->ass_state->cur_show_style = KEYBOARD_STYLE_FULL;
			state->result_len--;
			state->result[state->result_len] = '\0';
			updateFullKeyboardState(hDlg, state, 1);
			setKeyItemBitMap(state, hDlg, state->index, 0, 0);
			state->index = get_random_keyboard_index(KEYBOARD_STYLE_ASSN); // use random index
			setKeyItemBitMap(state, hDlg, state->index, 1, 0);
			hideAssKeyboard(hDlg, state);
		} else {
			str = wordlist[state->ass_state->ass_index - state->ass_state->fillStartIndex + state->ass_state->range.startIndex];
			memcpy(state->config->result, str, strlen(str) + 1);
			state->result_len = strlen(str) + 1;
			db_msg("select word:%s len:%d", str, state->result_len);
			EndDialog(hDlg, state->result_len);
		}
		return 0;
	}

	if (IS_CTRL_KEY(code)) {
		switch (code) {
			case CODE_DEL: {
				if (state->result_len > 0) {
					state->result_len--;
					state->result[state->result_len] = '\0';
					flushresult = 1;
				}
			}
				break;
			case CODE_TYPE: {
				if ((state->type==KEYBOARD_TYPE_NUMANDPUNC) || (state->type==KEYBOARD_TYPE_PUNC)) {
					state->type = state->caps ? KEYBOARD_TYPE_CAP : KEYBOARD_TYPE_LOWERCASE;
					state->item_code = gItemCode_Full_0;
				} else {
					state->type = KEYBOARD_TYPE_NUMANDPUNC;
					state->item_code = gItemCode_Full_1;
				}
				reload = 1;

				break;
			}
			case CODE_TYPE1: {
				if (state->type == KEYBOARD_TYPE_NUMANDPUNC) {
					state->type = KEYBOARD_TYPE_PUNC;
					state->item_code = gItemCode_Full_2;
				} else {
					state->type = KEYBOARD_TYPE_NUMANDPUNC;
					state->item_code = gItemCode_Full_1;
				}
				reload = 1;
				break;
			}
			case CODE_OK: {
				if (state->result_len <= 0) {
					return 0;
				}
				if (state->style == KEYBOARD_STYLE_ASSN) {
					if (!checkWord(state->result)) {
						picDialog(hDlg, "word_not_exit_alert", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
						return 0;
					}
					EndDialog(hDlg, state->result_len);
				} else {
					EndDialog(hDlg, state->result_len);
				}
			}
				break;
			case CODE_CAPS: {
				state->caps = state->caps ? 0 : 1;
				state->type = state->caps ? KEYBOARD_TYPE_CAP : KEYBOARD_TYPE_LOWERCASE;
				reload = 1;
			}
				break;
			case CODE_BACK: {
				EndDialog(hDlg, KEY_EVENT_BACK);
			}
				break;
		}
		if (reload) {
			for (int i = 0; i < state->itemsize; i++) {
				setKeyItemBitMap(state, hDlg, i, -1, 1);
			}
		}
	} else {
		if (state->result_len >= state->result_limit) {
			db_error("input full,len:%d limit:%d", state->result_len, state->result_limit);
			return 0;
		}
		flushresult = 1;
		assEnable = 1;
		
		if(state->type == KEYBOARD_TYPE_PUNC){//select 
			if((state->index>=11) && (state->index<=18)){
				//invalid
			}else if((state->index>=20) && (state->index<=26)){
				//invalid
			}else{
				state->result[state->result_len++] = code;
			}
		}else{
			state->result[state->result_len++] = code;
		}
	}
	if (flushresult) {
		db_msg("result len:%d data:%s", state->result_len, state->result);
		switch (state->style) {
			case KEYBOARD_STYLE_CHAR:
			case KEYBOARD_STYLE_FULL:
			case KEYBOARD_STYLE_ASSN: {
				hwnd = GetDlgItem(hDlg, IDC_TEXT_FIELD);
				if (state->config->flag & KEYBOARD_FLAG_HIDE_MORE) {
					set_hide_more_result(state, hwnd);
				} else {
					SetWindowText(hwnd, state->result);
				}
				break;
			}
			case KEYBOARD_STYLE_PIN:
				updateImg(state, hDlg, IDC_HIDE_CODE);
				break;
		}
		if (state->style != KEYBOARD_STYLE_ASSN) {
			return 0;
		}
		if (!strlen(state->result) || !assEnable) {
			return 0;
		}
		int count = getWordRange(state->result, &(state->ass_state->range));
		if (count > (state->ass_state->itemSize - 1)) {
			state->ass_state->cur_show_style = KEYBOARD_STYLE_CHAR;
			updateFullKeyboardState(hDlg, state, 1);
			return 0;
		}
		if (count <= 0) {
			setKeyItemBitMap(state, hDlg, state->index, 0, 0);
			state->index = CHAR_KEYBOARD_DEFAULT_SELECT_INDEX;
			setKeyItemBitMap(state, hDlg, state->index, 1, 0);
			picDialog(hDlg, "word_not_exit_alert", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
//			dialog_alert(hDlg, DIALOG_ICON_STYLE_ERR, rm->getLabel(LANG_LABEL_MNEMONIC_NOT_EXIST), rm->getLabel(LANG_LABEL_RE_ENTER));
			state->result_len--; 
			state->result[state->result_len] = '\0';
			updateFullKeyboardState(hDlg, state, 1);
		} else {
			state->ass_state->cur_show_style = KEYBOARD_STYLE_ASSN;
			updateFullKeyboardState(hDlg, state, 0);
			showAssKeyboard(hDlg, state->result, state);
		}
	}
	return 0;
}

static int onKeyDown(keyBoardState *state, HWND hDlg, WPARAM code) {
	switch (code) {
		case INPUT_KEY_OK:
			return onClickKey(state, hDlg);
		case INPUT_KEY_LEFT:
			return onKeyMove(state, hDlg, 0, KEY_DIRECTOR_LEFT);
		case INPUT_KEY_RIGHT:
			return onKeyMove(state, hDlg, 0, KEY_DIRECTOR_RIGHT);
		case INPUT_KEY_UP:
			return onKeyMove(state, hDlg, 0, KEY_DIRECTOR_UP);
		case INPUT_KEY_DOWN:
			return onKeyMove(state, hDlg, 0, KEY_DIRECTOR_DOWN);
		default:
			break;
	}
	return 0;
}

static int onKeyUp(keyBoardState *state, HWND hDlg, WPARAM code) {
	switch (code) {
		case INPUT_KEY_OK:
			break;
		case INPUT_KEY_LEFT:
			return onKeyMove(state, hDlg, 1, KEY_DIRECTOR_LEFT);
		case INPUT_KEY_RIGHT:
			return onKeyMove(state, hDlg, 1, KEY_DIRECTOR_RIGHT);
		case INPUT_KEY_UP:
			return onKeyMove(state, hDlg, 1, KEY_DIRECTOR_UP);
		case INPUT_KEY_DOWN:
			return onKeyMove(state, hDlg, 1, KEY_DIRECTOR_DOWN);
		case INPUT_KEY_ESC:
			EndDialog(hDlg, KEY_EVENT_ABORT);
		default:
			break;
	}
	return 0;
}

static int onInitAssKeyboard(HWND hDlg, keyBoardState *state) {
	HWND hwnd = HWND_INVALID;
	int id = 0;
	label_string _label_str;
	label_string *label_str = &_label_str;
	BITMAP *img = NULL;
	for (int i = 0; i < state->ass_state->itemSize; ++i) {
		id = getAssKeyboardItemID(i, state, 1) + KEYID_INDEX_0;
		hwnd = GetDlgItem(hDlg, id);
		if (IS_VALID_HWND(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
		if (i < state->ass_state->itemSize - 1) {
			img = state->ass_state->bgImg;
		} else {
			img = state->ass_state->backImg;
		}
		SendMessage(hwnd, STM_SETIMAGE, (WPARAM) img, 0);
		InvalidateRect(hwnd, NULL, TRUE);

		id = getAssKeyboardItemID(i, state, 0) + KEYID_INDEX_0;
		label_str->font = state->ass_state->item_set.font;
		label_str->style = state->ass_state->item_set.style;
		setDialogLabelStyle(hDlg, id, label_str);

		hwnd = GetDlgItem(hDlg, id);
		if (IS_VALID_HWND(hwnd)) {
			ShowWindow(hwnd, SW_HIDE);
		}
	}
	return 0;
}

static PROC_RET KeyBoardProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	keyBoardState *state;
	HWND hwnd;

	switch (message) {
		case MSG_INITDIALOG: {
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			SetWindowBkColor(hDlg, res_getBGColor());
			state = (keyBoardState *) lParam;
			db_msg("state item style:%d size:%d title:%s", state->style, state->itemsize, state->config->title.data);
			for (int i = 0; i < state->itemsize; i++) {
				setKeyItemBitMap(state, hDlg, i, -1, 1);
			}
			if (state->config->editor_style & KEYBOARD_EDITOR_HIDECODE) {
				updateImg(state, hDlg, IDC_HIDE_CODE);
			} else {
				updateImg(state, hDlg, IDC_TEXT_FIELD_LINE);
				state->config->text_field.data = state->config->result;
				db_msg("init keyboard dialog text field:%s", state->config->text_field.data);
				setDialogLabelStyle(hDlg, IDC_TEXT_FIELD, &(state->config->text_field));
			}
			setDialogLabelStyle(hDlg, IDC_TITLE, &(state->config->title));
			if (state->style == KEYBOARD_STYLE_ASSN) {
				onInitAssKeyboard(hDlg, state);
			}
		}
			break;
		case MSG_KEYDOWN: {
			db_msg("key down code:%u", wParam);
			state = (keyBoardState *) GetWindowAdditionalData(hDlg);
			onKeyDown(state, hDlg, wParam);
		}
			break;

		case MSG_KEYUP: {
			db_msg("key up code:%u", wParam);
			state = (keyBoardState *) GetWindowAdditionalData(hDlg);
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

static int initState(KeyBoardConfig_t *config, keyBoardState *state, AssKeyboardState *ass_state) {
	memset(state, 0, sizeof(keyBoardState));
	if (ass_state) {
		memset(ass_state, 0, sizeof(AssKeyboardState));
	}

	if (!config->result || config->result_limit < 1) {
		db_error("invalid result buffer");
		return -1;
	}

	state->result = config->result;
	state->result_len = strlen(state->result);
	state->result_limit = config->result_limit;
	state->config = config;
	db_msg("init state result:%s, result_len:%d initKey:%d", state->result, state->result_len, config->initKeyIndex);
	int style = config->keyStyle;
	if (style == KEYBOARD_STYLE_FULL) { 
		state->style = style;
		state->itemsize = FULL_KEYBOARK_ITEM_SIZE;
		state->neighbor = gItemStyleNeighbor_Full;
		state->item_code = gItemCode_Full_0;
		state->index = FULL_KEYBOARD_DEFAULT_SELECT_IDNEX;
		if (config->initKeyIndex > 0 && config->initKeyIndex < FULL_KEYBOARK_ITEM_SIZE) {
			state->index = config->initKeyIndex;
		}
		state->caps = 0;
		state->type = KEYBOARD_TYPE_LOWERCASE;
	} else if (style == KEYBOARD_STYLE_PIN) { 
		state->style = style;
		state->itemsize = NUMBER_KEYBOARK_ITEM_SIZE;
		state->neighbor = gItemStyleNeighbor_Number;
		state->item_code = gItemCode_NUMBER_DEFAULT;
		state->index = 4;
        if (config->initKeyIndex >= 0 && config->initKeyIndex <= 8) {
            state->index = config->initKeyIndex;
        }
	} else if (style == KEYBOARD_STYLE_CHAR || style == KEYBOARD_STYLE_ASSN) {
		state->style = style;
		state->itemsize = CHAR_KEYBOARK_ITEM_SIZE;
		state->neighbor = gItemStyleNeighbor_CHAR;
		state->item_code = gItemCode_CHAR;
		state->index = FULL_KEYBOARD_DEFAULT_SELECT_IDNEX;
		if (config->initKeyIndex > 0 && config->initKeyIndex < CHAR_KEYBOARK_ITEM_SIZE) {
			state->index = config->initKeyIndex;
		}
	} else {
		return -1;
	}

	if (style == KEYBOARD_STYLE_ASSN && ass_state) {
		ass_state->ass_index = 0;
		ass_state->cur_show_style = KEYBOARD_STYLE_FULL;
		ass_state->itemSize = ASS_KB_ITEMS_SIZE_DEFAULT;
		state->ass_state = ass_state;
	}

	return 0;
}

int initAssKeyboard(CTRLDATA *CtrlData, keyBoardState *state) {
	db_msg("initAssKeyboard");
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;
	WIN_RECT screenRect;
	res_getRect(MK_system, "pos", &screenRect);

	const int totalRowCnt = 5;
	WIN_RECT itemRect;
	int id = 0;
	int columnCnt = ASS_KB_COLUMN_CNT_MAX;
	int itemWidth = 0;
	int itemHeight = 0;
	int assKeyboardHeight = res_getInt(MK_kb, "ass_keyboard_height", 0);
	int assItemSpace = res_getInt(MK_kb, "ass_keyboard_item_space", 0);
	state->ass_state->itemSize = totalRowCnt * columnCnt;
	state->ass_state->columnCnt = columnCnt;
	state->ass_state->rowCnt = totalRowCnt;
	state->ass_state->bgImg = newMallocBitMap();
	if (!state->ass_state->bgImg) {
		freeAssKeyboardBitmap(state->ass_state);
		return -1;
	}
	loadBitmap(state->ass_state->bgImg, MK_kb, "ass_keyboard_bg_img");

	state->ass_state->highlightImg = newMallocBitMap();
	if (!state->ass_state->highlightImg) {
		freeAssKeyboardBitmap(state->ass_state);
		return -1;
	}
	loadBitmap(state->ass_state->highlightImg, MK_kb, "ass_keyboard_bg_highlight_img");

	state->ass_state->backImg = newMallocBitMap();
	if (!state->ass_state->backImg) {
		freeAssKeyboardBitmap(state->ass_state);
		return -1;
	}
	loadBitmap(state->ass_state->backImg, MK_kb, "ass_keyboard_back_img");

	state->ass_state->backHighlightImg = newMallocBitMap();
	if (!state->ass_state->backHighlightImg) {
		freeAssKeyboardBitmap(state->ass_state);
		return -1;
	}
	loadBitmap(state->ass_state->backHighlightImg, MK_kb, "ass_keyboard_back_highlight_img");

	itemRect.w = (screenRect.w - assItemSpace * (columnCnt - 1)) / 2;
	itemRect.h = (assKeyboardHeight - (totalRowCnt - 1) * assItemSpace) / totalRowCnt;
	res_getLabelSetParam(label_set, MK_kb, "ass_keyboard_item_txt_config");
	for (int i = 0; i < state->ass_state->itemSize; ++i) {
		itemRect.x = (i % columnCnt) * (assItemSpace + itemRect.w);
		itemRect.y = screenRect.h - assKeyboardHeight + (i / columnCnt) * (itemRect.h + assItemSpace);

		id = getAssKeyboardItemID(i, state, 1);
		initImageCtrlDataWithRect(&CtrlData[id], KEYID_INDEX_0 + id, itemRect);
		id = getAssKeyboardItemID(i, state, 0);
		label_set->x = itemRect.x;
		label_set->y = itemRect.y;
		label_set->w = itemRect.w;
		label_set->h = itemRect.h;
		label_set->y -= 3;
		initStringCtrlData(&CtrlData[id], KEYID_INDEX_0 + id, NULL, label_set);
		db_msg("rect x:%d y:%d w:%d h:%d", itemRect.x, itemRect.y, itemRect.w, itemRect.h);
	}

	return 0;
}

int showKeyBoard(HWND hParentWnd, KeyBoardConfig_t *config) {
	keyBoardState _state;
	keyBoardState *state = &_state;
	AssKeyboardState _ass_state;
	AssKeyboardState *ass_state = &_ass_state;
	char configkey[32];
	const char *confstr;
	int retval;
	DLGTEMPLATE dlgTemplate;
	CTRLDATA *CtrlData = NULL;
	unsigned int controlNrs;
	int text_field_index = 0;
	int text_field_line_index = 0;
	int hidecode_index = 0;
	int title_index = 0;
	int remind_title_index = 0;
	WIN_RECT screenRect;

	CTRLDATA *c;
	int offset_x = 0;
	int offset_y = 0;
	label_set_param _label_set;
	label_set_param *label_set = &_label_set;

	res_getRect(MK_system, "pos", &screenRect);

	if (initState(config, state, ass_state) != 0) {
		db_msg("init state false");
		return -1;
	}
	db_msg("init state OK");
	int i;
	if (config->keyStyle == KEYBOARD_STYLE_PIN) {
		if (config->random) {
			char random_result[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
			char t;
			for (i = 0; i < 10; i++) {
				int r = random32() % 10;
				if (r != i) {
					t = random_result[i];
					random_result[i] = random_result[r];
					random_result[r] = t;
				}
			}
			int random_result_index = 0;
			for (i = 0; i < NUMBER_KEYBOARK_ITEM_SIZE; i++) {
				if ((gItemCode_NUMBER_DEFAULT[i] != CODE_DEL) && (gItemCode_NUMBER_DEFAULT[i] != CODE_OK)) {
					gItemCode_NUMBER_DEFAULT[i] = random_result[random_result_index];
					gImgIndexMap_NUMBER[i] = random_result[random_result_index] - '0';
					random_result_index++;
				}
			}
			db_msg("random number keyboard");
		} else {
			for (i = 0; i < NUMBER_KEYBOARK_ITEM_SIZE; i++) {
				gItemCode_NUMBER_DEFAULT[i] = gItemCode_NUMBER_DEFAULT0[i];
				gImgIndexMap_NUMBER[i] = gImgIndexMap_NUMBER0[i];
			}
		}
	}

	if (key_img_path_prefix == NULL) {
		key_img_path_prefix = res_getString2(MK_kb, "img_path_prefix");
	}

	BITMAP hide_code_img;
	memset(&hide_code_img, 0, sizeof(BITMAP));
	BITMAP textfield_line_img;
	memset(&textfield_line_img, 0, sizeof(BITMAP));
	controlNrs = state->itemsize;

	if (state->style == KEYBOARD_STYLE_ASSN) {
		controlNrs += 2 * state->ass_state->itemSize;
	}
	db_msg("keyboar item controlNrs:%d", controlNrs);

	if (config->editor_style & KEYBOARD_EDITOR_HIDECODE) {
		hidecode_index = controlNrs++;
		state->hidecode_img = &hide_code_img;
	} else {
		text_field_index = controlNrs++;
		text_field_line_index = controlNrs++;
		state->textfield_line_img = &textfield_line_img;
	}

	if (config->title.data) {
		title_index = controlNrs++;
	}
	if (config->remind_title.data) {
		remind_title_index = controlNrs++;
	}

	db_msg("total controlNrs:%d", controlNrs);
	CtrlData = (PCTRLDATA) malloc(sizeof(CTRLDATA) * controlNrs);
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&dlgTemplate, 0, sizeof(dlgTemplate));
	dlgTemplate.dwStyle = WS_NONE;
	dlgTemplate.dwExStyle = WS_EX_USEPARENTFONT;

	state->images = (BITMAP *) malloc(sizeof(BITMAP) * state->itemsize);
	memset(state->images, 0, sizeof(BITMAP) * state->itemsize);

	state->images_active = (BITMAP *) malloc(sizeof(BITMAP) * state->itemsize);
	memset(state->images_active, 0, sizeof(BITMAP) * state->itemsize);

	snprintf(configkey, sizeof(configkey), "pnoff%d", getStyleFromState(state));
	confstr = res_getString2(MK_kb, configkey);
	if (confstr) {
		sscanf(confstr, "%d %d", &offset_x, &offset_y);
	}

	for (int i = 0; i < state->itemsize; i++) {
		c = &CtrlData[i];
		c->class_name = CTRL_STATIC;
		c->dwStyle = WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE;
		c->dwExStyle = WS_EX_TRANSPARENT;
		c->id = KEYID_INDEX_0 + i;
		c->caption = "";
		snprintf(configkey, sizeof(configkey), "k%d%02d", getStyleFromState(state), i);

		confstr = res_getString2(MK_kb, configkey);
		if (confstr) {
			sscanf(confstr, "%d %d %d %d", &(c->x), &(c->y), &(c->w), &(c->h));
		}
		c->x += offset_x;
		c->y += offset_y;
	}

	if (state->style == KEYBOARD_STYLE_ASSN) {
		initAssKeyboard(CtrlData, state);
	}

	if (title_index) {
		res_getLabelSetParam(label_set, MK_kb, "title_pos");
		label_string *str = &config->title;
		str->font = 2;
		str->style = LABEL_STYLE_CENTER;
		db_msg("keyboard title:%s font:%d style:%d x:%d y:%d w:%d h:%d", config->title.data, label_set->font, label_set->style, label_set->x, label_set->y, label_set->w, label_set->h);
		initStringCtrlData(&CtrlData[title_index], IDC_TITLE, &config->title, label_set);
	}

	if (remind_title_index) {
		res_getLabelSetParam(label_set, MK_kb, "remind_title_pos");
		initStringCtrlData(&CtrlData[remind_title_index], IDC_REMIND_TEXT, &config->remind_title, label_set);
	}

	if (text_field_index) {
		res_getLabelSetParam(label_set, MK_kb, "text_field_config");
		if (config->flag & KEYBOARD_FLAG_MULTI_LINE) {
			label_set->h *= 2;
		}
		initStringCtrlData(&CtrlData[text_field_index], IDC_TEXT_FIELD, &config->text_field, label_set);
	}

	if (text_field_line_index) {
		initImageCtrlData(&CtrlData[text_field_line_index], IDC_TEXT_FIELD_LINE, res_getString2(MK_kb, "text_field_line_pos"));
	}

	if (hidecode_index) {
		initImageCtrlData(&CtrlData[hidecode_index], IDC_HIDE_CODE, res_getString2(MK_kb, "hc_pos"));
	}
	db_msg("init CtrlData finish");

	dlgTemplate.x = 0;
	dlgTemplate.y = 0;
	res_screen_info(&dlgTemplate.w, &dlgTemplate.h);
	dlgTemplate.caption = "";
	dlgTemplate.controlnr = controlNrs;
	dlgTemplate.controls = CtrlData;
	db_msg("call DialogBoxIndirectParam");
	int pkeyesc = use_powerkey_as_esc(1);
	retval = DialogBoxIndirectParam(&dlgTemplate, hParentWnd, KeyBoardProc, (LPARAM) state);
	use_powerkey_as_esc(pkeyesc);
	db_secure("keyboard result len:%d data:%s", retval, config->result);
	for (int i = 0; i < state->itemsize; i++) {
		if (state->images[i].bmBits != NULL) {
			//db_msg("unload img:%i", i);
			UnloadBitmap(&state->images[i]);
		}
		if (state->images_active[i].bmBits != NULL) {
			//db_msg("unload active img:%i", i);
			UnloadBitmap(&state->images_active[i]);
		}
	}
	if (hide_code_img.bmBits != NULL) {
		UnloadBitmap(&hide_code_img);
	}
	if (textfield_line_img.bmBits != NULL) {
		UnloadBitmap(&textfield_line_img);
	}
	free(state->images);
	free(state->images_active);
	free(CtrlData);
	if (state->ass_state) {
		freeAssKeyboardBitmap(state->ass_state);
	}

	return retval;
}

int get_random_keyboard_index(int style) {
	uint32_t i = random32();
	int n;
#ifdef BUILD_FOR_DEV
	i = FULL_KEYBOARD_DEFAULT_SELECT_IDNEX;
#endif
	switch (style) {
		case KEYBOARD_STYLE_FULL:
			n = 27;
			break;
		case KEYBOARD_STYLE_ASSN:
		case KEYBOARD_STYLE_CHAR:
			n = 26;
			break;
		case KEYBOARD_STYLE_PIN:
		default:
			n = 9;
			break;
	}
	i = i % n;
	if (i == 19 && style == KEYBOARD_STYLE_FULL) {
		i = FULL_KEYBOARD_DEFAULT_SELECT_IDNEX;
	}
	return i;
}
