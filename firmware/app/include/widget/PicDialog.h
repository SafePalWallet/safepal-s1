#ifndef TRUNK_PICDIALOG_H
#define TRUNK_PICDIALOG_H

#include <minigui/common.h>
#include "widgets.h"

#define PIC_DLG_FLAG_LEFT_NOT_BACK 0x1
#define PIC_DLG_FLAG_RIGHT_AS_OK 0x2
#define PIC_DLG_FLAG_MULTI_PIC 0x4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *name;
	int total;
	int initIndex;
	const char **btnTitles;
	int buttonCount;
	int initBtnIndex;
	int flag;

	const char *dync_label;
	const char *dync_text;

	void (*onInitCallback)(HWND hParent);
} PicDialogConfig_t;

extern int showPicDialog(HWND hParent, PicDialogConfig_t *config);

extern int picDialogFlag(HWND hParent, const char *name, const char *btn0, const char *btn1, int btnInitIndex, int flag);

extern int picDialog(HWND hParent, const char *name, const char *btn0, const char *btn1, int btnInitIndex);

#ifdef __cplusplus
}
#endif

#endif
