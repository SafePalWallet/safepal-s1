
#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <minigui/common.h>
#include "widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DIALOG_ICON_STYLE_NONE = 0,
	DIALOG_ICON_STYLE_SUCCESS = 1,
	DIALOG_ICON_STYLE_ERR = 2,
	DIALOG_ICON_STYLE_ALTER = 3
} dialogIconStyle_t;

typedef enum {
	DIALOG_BUTTON_ALIGN_NONE = 0,
	DIALOG_BUTTON_ALIGN_VERT = 1, 
	DIALOG_BUTTON_ALIGN_HORI = 2, 
	DIALOG_BUTTON_ALIGN_CENTER = 3, 
} dialogButtonAlign_t;

#define MAX_DIALOG_BUTTON_NUMBER 3

typedef struct {
	int win_style;
	int title_style;
	label_string title;
	int icon_style;
	int msg_style;
	label_string msg;
	int button_style;
	int button_count;
	int init_btn_index;
	label_string buttons[MAX_DIALOG_BUTTON_NUMBER];
} DialogConfig_t;

extern int showDialog(HWND hParent, DialogConfig_t *config);

extern int dialog(HWND hParent, const char *title, dialogIconStyle_t icon_style, const char *msg, dialogButtonAlign_t align,
                  const char *btn1, const char *btn2, int init_state);

extern int dialog_l(HWND hParent, const char *title, dialogIconStyle_t icon_style, const char *msg, dialogButtonAlign_t align,
                    const char *btn1, const char *btn2, int init_state);

extern int dialog_alert(HWND hParent, dialogIconStyle_t icon_style, const char *msg, const char *button_msg);

extern int dialog_error(HWND hParent, const char *msg);

extern int dialog_error2(HWND hParent, const char *msg, const char *btn_msg);

extern int dialog_error3(HWND hParent, int code, const char *msg);

extern int dialog_system_error(HWND hParent, int code);

extern int dialog_system_error2(HWND hParent, int code, const char *tips_msg, const char *btn_msg);

extern int dialog_confirm(HWND hParent,
                          const char *title,
                          const char *msg,
                          dialogButtonAlign_t align,
                          const char *cancel_msg,
                          const char *ok_msg,
                          int init_state);

#ifdef __cplusplus
}
#endif
#endif
