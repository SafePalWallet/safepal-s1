#define LOG_TAG "gui_comm"

#include "minigui_comm.h"
#include "resource.h"
#include "debug.h"
#include "msgstr.h"

const char *Message2Str(int message) {
	if (message >= 0x0000 && message <= 0x006F)
		return __mg_msgstr1[message];
	else if (message >= 0x00A0 && message <= 0x010F)
		return __mg_msgstr2[message - 0x00A0];
	else if (message >= 0x0120 && message <= 0x017F)
		return __mg_msgstr3[message - 0x0120];
	else if (message >= 0xF000)
		return "Control Messages";
	else
		return "MSG_USER";
}

HWND createTxtWidget(HWND parent, int mkey, int id, int offset) {
	if (!mkey) {
		db_error("invalid mkey:%d ", mkey);
		return -1;
	}
	if (!IS_VALID_HWND(parent)) {
		db_error("invalid parent ");
		return -1;
	}

	int retval;
	WIN_RECT rect;
	HWND retWnd;
	retval = res_getPos(mkey, &rect);
	if (retval < 0 || rect.w <= 1) {
		db_error("get rect failed code:0x%x ret:%d,w:%d", mkey, retval, rect.w);
		return -1;
	}
	int config_style = 0;
	int config_font = 0;
	const char *configstr = res_getString2(mkey, "style");
	if (configstr != NULL && strlen(configstr) > 0) {
		sscanf(configstr, "%d %d", &config_style, &config_font);
	}

	DWORD style = SS_SIMPLE;
	if (config_style) {
		if (config_style & LABEL_STYLE_LEFT) {
			style |= SS_LEFT;
		} else if (config_style & LABEL_STYLE_CENTER) {
			style |= SS_CENTER;
		} else if (config_style & LABEL_STYLE_RIGHT) {
			style |= SS_RIGHT;
		}
	}
	offset += rect.y;
	retWnd = CreateWindowEx(CTRL_STATIC, "",
	                        WS_CHILD | WS_VISIBLE | style,
	                        WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
	                        id,
	                        rect.x, offset, rect.w, rect.h,
	                        parent, 0);
	if (retWnd == HWND_INVALID) {
		db_error("create label false code:%x failed", mkey);
		return -1;
	}
	setLabelWindowAttr(retWnd, config_style, config_font);
	return retWnd;
}

HWND createImageWidget(HWND parent, int id, int x, int y, int w, int h, DWORD dwAddData) {
	HWND retWnd = CreateWindowEx(CTRL_STATIC, "",
	                             WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | WS_VISIBLE,
	                             WS_EX_TRANSPARENT,
	                             id,
	                             x, y, w, h,
	                             parent, dwAddData);
	if (!IS_VALID_HWND(retWnd)) {
		db_error("create image false id:%x failed", id);
		return HWND_INVALID;
	}
	return retWnd;
}

int set_window_text2(HWND hwnds0, int width0, HWND hwnds1, int width1, const char *text) {
	char buf[64];
	int len;
	char c;
	int textlen = strlen(text);
	if (width0 < 0) {
		len = 0;
	} else if (width1 < 0) {
		len = textlen;
	} else if (width0 == width1) {
		len = textlen / 2;
	} else if (width1 == 0) {
		len = textlen;
	} else {
		len = textlen * width0 / (width0 + width1);
	}
	if (len > textlen) {
		len = textlen;
	}
	if (len > 0) {
		if (len != textlen) {
			if (len > 63) {
				len = 63;
			}
			memcpy(buf, text, len);
			buf[len] = '\0';
			SetWindowMText(hwnds0, buf);
		} else {
			SetWindowMText(hwnds0, text);
		}
	} else {
		SetWindowMText(hwnds0, "");
	}
	if (textlen > len) {
		SetWindowMText(hwnds1, text + len);
	} else {
		SetWindowMText(hwnds1, "");
	}
	return len;
}

void set_label_to_string(label_set_param *p, label_string *str) {
	str->style = p->style;
	str->font = p->font;
}

int setDialogLabelStyle(HWND hDlg, int id, label_string *str) {
	HWND hwnd;
	hwnd = GetDlgItem(hDlg, id);
	if (IS_VALID_HWND(hwnd)) {
		return setLabelWindowAttr(hwnd, str->style, str->font);
	}
	return 0;
}

int setLabelWindowAttr(HWND hwnd, int style, int font) {
	if (!IS_VALID_HWND(hwnd)) {
		db_error("invalid hwnd:%d", hwnd);
		return -1;
	}
	db_verbose("hwnd:%d style:%d font:%d", hwnd, style, font);
	if (style & LABEL_STYLE_HIGHLIGHT) {
		SetWindowElementAttr(hwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, res_getTextHColor(0)));
	} else if (style & LABEL_STYLE_HL_2) {
		SetWindowElementAttr(hwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, res_getTextHColor(2)));
	} else if (style & LABEL_STYLE_DISABLE) {
		SetWindowElementAttr(hwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, res_getDisableColor()));
	} else {
		SetWindowElementAttr(hwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, res_getTextColor(0)));
	}

	if (font < 0) {
		db_error("invalid font:%d", font);
	} else {
		SetWindowFont(hwnd, res_getFont(font));
	}

	return 0;
}

