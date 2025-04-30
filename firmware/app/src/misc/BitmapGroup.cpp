#define LOG_TAG "BitmapGroup"

#include "common.h"
#include "BitmapGroup.h"

BitmapGroup::BitmapGroup() {
	map_init(&mBuffer);
}

BitmapGroup::~BitmapGroup() {
	cleanBuffer();
}

BITMAP *BitmapGroup::getBmp(int cfgkey) {
	char key[20];
	snprintf(key, sizeof(key), "%d", cfgkey);
	return _getBmp(key, cfgkey, NULL);
}

BITMAP *BitmapGroup::getBmp(int mk, int sk, int state) {
	int cfgkey;
	if (sk == SK_NONE) {
		cfgkey = mk; //mk is cfgkey
	} else {
		cfgkey = CFKEY(mk, sk);
	}
	if (state < 0) {
		return getBmp(cfgkey);
	} else {
		return getBmp(CFKEYX2(cfgkey, state));
	}
}

BITMAP *BitmapGroup::getBmp(int mk, const char *skey) {
	char key[64];
	if (!skey) {
		return NULL;
	}
	snprintf(key, sizeof(key), "%d_%s", mk, skey);
	return _getBmp(key, mk, skey);
}

BITMAP *BitmapGroup::getBmp(const char *path) {
	return _getBmpWithPath(path);
}


BITMAP *BitmapGroup::_getBmp(const char *key, int mk, const char *skey) {
	BITMAP *bitmap = get(key);
	if (bitmap) {
		db_msg("buffed bitmap key:%s", key);
		return bitmap;
	}
	bitmap = (BITMAP *) malloc(sizeof(BITMAP));
	if (!bitmap) {
		db_error("memory false");
		return NULL;
	}
	memset(bitmap, 0, sizeof(BITMAP));
	if (skey == NULL) {
		if (res_getBmpByKey(mk, bitmap) == 0) {
			set(key, bitmap);
			return bitmap;
		}
	} else {
		if (res_getBmp(mk, skey, bitmap) == 0) {
			set(key, bitmap);
			return bitmap;
		}
	}
	db_error("unable load bitmap:%s", key);
	free(bitmap);
	return NULL;
}

BITMAP *BitmapGroup::_getBmpWithPath(const char *path) {
	if (NULL == path) {
		db_error("path:%p null", path);
		return NULL;
	}
	if (strlen(path) <= 0) {
		db_error("get bmp path:%s false", path);
		return NULL;
	}

	BITMAP *bitmap = get(path);
	if (bitmap) {
		db_msg("buffed bitmap key:%s", path);
		return bitmap;
	}
	bitmap = (BITMAP *) malloc(sizeof(BITMAP));
	if (!bitmap) {
		db_error("memory false");
		return NULL;
	}
	memset(bitmap, 0, sizeof(BITMAP));
	if (res_loadBmp(path, bitmap) == 0) {
		set(path, bitmap);
		return bitmap;
	}
	db_error("unable load bitmap:%s", path);
	free(bitmap);
	return NULL;
}

int BitmapGroup::cleanBuffer() {
	const char *key;
	BITMAP *bitmap;
	map_iter_t iter = map_iter(&mBuffer);
	while ((key = map_next(&mBuffer, &iter))) {
		bitmap = get(key);
		db_msg("clean:%s", key);
		db_msg("clean:%s bitmap:%p w:%d h:%d", key, bitmap, bitmap->bmWidth, bitmap->bmHeight);
		UnloadBitmap(bitmap);
	}
	map_clean(&mBuffer);
	return 0;
}

BITMAP *BitmapGroup::get(const char *key) {
	void *t = map_get_void_p(&mBuffer, key);
	return t ? (BITMAP *) t : NULL;
}

void BitmapGroup::set(const char *key, BITMAP *bitmap) {
	db_msg("new buffer key:%s", key);
	map_set(&mBuffer, key, bitmap);
}
