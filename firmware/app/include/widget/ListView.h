#ifndef WALLET_LISTVIEW_H
#define WALLET_LISTVIEW_H

#include "minigui_comm.h"
#include "PicObj.h"

enum {
	LISTVIEW_INDEX_HEADER = -1,
	LISTVIEW_INDEX_BOTTOM = 100000
};

class ListView {

public:
	ListView(HWND parent, int group_code);

	~ListView();

	int init(int total_size, int icon_size, const int *icon_mk_map, int label_size, const int *label_mk_map, const int bottom = 0);

	HWND getGroupHwnd(int g);

	HWND getIconHwnd(int g, int index);

	HWND getLabelHwnd(int g, int index);

	HWND getBottomHwnd();

	int supportBottom();

	int setLabelText(int g, int index, const char *text);

	int setIconImage(int g, int index, BITMAP *pbitmap);

	int clean();

	int getSelectIndex();

	int select(int i);

	int move(int dst);

	int setMoveCyclic(bool cyclic);

private:
	void initValues();

	int createGroupContainer(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds);

	int createIconWidgets(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds);

	int createLabelWidgets(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds);

	int cleanWidgets();

	int select_index;
	HWND mParent;
	int mGroupCode;
	int mWinHeight;
	int mScrollSize;
	int mBottom;
	bool mCyclic;

	gal_pixel bg_0;
	gal_pixel bg_1;
	gal_pixel bg_h;

	WIN_RECT group_rect;
	WIN_RECT pad_rect;

	int mTotalSize;
	int mIconSize;
	int mLabelSize;
	HWND *mGroups;
	HWND *mIcons;
	HWND *mLabels;
	HWND mBottomHwnd;
};


#endif
