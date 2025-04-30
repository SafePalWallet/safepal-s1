#define LOG_TAG "Resource"

#include <string.h>
#include <errno.h>
#include "minigui_comm.h"
#include "resource.h"
#include "ini_parser.h"
#include "win_comm.h"
#include <sys/stat.h>
#include "common.h"
#include "win.h"
#include "ConfigKeyFileMap.h"

static const char *LANG_FILE_MAP[LANG_MAXID] = {
		"en",
		"zh-CN",
		"zh-TW",
		"jpn",
		"korean",
		"german",
		"french",
		"italian",
		"spanish",
		"vietnam",
		"russia",
		"portugal",
		"indonesia",
		"turkey",
		"thailand",
};

static const char *LANG_NAME_MAP[LANG_MAXID] = {
		"English",
		"简体中文",
		"繁體中文",
		"日本語",
		"한국어",
		"Deutsch",
		"Français",
		"Italiano",
		"Español",
		"Tiếng Việt",
		"Pусский",
		"Português",
		"Indonesia",
		"Türkçe",
		"ภาษาไทย",
};


static int mLang = CONFIG_DEFAULT_LANG;
static PLOGFONT mLogFonts[FONT_TYPE_SIZE];

static map_str_t mConfigMap;

static char **mLangLabel = NULL;
static char *mLangLabelBuffer = NULL;
static int mLangLabelSize = 0;

static int res_loadConfigValues(const char *configfile, map_str_t *map) {
	char ext_config[64];
	db_msg("load:%s", configfile);
	if (ini_parser_load(configfile, map) < 0) {
		db_error("cannot parse file: %s", configfile);
		return -1;
	}
	snprintf(ext_config, 48, "%s", configfile);
	int len = strlen(ext_config) - 4;

	const char *suffix = settings_get_ui_suffix();
	if (is_not_empty_string(suffix)) {
		snprintf(ext_config + len, sizeof(ext_config) - len, "-%s.cfg", suffix);
		db_msg("ext file:%s", ext_config);
		if (access(ext_config, F_OK) == 0) {
			res_loadConfigValues(ext_config, map);
		}
	}
	suffix = settings_get_lang_suffix();
	if (suffix || suffix[0] != 0) {
		snprintf(ext_config + len, sizeof(ext_config) - len, "-%s.cfg", suffix);
		db_msg("ext file:%s", ext_config);
		if (access(ext_config, F_OK) == 0) {
			res_loadConfigValues(ext_config, map);
		}
	}
	return 0;
}

static int res_initSystemValue() {
	map_str_t *map = &mConfigMap;
	map_set_str(map, "var:data", DATA_POINT);
	map_set_str(map, "var:res", get_system_res_point());
	map_set_str(map, "var:storage", STORAGE_POINT);
	return 0;
}

static int res_loadLabelFromLangFile(const char *langFile) {
	FILE *fp;
	int fsize;
	if (mLangLabelBuffer != NULL) {
		free(mLangLabelBuffer);
		mLangLabelBuffer = NULL;
	}
	if (mLangLabel != NULL) {
		free(mLangLabel);
		mLangLabel = NULL;
		mLangLabelSize = 0;
	}
	fp = fopen(langFile, "r");
	if (fp == NULL) {
		db_error("open file %s failed: %s", langFile, strerror(errno));
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fsize < 10 || fsize > 512 * 1024) {
		db_error("error file %s filesize:%d", langFile, fsize);
		fclose(fp);
		return -1;
	}
	mLangLabelBuffer = (char *) malloc(fsize + 1);
	if (mLangLabelBuffer == NULL) {
		db_error("new memory:%d false", fsize);
		fclose(fp);
		return -1;
	}
	if (fread(mLangLabelBuffer, fsize, 1, fp) <= 0) {
		db_error("read file %s false", langFile);
		fclose(fp);
		return -1;
	}
	mLangLabelBuffer[fsize] = 0;
	fclose(fp);

	int linenum = 0;
	char *s;
	char *p = mLangLabelBuffer;
	char *pend = mLangLabelBuffer + fsize;
	//count line number
	while (p < pend) {
		if (*p == '\n') {
			linenum++;
		}
		p++;
	}
	db_msg("line num:%d", linenum);

	mLangLabel = (char **) calloc(linenum, sizeof(char *));
	if (mLangLabel == NULL) {
		db_error("new memory false");
		return -1;
	}
	mLangLabelSize = linenum;

	linenum = 0;
	s = mLangLabelBuffer;
	p = mLangLabelBuffer;
	while (p < pend) {
		if (*p == '\r') {
			*p = 0;
			p++;
		}
		if (*p == '\n') {
			*p = 0;
			mLangLabel[linenum] = s; //save line
			linenum++;
			s = p + 1;
		}
		if (*p == '\\' && *(p + 1) == 'n') {
			*p++ = ' ';
			*p = '\n';
		}
		p++;
	}

	return 0;
}

