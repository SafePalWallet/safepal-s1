
#ifndef WALLET_RES_H
#define WALLET_RES_H

#include <minigui/common.h>
#include <minigui/gdi.h>
#include <unistd.h>
#include "win_comm.h"
#include "walletLang.h"

#include "ConfigKey.h"
#include "settings.h"
#include "minigui_comm.h"
#include "map.h"
#include "widgets.h"

#define CSTATE_SP_CLICK 0x1
#define CSTATE_SP_ACTIVE 0x2

#define CFKEYX(m, s, x) ( 0x1000000 | ((x)&0xFF)<<16 | ((s)&0xFF)<<8 | ((m)&0xFF) )
#define CFKEYX2(ms, x)  ( 0x1000000 | ((x)&0xFF)<<16 | ((ms)&0xFFFF) )
#define CFKEY(m, s) ( ((s)&0xFF)<<8 | ((m)&0xFF) )

#define ICON_KEY(m) CFKEY(m,SK_icon)
#define ICON_STATE_KEY(m, x) CFKEYX(m,SK_icon,x)

#define SET_CSTATE_SP(s, p) (((s)&0x3F) | (((p)&0x3)<<6))
#define GET_CSTATE_SP(s) (((s)>>6)&0x3)
#define DROP_CSTATE_SP(s) ((s)&0x3F)

#define FONT_TYPE_SIZE 12

#define FONT_18 (3)
#define FONT_16 (4)
#define FONT_14 (5)
#define FONT_12 (11)

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

#ifdef __cplusplus
extern "C" {
#endif

int res_initStage1();

void res_screen_info(int *w, int *h);

const char *res_getLabel(int labelIndex);

const char *res_getLangName(int index);

PLOGFONT res_getFont(int type);

const char *res_getString(int cfgkey);

const char *res_getString2(int mk, const char *skey);

int res_getInt(int mk, const char *skey, int def);

gal_pixel res_getColor(int mk, const char *skey);

gal_pixel res_getTextColor(int def);

gal_pixel res_getTextHColor(int id);

gal_pixel res_getDisableColor();

gal_pixel res_getBGColor();

gal_pixel res_getBGColor1(int def);

int res_getRect(int mkey, const char *skey, WIN_RECT *rect);

int res_getPos(int mkey, WIN_RECT *rect);

int res_getBmpByKey(int cfgkey, BITMAP *bmp);

int res_getBmpByState(int mk, int sk, int state, BITMAP *bmp);

int res_getBmp(int mk, const char *skey, BITMAP *bmp);

int res_loadBmp(const char *path, BITMAP *bmp);

void res_unloadBmp(BITMAP *bmp);

int res_updateLangAndFont(int newLang);

int res_parseLabelSetParam(label_set_param *p, const char *setting);

int res_getLabelSetParam(label_set_param *p, int mk, const char *skey);

#ifdef __cplusplus
}
#endif

#endif
