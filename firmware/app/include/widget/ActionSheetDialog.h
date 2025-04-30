#ifndef TRUNK_ACTIONSHEETDIALOG_H
#define TRUNK_ACTIONSHEETDIALOG_H

#include <stdlib.h>
#include <minigui/common.h>
#include "widgets.h"

#define ACTION_SHEET_DIALOG_FLAG_NAVI 1

typedef struct {
	label_string title;
	label_string msg;
	const char **items;
	int itemsCnt;
	int initIndex;
	//unsigned int flag;
} ActionSheetDialogConfig_t;

extern int showActionSheetDialog(HWND hParent, ActionSheetDialogConfig_t *config);

#endif
