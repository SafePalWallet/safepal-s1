
#define LOG_TAG "widget_common"

#include <minigui_comm.h>
#include "minigui_comm.h"
#include "widgets.h"
#include "debug.h"
#include "resource.h"
#include <minigui/gdi.h>

int initStringCtrlData(CTRLDATA *c, int id, label_string *str, label_set_param *set) {
	int style = set->style;
	DWORD dwStyle = SS_CENTER;

	c->x = set->x;
	c->y = set->y;
	c->w = set->w;
	c->h = set->h;

	if (style) {
		dwStyle = 0;
		if (style & LABEL_STYLE_LEFT) {
			dwStyle |= SS_ALIGNMASK;
//			dwStyle |= SS_LEFT;
		} else if (style & LABEL_STYLE_CENTER) {
			dwStyle |= SS_CENTER;
		} else if (style & LABEL_STYLE_RIGHT) {
			dwStyle |= SS_RIGHT;
		} else {
			dwStyle = SS_CENTER;
		}
	}
	c->class_name = CTRL_STATIC;
	c->dwStyle = WS_CHILD | WS_VISIBLE | dwStyle;
	c->dwExStyle = WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT;
	c->id = id;

	if (str) {
		int font = -1, pos = 0;
		font = strGetFont(str->data, &pos);
		if (font >= 0) {
			c->caption = str->data + pos;
			set_label_to_string(set, str);
			str->font = font;
		} else {
			c->caption = str->data;
			set_label_to_string(set, str);
		}
	}
	return 0;
}

int updateCtrlDataRect(CTRLDATA *c, WIN_RECT rect) {
	if (NULL == c) {
		return -1;
	}
	c->x = rect.x;
	c->y = rect.y;
	c->w = rect.w;
	c->h = rect.h;

	return 0;
}

int initImageCtrlData(CTRLDATA *c, int id, const char *pos) {
	c->class_name = CTRL_STATIC;
	c->dwStyle = WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE;
	c->dwExStyle = WS_EX_TRANSPARENT;
	c->id = id;
	c->caption = "";
	if (pos && pos[0] != 0) {
		sscanf(pos, "%d %d %d %d", &(c->x), &(c->y), &(c->w), &(c->h));
	}
	return 0;
}

int initImageCtrlDataWithRect(CTRLDATA *c, int id, const WIN_RECT rect) {
	c->class_name = CTRL_STATIC;
	c->dwStyle = WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE;
	c->dwExStyle = WS_EX_TRANSPARENT;
	c->id = id;
	c->caption = "";
	c->x = rect.x;
	c->y = rect.y;
	c->w = rect.w;
	c->h = rect.h;
	return 0;
}

int deepSetWindowFont(HWND hwnd, PLOGFONT font) {
	HWND hChild;
	PLOGFONT f = GetWindowFont(hwnd);
	if (font != f) {
		db_msg("changed font %s %s %s -> %s %s %s", f->type, f->family, f->charset, font->type, font->family, font->charset);
		hChild = GetNextChild(hwnd, 0);
		while (hChild && (hChild != HWND_INVALID)) {
			SetWindowFont(hChild, font);
			hChild = GetNextChild(hwnd, hChild);
		}
	}
	return 0;
}

int getTextRect(const char *text, int maxWidth, SIZE *size, PLOGFONT logFont) {
	if (NULL == size) {
		return -1;
	}

	HDC hdc = GetDC(HWND_DESKTOP);
	if (hdc == HDC_INVALID) {
		db_error("invalid hdc");
		return -1;
	}
	PLOGFONT of = NULL;
	if (logFont) {
		of = SelectFont(hdc, logFont);
	}
	size->cx = 0;
	size->cy = 0;
	if (!(maxWidth >= GetMaxFontWidth(hdc))) {
		ReleaseDC(hdc);
		db_error("invalid maxWidth:%d", maxWidth);
		return -1;
	}
	SIZE ls;
	const char *pstr = text ? text : "";
	int offset = 0;
	int len = strlen(pstr);
	int progress = 0;
//	while (strlen(pstr += offset)) {
	while (strlen(pstr += offset)) {
		offset = GetTabbedTextExtentPoint(hdc, pstr, strlen(pstr), maxWidth, NULL, NULL, NULL, &ls);
		size->cx = MAX(size->cx, ls.cx);
		size->cy += ls.cy;
		offset = MIN(offset, (int) strlen(pstr));
	}
	ReleaseDC(hdc);
	return 0;
}

HWND createWidgetWindow(HWND parent, DWORD dwAddData, int x, int y, int w, int h, int id, int wigetType, int style, int font) {
	HWND retWnd = HWND_INVALID;
	DWORD retStyle = SS_SIMPLE;
	DWORD dwExStyle = SS_SIMPLE;

	if (wigetType == WIDGET_TYPE_NORMAL) {
		retStyle = WS_CHILD | WS_VISIBLE;
		dwExStyle = WS_EX_USEPARENTFONT;
	} else if (wigetType == WIDGET_TYPE_IMG) {
		retStyle = WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE;
		dwExStyle = WS_EX_TRANSPARENT;
	} else if (wigetType == WIDGET_TYPE_TEXT) {
		retStyle = WS_CHILD | WS_VISIBLE;
		dwExStyle = WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT;
		if (style) {
			if (style & LABEL_STYLE_LEFT) {
				retStyle |= SS_LEFT;
			} else if (style & LABEL_STYLE_CENTER) {
				retStyle |= SS_CENTER;
			} else if (style & LABEL_STYLE_RIGHT) {
				retStyle |= SS_RIGHT;
			}
		}
	}
	retWnd = CreateWindowEx(CTRL_STATIC, "",
	                        retStyle,
	                        dwExStyle,
	                        id,
	                        x, y, w, h,
	                        parent, dwAddData);
	if (!IS_VALID_HWND(retWnd)) {
		db_error("create window false id:%x failed", id);
		return HWND_INVALID;
	}
	if (wigetType == WIDGET_TYPE_TEXT) {
		setLabelWindowAttr(retWnd, style, font);
	}
	return retWnd;
}

int setDCLabelText(HWND hwnd, const char *txt, BOOL bEraseBkgnd) {
	RECT rc;
	GetClientRect(hwnd, &rc);
	db_msg("txt:%s", txt);
	SetWindowCaption(hwnd, txt);
	InvalidateRect(hwnd, &rc, bEraseBkgnd);
	return 0;
}
