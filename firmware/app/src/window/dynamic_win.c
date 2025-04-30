#define LOG_TAG "dwin"

#include "common_c.h"
#include "dynamic_win.h"
#include "minigui_comm.h"

#define HWND_SIZE sizeof(HWND)

#define CTX_HWND_LEN(v) ((v)->labels->len / HWND_SIZE)

int dwin_init(DynamicViewCtx *view, HWND hwnd, size_t init_size) {
	db_msg("init view:%p hwnd:%d", view, hwnd);
	if (!IS_VALID_HWND(hwnd)) {
		db_error("invalid hwnd:%d", hwnd);
	}
	memset(view, 0, sizeof(DynamicViewCtx));
	view->hwnd = hwnd;
	view->total_height = 0;
	cstr_init(view->labels, init_size * HWND_SIZE);
	return 0;
}

int dwin_destory(DynamicViewCtx *view) {
	//destory txt label
	size_t n = CTX_HWND_LEN(view);
	db_msg("destory view:%p n:%d", view, n);
	HWND *p = (HWND *) view->labels->str;
	for (size_t i = 0; i < n; i++) {
//		db_msg("del hwnd:%d", *p);
		DestroyWindow(*p);
		p++;
	}
	cstr_free(view->labels);
	view->hwnd = HWND_INVALID;
	view->total_height = 0;
	memset(view, 0, sizeof(DynamicViewCtx));
	return 0;
}

HWND dwin_add_txt(DynamicViewCtx *view, int mkey, int id, const char *value) {
	return dwin_add_txt_offset(view, mkey, id, value, 0);
}

HWND dwin_add_txt_offset(DynamicViewCtx *view, int mkey, int id, const char *value, int offset) {
	db_msg("mkey:%d id:%d val:%s", mkey, id, value);
	if (is_empty_string(value)) {
		return HWND_INVALID;
	}
	HWND hwnd = createTxtWidget(view->hwnd, mkey, id, offset);
	if (!IS_VALID_HWND(hwnd)) {
		return HWND_INVALID;
	}
	cstr_append_buf(view->labels, &hwnd, HWND_SIZE);
	//db_msg("add hwnd:%d", hwnd);
	SetWindowMText(hwnd, value);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	return hwnd;
}
