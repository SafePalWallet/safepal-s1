#ifndef MY_MINIGUI_COMM_H
#define MY_MINIGUI_COMM_H

#define USE_MINIGUI_3_0

#include <minigui/mgconfig.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#ifdef USE_MINIGUI_3_0
#define PROC_RET int
#define PROC_MSG_TYPE  int
#define LINT int

#define WPARAM_FMT "%u"
#define HWND_FMT "%u"
#else
#define PROC_RET LRESULT
#define PROC_MSG_TYPE UINT
#define WPARAM_FMT "%ld"
#define HWND_FMT "%p"
#endif

#define SPEC_TEXT_PREFIX_LEN 3

#define IS_VALID_HWND(x) ((x) && (x) != HWND_INVALID)

#define LABEL_STYLE_HIGHLIGHT 0x1
#define LABEL_STYLE_LEFT 0x2
#define LABEL_STYLE_CENTER 0x4
#define LABEL_STYLE_RIGHT 0x8
#define LABEL_STYLE_VERBOSE 0x10
#define LABEL_STYLE_HL_0 LABEL_STYLE_HIGHLIGHT
#define LABEL_STYLE_HL_1 LABEL_STYLE_HIGHLIGHT
#define LABEL_STYLE_HL_2 0x20
#define LABEL_STYLE_DISABLE 0x40

#define STATUS_ALL  0xFFFFFFFF
#define HAVE_ANY_STATUS(s, need) (((s) & (need)) != 0)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int x, y;
	int w, h;
} WIN_RECT;

typedef enum {
	SPEC_TEXT_ALIGN_UNKOWN = -1,
	SPEC_TEXT_ALIGN_DEFAULT, 
	SPEC_TEXT_ALIGN_LEFT, // <1>xxxxx
	SPEC_TEXT_ALIGN_CENTER, // <2>xxxxx
	SPEC_TEXT_ALIGN_RIGHT, // <3>xxxxxxx
} SPEC_TEXT_ALIGN;

enum {
	WIDGET_TYPE_NORMAL = 0, // window
	WIDGET_TYPE_TEXT, // text Window
	WIDGET_TYPE_IMG // img window
};

typedef struct {
	int config_code;
	const char *config_key;
	int hwnd_index;
	unsigned int state;
} layout_icon_param;

typedef struct {
	int config_code;
	const char *config_key;
	int hwnd_index;
	unsigned int state;
	unsigned int style;
	int font;
} layout_label_param;

typedef struct {
	int x, y, w, h;
	unsigned int style;
	int font;
	int hwnd_index;
	unsigned int state;
} label_set_param;

typedef struct {
	unsigned int style;
	int font;
	const char *data;
} label_string;

extern const char *Message2Str(int message);

HWND createTxtWidget(HWND parent, int mkey, int id, int offset);

HWND createImageWidget(HWND parent, int id, int x, int y, int w, int h, DWORD dwAddData);

int set_window_text2(HWND hwnds0, int width0, HWND hwnds1, int width1, const char *text);

void set_label_to_string(label_set_param *p, label_string *str);

int setDialogLabelStyle(HWND hDlg, int id, label_string *str);

int setLabelWindowAttr(HWND hwnd, int style, int font);

int setLabelTextColor(HWND hwnd, gal_pixel color);

int setLabelFont(HWND hwnd, int font);

int getSpecAlignFromStr(const char *str);

const char *filterSpecTextFromStr(const char *str);

int SetWindowMText(HWND hWnd, const char *spString);

int strGetFont(const char *spString, int *pos);

int createDynaLayoutWidgets(HWND hParent, layout_label_param *p, int pos_offset, int block_height, int dyna_navi_height, int dyna_navi_start, int create_num, HWND *hwnds);

#ifdef __cplusplus
}
#endif
#endif