static int res_initLangLabel() {
	if (!IS_VALID_LANG_ID(mLang)) {
		db_error("lang is no initialized");
		return -1;
	}
	char dataFile[48];
#ifdef DEBUG_TEMP_QUICKLY
	snprintf(dataFile, sizeof(dataFile), "%s/lang/%s.bin", STORAGE_POINT, LANG_FILE_MAP[mLang]);
	if (access(dataFile, F_OK) != 0) {
		snprintf(dataFile, sizeof(dataFile), "%s/lang/%s.bin", get_system_res_point(), LANG_FILE_MAP[mLang]);
	}
#else
	snprintf(dataFile, sizeof(dataFile), "%s/lang/%s.bin", get_system_res_point(), LANG_FILE_MAP[mLang]);
#endif
	if (res_loadLabelFromLangFile(dataFile) < 0) {
		db_error("load label from %s failed", dataFile);
		return -1;
	}
	db_msg("use lang:%s", dataFile);
	return 0;
}

static int res_initConfig() {
	char configPath[32];
	int usetest = 0;
#ifdef DEBUG_TEMP_QUICKLY
	snprintf(configPath, sizeof(configPath), "%s/wallet.cfg", STORAGE_POINT);
	usetest = access(configPath, F_OK) == 0;
	if (usetest) {
		struct stat buf;
		if (stat(configPath, &buf) != 0) {
			usetest = 0;
		} else {
			usetest = buf.st_size > 0;
		}
	}
#endif
	if (!usetest) {
		snprintf(configPath, sizeof(configPath), "%s/cfg/wallet.cfg", get_system_res_point());
	}
	db_msg("use config:%s", configPath);
	if (access(configPath, F_OK)) {
		db_error("config file:%s not exist", configPath);
		return -1;
	}

	map_str_t tmpConfig;
	map_init(&tmpConfig);
	res_loadConfigValues(configPath, &tmpConfig);

	//init system var
	map_init(&mConfigMap);
	res_initSystemValue();
	ini_parser_trans(&tmpConfig, &mConfigMap);
	map_deinit(&tmpConfig);

	return 0;
}

static int res_initLangAndFont() {
	int i;
	mLang = settings_get_lang();
	db_msg("lang:%d", mLang);
	char key[64];
	char type[20];
	int run_in_dev = app_run_in_dev_mode();
	for (i = 0; i < FONT_TYPE_SIZE; i++) {
		snprintf(key, sizeof(key), "font%d", i);
		const char *font_set = res_getString2(MK_system, key);
		if (is_empty_string(font_set)) {
			db_msg("not fontset:%d", i);
			continue;
		}
		if (strlen(font_set) > 63) {
			db_msg("invalid set font:%i data:%s", i, font_set);
			continue;
		}
		char weight = FONT_WEIGHT_BOOK;
		int font_size = 20;
		if (run_in_dev) {
			strncpy(key, "dev", 3);
			sscanf(font_set, "%19s %s %c %d", type, key + 3, &weight, &font_size);
		} else {
			sscanf(font_set, "%19s %s %c %d", type, key, &weight, &font_size);
		}
		db_msg("font:%d font set:%s font type:%s family:%s weight:%c font_size:%d", i, font_set, type, key, weight, font_size);

		mLogFonts[i] = CreateLogFont(type, key, "UTF-8",
		                             weight, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
		                             FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, font_size, 0);


		if (i == 0) {
			Global_Font20 = mLogFonts[i];
		} else if (i == 3) {
			Global_Font18 = mLogFonts[i];
		}
	}

	res_initLangLabel(); /* init lang labels */
	return 0;
}

int res_initStage1() {
	db_msg("Resource init1");
	int i;
	for (i = 0; i < FONT_TYPE_SIZE; i++) {
		mLogFonts[i] = NULL;
	}
	map_init(&mConfigMap);
	res_initConfig();
	res_initLangAndFont();
	return 0;
}

int res_initStage2() {
	db_msg("Resource init2");
	return 0;
}

void res_screen_info(int *w, int *h) {
	*w = SCREEN_WIDTH;
	*h = SCREEN_HEIGHT;
}

