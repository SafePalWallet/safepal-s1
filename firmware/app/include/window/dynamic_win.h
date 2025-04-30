#ifndef WALLET_DYNAMIC_WIN_H
#define WALLET_DYNAMIC_WIN_H

#include "minigui_comm.h"
#include "storage_manager.h"
#include "cstr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	//---- in -----
	HWND hwnd;
	int msg_from;
	int show_more;
	//---- out ----
	cstring labels[1];
	int has_more;
	int total_height;
	int coin_type;
	uint32_t flag;
	const char *coin_uname;
	const char *coin_name;
	const char *coin_symbol;
	DBTxCoinInfo db;
} DynamicViewCtx;

int dwin_init(DynamicViewCtx *view, HWND hwnd, size_t init_size);

int dwin_destory(DynamicViewCtx *view);

HWND dwin_add_txt(DynamicViewCtx *view, int mkey, int id, const char *value);

HWND dwin_add_txt_offset(DynamicViewCtx *view, int mkey, int id, const char *value, int offset);

#ifdef __cplusplus
}
#endif
#endif
