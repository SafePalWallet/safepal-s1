#ifndef WALLET_WIDGETS_H
#define WALLET_WIDGETS_H

#include "win_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_EDITOR_INPUT 0x0
#define KEYBOARD_EDITOR_HIDECODE 0x1

typedef enum {
	KEYBOARD_STYLE_FULL = 0,
	KEYBOARD_STYLE_PIN = 1,
	KEYBOARD_STYLE_ASSN = 2, 
	KEYBOARD_STYLE_CHAR = 3  
} KEYBOARD_STYLE;


#define  KEYBOARD_FLAG_MULTI_LINE 0x1
#define  KEYBOARD_FLAG_HIDE_MORE  0x2

#define FULL_KEYBOARD_DEFAULT_SELECT_IDNEX 14
#define FULL_KEYBOARD_DEL_KEY_INDEX 27

#define CHAR_KEYBOARD_DEFAULT_SELECT_INDEX FULL_KEYBOARD_DEFAULT_SELECT_IDNEX
#define CHAR_KEYBOARD_BACK_KEY_INDEX 26
#define CHAR_KEYBOARD_OK_KEY_INDEX   27
#define CHAR_KEYBOARD_DEL_KEY_INDEX  28

#define SHOW_QR_FLAG_RAW_DATA 0x10000

typedef struct {
	int keyStyle; //0 full key 1 pin key 2 association key
	char *result;
	int result_limit;
	int editor_style; //0 input editor 1 hide code
	int random;
	label_string title;
	label_string remind_title;
	label_string text_field;
	int initKeyIndex;
	int flag;
} KeyBoardConfig_t;

extern int showKeyBoard(HWND hParentWnd, KeyBoardConfig_t *config);
extern int get_random_keyboard_index(int style);
extern int genQrcodeBitmap(PBITMAP pBitmap, const unsigned char *src, int src_size, int w, int h, int separator, int qr_mode);
extern int showQRWindow(HWND hParent, int client_id, unsigned int flag, int msgtype, const unsigned char *qrdata, int size);

int initStringCtrlData(CTRLDATA *c, int id, label_string *str, label_set_param *set);
int initImageCtrlData(CTRLDATA *c, int id, const char *pos);
int initImageCtrlDataWithRect(CTRLDATA *c, int id, const WIN_RECT rect);
int deepSetWindowFont(HWND hwnd, PLOGFONT font);
int updateCtrlDataRect(CTRLDATA *c, WIN_RECT rect);

int getTextRect(const char *text, int maxWidth, SIZE *size, PLOGFONT logFont);

HWND createWidgetWindow(HWND parent, DWORD dwAddData, int x, int y, int w, int h, int id, int wigetType, int style, int font);

int setDCLabelText(HWND hwnd, const char *txt, BOOL bEraseBkgnd);

#ifdef __cplusplus
}
#endif

#endif