int setLabelTextColor(HWND hwnd, gal_pixel color) {
	if (!IS_VALID_HWND(hwnd)) {
		return -1;
	}
	SetWindowElementAttr(hwnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, color));
	return 0;
}

int setLabelFont(HWND hwnd, int font) {
	if (!IS_VALID_HWND(hwnd)) {
		return -1;
	}
	SetWindowFont(hwnd, res_getFont(font));
	return 0;
}

int getSpecAlignFromStr(const char *str) {
	if (strlen(str) < SPEC_TEXT_PREFIX_LEN) {
		return SPEC_TEXT_ALIGN_DEFAULT;
	}
	if (str[0] != '<') {
		return SPEC_TEXT_ALIGN_DEFAULT;
	} else if (str[2] == '>') {
		switch (str[1]) {
			case '1':
				return SPEC_TEXT_ALIGN_LEFT;
			case '2':
				return SPEC_TEXT_ALIGN_CENTER;
			case '3':
				return SPEC_TEXT_ALIGN_RIGHT;
			default:
				db_error("unkown spec text align %s", str);
				return SPEC_TEXT_ALIGN_DEFAULT;
		}
	}

	return SPEC_TEXT_ALIGN_DEFAULT;
}

const char *filterSpecTextFromStr(const char *str) {
	if (NULL == str) {
		return NULL;
	}
	if (strlen(str) < SPEC_TEXT_PREFIX_LEN) {
		return str;
	}
	SPEC_TEXT_ALIGN align = (SPEC_TEXT_ALIGN) getSpecAlignFromStr(str);
	if (align == SPEC_TEXT_ALIGN_DEFAULT || align == SPEC_TEXT_ALIGN_UNKOWN) {
		return str;
	}
	return (const char *) (str + SPEC_TEXT_PREFIX_LEN);
}

int SetWindowMText(HWND hWnd, const char *spString) {
	if (spString && spString[0] == '{' && (spString[1] >= '0' && spString[1] <= '9')) {
		int f = spString[1] - '0';
		if (spString[2] == '}') {
			spString += 3;
		} else if ((spString[2] >= '0' && spString[2] <= '9') && spString[3] == '}') {
			f = f * 10 + (spString[2] - '0');
			spString += 4;
		}
		SetWindowFont(hWnd, res_getFont(f));
	}
	return SetWindowText(hWnd, spString);
}

int strGetFont(const char *spString, int *pos){
    *pos = 0;
	if (spString && spString[0] == '{' && (spString[1] >= '0' && spString[1] <= '9')) {
		int f = spString[1] - '0';
		if (spString[2] == '}') {
			*pos = 3;
			
		} else if ((spString[2] >= '0' && spString[2] <= '9') && spString[3] == '}') {
			f = f * 10 + (spString[2] - '0');
			*pos = 4;
		}
		
		return f;
	}
	

	return -1;
}

int createDynaLayoutWidgets(HWND hParent, layout_label_param *p, int pos_offset, int block_height, int dyna_navi_height, int dyna_navi_start, int create_num, HWND *hwnds) {
	if (!p->config_code) {
		db_error("config code:%d invalid", p->config_code);
		return -1;
	}
	int retval;
	WIN_RECT rect;
	HWND retWnd;

	retval = res_getPos(p->config_code, &rect);
	if (retval < 0 || rect.w <= 1) {
		db_error("get rect failed code:0x%x ret:%d,w:%d", p->config_code, retval, rect.w);
		return -1;
	}
	const char *configstr = res_getString2(p->config_code, "style");
	if (configstr != NULL && strlen(configstr) > 0) {
		sscanf(configstr, "%d %d", &(p->style), &(p->font));
	}

	if (p->hwnd_index < 0 || p->hwnd_index > 3) {
		db_error("skip create label,code:0x%x hwnd_index:%d state:%d", p->config_code, p->hwnd_index, p->state);
		return 0;
	}

	DWORD style = SS_SIMPLE;
	if (p->style) {
		if (p->style & LABEL_STYLE_LEFT) {
			style |= SS_LEFT;
		} else if (p->style & LABEL_STYLE_CENTER) {
			style |= SS_CENTER;
		} else if (p->style & LABEL_STYLE_RIGHT) {
			style |= SS_RIGHT;
		}
	}
	int num_perpage = SCREEN_HEIGHT / block_height;

	int y;
	for (int i = 0; i < create_num; i++) {
		y = pos_offset + rect.y + block_height * i;
		//db_msg("1 i:%d y:%d num_perpage:%d dyna_navi_height:%d dyna_navi_start:%d", i, y, num_perpage, dyna_navi_height, dyna_navi_start);
		if (dyna_navi_height) {
			if ((i - dyna_navi_start) > 0) {
				y += (i - dyna_navi_start) / num_perpage * dyna_navi_height;
			}
		}
		retWnd = CreateWindowEx(CTRL_STATIC, "",
		                        WS_CHILD | WS_VISIBLE | style,
		                        WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
		                        i,
		                        rect.x, y, rect.w, rect.h,
		                        hParent, 0);
		if (retWnd == HWND_INVALID) {
			db_error("create label false code:%x failed", p->config_code);
			return -1;
		}
		setLabelWindowAttr(retWnd, p->style, p->font);
		hwnds[i] = retWnd;
	}
	return 0;
}
