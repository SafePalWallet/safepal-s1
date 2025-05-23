#ifndef WALLET_ACTIVEUTIL_H
#define WALLET_ACTIVEUTIL_H

#include "minigui_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char tag[2];
	unsigned char version;
	unsigned char sign_index;
	char sn[24];
	int time;
	int time_zone;
	unsigned char sign_data[64];
} user_active_info;

int active_get_vnumber();

int active_get_url(char *url_buffer, int len);

void active_free_vnum(int freeall);

void active_init_vnum_cb(HWND hwnd);

int active_decode_info(user_active_info *info, const unsigned char *data, int len);

#ifdef __cplusplus
}
#endif
#endif
