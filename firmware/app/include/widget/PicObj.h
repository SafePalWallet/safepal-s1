
#ifndef __PICOBJ_H__
#define __PICOBJ_H__

#include "minigui_comm.h"

#include "resource.h"

class PicObj {
public:
	PicObj(HWND parent, int id, int cfgkey);

	~PicObj();

	int update(int state, bool autoShow = true, BITMAP *pbitmap = NULL);

	int show();

	int hideInValid(int state);

	int hide();

	int onClick(bool action, int state = -1, int show = -1);

	int onActive(bool action, int state = -1, int show = -1);

	HWND getHwnd() {
		return mHwnd;
	}

private:
	int mId;
	int mCfgKey;
	int mState;

	BITMAP mBmp;
	HWND mHwnd;
	HWND mParent;
	bool mShowing;
	bool mImgValid;
	bool mHideInValid;

	int createHwnd();
};

#endif
