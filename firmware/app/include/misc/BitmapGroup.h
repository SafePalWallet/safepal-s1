#ifndef WALLET_BITMAPGROUP_H
#define WALLET_BITMAPGROUP_H

#include "resource.h"

class BitmapGroup {
public:
	BitmapGroup();

	~BitmapGroup();

	BITMAP *getBmp(int cfgkey);

	BITMAP *getBmp(int mk, int sk, int state);

	BITMAP *getBmp(int mk, const char *skey);

	BITMAP *getBmp(const char *path);

	int cleanBuffer();

private:
	map_void_t mBuffer;

	BITMAP *_getBmpWithPath(const char *path);

	BITMAP *_getBmp(const char *key, int cfgkey, const char *skey);

	BITMAP *get(const char *key);

	void set(const char *key, BITMAP *bitmap);

};

#endif
