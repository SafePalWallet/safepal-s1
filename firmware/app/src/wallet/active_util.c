#define LOG_TAG "active_util"

#include <arpa/inet.h>
#include "common.h"
#include "active_util.h"
#include "resource.h"
#include "device.h"
#include "secure_api.h"
#include "sha2.h"
#include "bignum.h"
#include "fileutil.h"
#include "rand.h"
#include "secure_util.h"
#include "base64.h"

BITMAP *gVNumberBitmap = NULL;

static void showVNumber(HWND hParent, int n, int x, BITMAP *bmp) {
	char path[64];
	snprintf(path, sizeof(path), "%s/img/num/vn%d.png", get_system_res_point(), n);
	res_loadBmp(path, bmp);
	HWND hwnd = createWidgetWindow(hParent, 0, x, 116, 30, 44, 0, WIDGET_TYPE_IMG, 0, -1);
	SendMessage(hwnd, STM_SETIMAGE, (WPARAM) (bmp), 0);
	InvalidateRect(hwnd, NULL, TRUE);
}

int active_get_vnumber() {
	//...
	return 123456;
}

int active_get_url(char *url_buffer, int len) {
	//...
	return -1;
}

void active_free_vnum(int freeall) {
	db_msg("freeall:%d", freeall);
	if (gVNumberBitmap != NULL) {
		for (int i = 0; i < 6; i++) {
			if (gVNumberBitmap[i].bmBits != NULL) {
				UnloadBitmap(&gVNumberBitmap[i]);
				gVNumberBitmap[i].bmBits = NULL;
			}
		}
		if (freeall) {
			free(gVNumberBitmap);
			gVNumberBitmap = NULL;
		}
	}
}

void active_init_vnum_cb(HWND hwnd) {
	db_msg("hwnd:%d", hwnd);
	if (gVNumberBitmap == NULL) {
		gVNumberBitmap = (BITMAP *) malloc(sizeof(BITMAP) * 6);
		if (gVNumberBitmap) {
			memset(gVNumberBitmap, 0, sizeof(BITMAP) * 6);
		} else {
			return;
		}
	} else {
		active_free_vnum(0);
	}
	int xs[6] = {195, 159, 123, 87, 51, 15};
	int n = active_get_vnumber();
	if (n < 0) {
		return;
	}
	for (int i = 0; i < 6; i++) {
		showVNumber(hwnd, n % 10, xs[i], &gVNumberBitmap[i]);
		n /= 10;
	}
}

int active_decode_info(user_active_info *info, const unsigned char *data, int len) {
	//...
	return 0;
}