const char *res_getLabel(int labelIndex) {
	if (labelIndex >= 0 && labelIndex < mLangLabelSize) {
		return mLangLabel[labelIndex];
	} else {
		db_msg("invalide label Index: %d, size is %d", labelIndex, mLangLabelSize);
		return "";
	}
}

const char *res_getLangName(int index) {
	return IS_VALID_LANG_ID(index) ? LANG_NAME_MAP[index] : LANG_NAME_MAP[LANG_EN];
}

PLOGFONT res_getFont(int type) {
	if (type > 0 && type < FONT_TYPE_SIZE && mLogFonts[type]) {
		return mLogFonts[type];
	}
	return mLogFonts[0];
}

static const char *cfgKey2String(int key, char *buff, unsigned int size) {

	int mk = key & 0XFF;
	int sk = (key >> 8) & 0xFF;
	int st = (key >> 16) & 0xFF;
	int flag = (key >> 24) & 0xFF;

	if (mk < 0 || mk >= MK_MAX_ID || sk < 0 || sk >= SK_MAX_ID) {
		buff[0] = '\0';
		return buff;
	}
	if (flag == 0x1) {
		snprintf(buff, size, "%s:%s%d", CONFIG_MKEY[mk], CONFIG_SKEY[sk], st);
	} else {
		snprintf(buff, size, "%s:%s", CONFIG_MKEY[mk], CONFIG_SKEY[sk]);
	}
	return buff;
}

const char *res_getString(int cfgkey) {
	if (!cfgkey) {
		return NULL;
	}
	char inikey[80];
	cfgKey2String(cfgkey, inikey, sizeof(inikey));
	const char *str = map_get_str(&mConfigMap, inikey);
	if (!str) {
		db_error("get config key:%s id:0x%x not found", inikey, cfgkey);
		return NULL;
	}
	return str;
}

const char *res_getString2(int mk, const char *skey) {
	if (mk < 0 || mk >= MK_MAX_ID) {
		db_error("invalid mk:%d", mk);
		return NULL;
	}
	char inikey[80];
	snprintf(inikey, sizeof(inikey), "%s:%s", CONFIG_MKEY[mk], skey);

	const char *str = map_get_str(&mConfigMap, inikey);
	if (!str) {
		db_error("get config mk:%d key:%s not found", mk, inikey);
		return NULL;
	}
	return str;
}

int res_getInt(int mk, const char *skey, int def) {
	const char *str = res_getString2(mk, skey);
	if (is_empty_string(str)) return def;
	return (int) strtol(str, NULL, 0);
}

gal_pixel res_getColor(int mk, const char *skey) {
	unsigned char r, g, b, a;
	const char *str = res_getString2(mk, skey);
	unsigned long hex = is_empty_string(str) ? 0 : strtoul(str, NULL, 0);

	a = (unsigned char) ((hex >> 24) & 0xff);
	r = (unsigned char) ((hex >> 16) & 0xff);
	g = (unsigned char) ((hex >> 8) & 0xff);
	b = (unsigned char) (hex & 0xff);

	//db_msg("getColor key:0x%x hex:%ld a:%x b:%x g:%x r:%x", cfgkey, hex, a, b, g, r);

	return RGBA2Pixel(HDC_SCREEN, r, g, b, a);
}

gal_pixel res_getTextColor(int def) {
	static gal_pixel color = 0;
	static gal_pixel color1 = 0;
	if (def == 0) {
		if (!color) {
			color = res_getColor(MK_system, "text_color");
		}
		return color;
	} else {
		if (!color1)
			color1 = res_getColor(MK_system, "text_color1");
		return color1;
	}

	return color;
}

gal_pixel res_getDisableColor() {
	static gal_pixel color = 0;
	if (!color) {
		color = res_getColor(MK_system, "disable_color");
	}
	return color;
}

gal_pixel res_getTextHColor(int id) {
	static gal_pixel color1 = 0;
	static gal_pixel color2 = 0;
	gal_pixel c;

	if (id == 2) {
		if (!color2) {
			color2 = res_getColor(MK_system, "text_hcolor2");
		}
		c = color2;
	} else {
		if (!color1) {
			color1 = res_getColor(MK_system, "text_hcolor");
		}
		c = color1;
	}
	return c ? c : res_getTextColor(0);
}

gal_pixel res_getBGColor() {
	return res_getBGColor1(0);
}

gal_pixel res_getBGColor1(int def) {
	if (def == 0) {
		return res_getColor(MK_system, "bg_color");
	} else if (def == 1) {
		return res_getColor(MK_system, "bg_color1");
	} else if (def == 2) {
		return res_getColor(MK_system, "bg_hcolor");
	} else {
		return res_getColor(MK_system, "bg_color");
	}
}

int res_getRect(int mkey, const char *skey, WIN_RECT *rect) {
	const char *pos = res_getString2(mkey & 0xFF, skey);
	int n = 0;
	if (!is_empty_string(pos)) {
		n = sscanf(pos, "%d %d %d %d", &(rect->x), &(rect->y), &(rect->w), &(rect->h));
	}
	if (n != 4) {
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	return 0;
}

int res_getPos(int mkey, WIN_RECT *rect) {
	return res_getRect(mkey, "pos", rect);
}

int res_getBmpByKey(int cfgkey, BITMAP *bmp) {
	int st = (cfgkey >> 16) & 0xFF;
	int flag = (cfgkey >> 24) & 0xFF;
	int sp = GET_CSTATE_SP(st);
	if (flag == 0x1 && sp != 0) {
		const char *filepath = res_getString(CFKEYX2(cfgkey & 0xFFFF, DROP_CSTATE_SP(st)));
		if (filepath == NULL) {
			db_error("can not find img path key:0x%x st:%d", cfgkey, st);
			return -1;
		}
		size_t len = strlen(filepath);
		if (len < 5) {
			db_error("error img path key:0x%x st:%d path:%s", cfgkey, st, filepath);
			return -1;
		}
		char path[128] = {0};
		strncpy(path, filepath, len - 4);
		if (sp == CSTATE_SP_CLICK) {
			strcat(path, "_click");
		} else if (sp == CSTATE_SP_ACTIVE) {
			strcat(path, "_active");
		}
		strcat(path, filepath + len - 4);
		db_msg("filepath:%s new path:%s", filepath, path);
		return res_loadBmp(path, bmp);
	} else {
		return res_loadBmp(res_getString(cfgkey), bmp);
	}
}

int res_getBmp(int mk, const char *skey, BITMAP *bmp) {
	return res_loadBmp(res_getString2(mk, skey), bmp);
}

int res_getBmpByState(int mk, int sk, int state, BITMAP *bmp) {
	int cfgkey;
	if (sk == SK_NONE) {
		cfgkey = mk; //mk is cfgkey
	} else {
		cfgkey = CFKEY(mk, sk);
	}
	if (state < 0) {
		return res_getBmpByKey(cfgkey, bmp);
	} else {
		return res_getBmpByKey(CFKEYX2(cfgkey, state), bmp);
	}
}

int res_loadBmp(const char *path, BITMAP *bmp) {
	if (bmp == NULL || path == NULL) {
		db_error("load bitmap error param");
		return -1;
	}
	if (strlen(path) < 1) {
		db_error("load bitmap but path is empty");
		return -1;
	}
	int ret = -1;
	if (!access(path, F_OK)) {
		ret = LoadBitmapFromFile(HDC_SCREEN, bmp, path);
		if (ret == ERR_BMP_OK) {
			db_msg("load bitmap path=%s OK", path);
		} else {
			db_error("load bitmap path=%s failed", path);
		}
	} else {
		db_error("load bitmap path=%s not exist", path);
	}
	return ret;
}

void res_unloadBmp(BITMAP *bmp) {
	if (bmp->bmBits != NULL) {
		UnloadBitmap(bmp);
		bmp->bmBits = NULL;
	}
}

int res_updateLangAndFont(int newLang) {
	if (!IS_VALID_LANG_ID(newLang)) {
		db_error("error lang:%d", newLang);
		return -1;
	}
	if (newLang == mLang)
		return 0;
	mLang = newLang;
	db_msg("new lang is:%d", mLang);
	res_initLangLabel(); /* init lang labels */
	win_send_message(WINDOWID_MAIN, MSG_RM_LANG_CHANGED, (WPARAM) &mLang, 0);
	return 0;
}

int res_parseLabelSetParam(label_set_param *p, const char *setting) {
	memset(p, 0, sizeof(label_set_param));
	int ret = 0;
	if (setting && setting[0] != 0) {
		ret = sscanf(setting, "%d %d %d %d %d %d %d %d", &p->x, &p->y, &p->w, &p->h, &p->style, &p->font, &p->hwnd_index, &p->state);
	}
	return ret;
}

int res_getLabelSetParam(label_set_param *p, int mk, const char *skey) {
	return res_parseLabelSetParam(p, res_getString2(mk, skey));
}
