#define LOG_TAG "ListView"

#include "common.h"
#include "ListView.h"

#define IDC_BOTTOM (9999)

ListView::ListView(HWND parent, int group_code) {
	mParent = parent;
	mGroupCode = group_code;
	memset(&group_rect, 0, sizeof(group_rect));
	memset(&pad_rect, 0, sizeof(pad_rect));

	RECT rect;
	GetClientRect(parent, &rect);
	mWinHeight = RECTH(rect);

	db_debug("parent:%d group_code:%d WinHeight:%d", parent, group_code, mWinHeight);

	if (mGroupCode) {
		res_getPos(mGroupCode, &group_rect);
		res_getRect(mGroupCode, "pad_pos", &pad_rect);

		db_msg("group rect:%d %d %d %d pad rect:%d %d %d %d",
		       group_rect.x, group_rect.y, group_rect.w, group_rect.h,
		       pad_rect.x, pad_rect.y, pad_rect.w, pad_rect.h
		);

		if (pad_rect.w == 0 || pad_rect.h == 0) {
			memcpy(&pad_rect, &group_rect, sizeof(group_rect));
		}
		bg_0 = res_getColor(mGroupCode, "bg_color0");
		bg_1 = res_getColor(mGroupCode, "bg_color1");
		bg_h = res_getColor(mGroupCode, "bg_hcolor");

		if (!bg_1) bg_1 = bg_0;
		if (!bg_h) bg_h = bg_0;
	} else {
		bg_0 = 0;
		bg_1 = 0;
		bg_h = 0;
	}

	initValues();
}

void ListView::initValues() {
	mGroups = NULL;
	mIcons = NULL;
	mLabels = NULL;
	mTotalSize = 0;
	mIconSize = 0;
	mLabelSize = 0;
	select_index = -1;
	mScrollSize = 0;
	mBottom = 0;
	mCyclic = false;
	mBottomHwnd = HWND_INVALID;
}

int ListView::init(int total_size, int icon_size, const int *icon_mk_map, int label_size, const int *label_mk_map, const int bottom) {
	if (mTotalSize) {
		cleanWidgets();
	}
	mTotalSize = total_size;
	mBottom = bottom;
	size_t s;
	if (mGroupCode) {
		s = sizeof(HWND *) * mTotalSize;
		mGroups = (HWND *) malloc(s);
		if (!mGroups) {
			mGroupCode = false;
			mTotalSize = 0;
			db_error("malloc false size:%d", s);
			return -1;
		} else {
			memset(mGroups, 0, s);
		}
	}

	if (icon_size > 0) {
		s = sizeof(HWND *) * icon_size * mTotalSize;
		mIcons = (HWND *) malloc(s);
		if (!mIcons) {
			db_error("malloc false size:%d", s);
		} else {
			mIconSize = icon_size;
			memset(mIcons, 0, s);
		}
	}

	if (label_size > 0) {
		s = sizeof(HWND *) * label_size * mTotalSize;
		mLabels = (HWND *) malloc(s);
		if (!mLabels) {
			db_error("malloc false size:%d", s);
		} else {
			mLabelSize = label_size;
			memset(mLabels, 0, s);
		}
	}
	int i;

	if (mGroupCode) {
		createGroupContainer(mGroupCode, pad_rect.y, pad_rect.h, mTotalSize, mGroups);
	}
	for (i = 0; i < mIconSize; i++) {
		createIconWidgets(icon_mk_map[i], pad_rect.y, pad_rect.h, mTotalSize, mIcons + mTotalSize * i);
	}
	for (i = 0; i < mLabelSize; i++) {
		createLabelWidgets(label_mk_map[i], pad_rect.y, pad_rect.h, mTotalSize, mLabels + mTotalSize * i);
	}
	return 0;
}

ListView::~ListView() {
	cleanWidgets();
}

int ListView::clean() {
	int ret = 0;
	ret = cleanWidgets();
	initValues();
	return ret;
}

int ListView::getSelectIndex() {
	return select_index;
}

int ListView::select(int i) {
	if (!supportBottom() && (i == LISTVIEW_INDEX_BOTTOM)) {
		db_error("listView not support bottom");
		return 0;
	}
	db_msg("curr:%d new:%d total:%d", select_index, i, mTotalSize);
	if (i < 0) {
		i = mTotalSize + i;
		if (i < 0) {
			db_error("invalid i:%d", i);
			return 0;
		}
	}
	if (i == select_index) {
		return 0;
	}

	HWND hwnd;
	int totalHeight = 0;
	if (select_index != -1 && (select_index != LISTVIEW_INDEX_BOTTOM)) {
		hwnd = getGroupHwnd(select_index);
		if (IS_VALID_HWND(hwnd)) {
			db_msg("curr:%d hwnd:%d", select_index, hwnd);
			SetWindowBkColor(hwnd, (select_index % 2 == 0) ? bg_0 : bg_1);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		select_index = -1;
	}
	if (i == LISTVIEW_INDEX_BOTTOM) {
		hwnd = mBottomHwnd;
	} else if (i >= 0) {
		hwnd = getGroupHwnd(i);
		db_msg("new:%d hwnd:%d", i, hwnd);
		if (!IS_VALID_HWND(hwnd)) {
			return -1;
		}
		SetWindowBkColor(hwnd, bg_h);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	select_index = i;
	int move = 0;
	int dstOffset = mScrollSize; 
	int curVisibleMinY = mScrollSize;
	int curVisibleMaxY = mScrollSize + mWinHeight;
	totalHeight = pad_rect.y + mTotalSize * pad_rect.h + mBottom;
	if (select_index == LISTVIEW_INDEX_BOTTOM) {
		if (totalHeight > mWinHeight) { // down
			dstOffset = totalHeight - mWinHeight;
		}
		db_msg("bottom select_index:%d dstOffset:%d", select_index, dstOffset);
	} else if (select_index < 0) {
		dstOffset = 0;
		db_msg("select_index:%d dstOffset:%d", select_index, dstOffset);
	} else if (select_index >= 0) {
		int newSelectMinY = pad_rect.y + (select_index) * pad_rect.h;
		int newSelectMaxY = newSelectMinY + pad_rect.h;
		int newDstOffset = 0;
		if (newSelectMinY < curVisibleMinY) {
			newDstOffset = newSelectMaxY - mWinHeight;
			if (newDstOffset < 0) newDstOffset = 0;
			dstOffset = newDstOffset;
		} else if (newSelectMaxY > curVisibleMaxY) {
			newDstOffset = pad_rect.y + select_index * pad_rect.h;
			if (newDstOffset + mWinHeight >= totalHeight) {
				dstOffset = totalHeight - mWinHeight;
			} else {
				dstOffset = newDstOffset;
			}
		} else {
			dstOffset = mScrollSize;
		}
		db_msg("select_index:%d dstOffset:%d", select_index, dstOffset);
	}
	if (mScrollSize != dstOffset) {
		move = mScrollSize - dstOffset;
	}
	db_msg("dstOffset:%d mScrollSize:%d, move:%d", dstOffset, mScrollSize, move);
	if (move) {
		ScrollWindow(mParent, 0, move, NULL, NULL);
		mScrollSize -= move;
		InvalidateRect(mParent, NULL, TRUE);
	}

	return 0;
}

int ListView::move(int dst) {
	db_msg("current:%d dst:%d total:%d cyclic:%d", select_index, dst, mTotalSize, mCyclic);
	int newindex = select_index + dst;
	if (newindex < 0) {
		if (mCyclic) {
			newindex = mTotalSize - 1;
		} else {
			newindex = 0;
		}

	} else if (newindex >= mTotalSize) {
		if (mCyclic) {
			newindex = 0;
		} else {
			newindex = mTotalSize - 1;
		}
	}
	if (newindex == select_index) {
		return 0;
	}
	return select(newindex);
}

int ListView::setMoveCyclic(bool cyclic) {
	mCyclic = cyclic;
	return 0;
}

int ListView::cleanWidgets() {
	int i;
	db_debug("total:%d labels:%d %p icons:%d %p,groups:%d %p", mTotalSize, mLabelSize, mLabels, mIconSize, mIcons, mGroupCode, mGroups);
	if (mLabels) {
		for (i = 0; i < mLabelSize * mTotalSize; i++) {
			if (IS_VALID_HWND(mLabels[i])) {
				DestroyWindow(mLabels[i]);
			}
		}
		free(mLabels);
	}
	if (mIcons) {
		for (i = 0; i < mIconSize * mTotalSize; i++) {
			if (IS_VALID_HWND(mIcons[i])) {
				DestroyWindow(mIcons[i]);
			}
		}
		free(mIcons);
	}
	if (mGroups) {
		for (i = 0; i < mTotalSize; i++) {
			if (IS_VALID_HWND(mGroups[i])) {
				DestroyWindow(mGroups[i]);
			}
		}
		free(mGroups);
	}
	if (IS_VALID_HWND(mBottomHwnd)) {
		DestroyWindow(mBottomHwnd);
		mBottomHwnd = HWND_INVALID;
	}
	initValues();
	return 0;
}

int ListView::createGroupContainer(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds) {
	HWND retWnd;
	int lastOffsetY = 0;
	for (int i = 0; i < create_num; i++) {
		retWnd = CreateWindowEx(CTRL_STATIC, "",
		                        WS_CHILD | WS_VISIBLE,
				//     WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
				                WS_EX_USEPARENTFONT,
				                i,
				                group_rect.x, pos_offset + group_rect.y + block_height * i, group_rect.w, group_rect.h,
				                mParent, (DWORD) this);
		db_debug("i:%d x:%d y:%d w:%d h:%d retWnd:%d", i, group_rect.x, pos_offset + group_rect.y + block_height * i, group_rect.w, group_rect.h, retWnd);
		if (!IS_VALID_HWND(retWnd)) {
			db_error("create label false failed");
			return -1;
		}
		SetWindowBkColor(retWnd, (i % 2 == 0) ? bg_0 : bg_1);
		hwnds[i] = retWnd;
	}
	lastOffsetY = pos_offset + group_rect.y + block_height * create_num;
	if (supportBottom()) {
		mBottomHwnd = CreateWindowEx(CTRL_STATIC, "",
		                             WS_CHILD | WS_VISIBLE,
		                             WS_EX_USEPARENTFONT,
		                             IDC_BOTTOM,
		                             group_rect.x, lastOffsetY, group_rect.w, mBottom,
		                             mParent, (DWORD) this);
	}

	return 0;
}

int ListView::createIconWidgets(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds) {
	int retval;
	WIN_RECT rect;
	HWND retWnd;
	HWND hwnd;
	retval = res_getPos(config_code, &rect);
	if (retval < 0 || rect.w <= 1) {
		db_error("get rect failed code:0x%x ret:%d,w:%d", config_code, retval, rect.w);
		return -1;
	}
	for (int i = 0; i < create_num; i++) {
		hwnd = mGroupCode ? mGroups[i] : mParent;
		retWnd = CreateWindowEx(CTRL_STATIC, "",
		                        WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
		                        WS_EX_TRANSPARENT,
		                        i,
		                        rect.x, mGroupCode ? rect.y : (pos_offset + rect.y + block_height * i), rect.w, rect.h,
		                        hwnd, 0);

		db_debug("i:%d x:%d y:%d w:%d h:%d retWnd:%d hwnd:%d", i, rect.x, pos_offset + rect.y + block_height * i, rect.w, rect.h, retWnd, hwnd);
		if (!IS_VALID_HWND(retWnd)) {
			db_error("create label false code:%x failed", config_code);
			return -1;
		}
		hwnds[i] = retWnd;
	}
	return 0;
}

int ListView::createLabelWidgets(int config_code, int pos_offset, int block_height, int create_num, HWND *hwnds) {
	int retval;
	WIN_RECT rect;
	HWND retWnd;
	HWND hwnd;
	int style_config = 0;
	int hwnd_index_config = 0;
	int state_config = 0;
	int font = 0;
	retval = res_getPos(config_code, &rect);
	if (retval < 0 || rect.w <= 1) {
		db_error("get rect failed code:0x%x ret:%d,w:%d", config_code, retval, rect.w);
		return -1;
	}
	const char *configstr = res_getString2(config_code, "style");
	if (configstr != NULL && strlen(configstr) > 0) {
		sscanf(configstr, "%d %d", &(style_config), &(font));
	}

	configstr = res_getString2(config_code, "state");
	if (configstr != NULL && strlen(configstr) > 0) {
		sscanf(configstr, "%d %d", &(hwnd_index_config), &(state_config));
	}


	DWORD style = SS_SIMPLE;
	if (style_config) {
		if (style_config & LABEL_STYLE_LEFT) {
			style |= SS_LEFT;
		} else if (style_config & LABEL_STYLE_CENTER) {
			style |= SS_CENTER;
		} else if (style_config & LABEL_STYLE_RIGHT) {
			style |= SS_RIGHT;
		}
	}

	for (int i = 0; i < create_num; i++) {
		hwnd = mGroupCode ? mGroups[i] : mParent;
		retWnd = CreateWindowEx(CTRL_STATIC, "",
		                        WS_CHILD | WS_VISIBLE | style,
		                        WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
		                        i,
		                        rect.x, mGroupCode ? rect.y : (pos_offset + rect.y + block_height * i), rect.w, rect.h,
		                        hwnd, 0);
		db_debug("i:%d x:%d y:%d w:%d h:%d retWnd:%d hwnd:%d", i, rect.x, pos_offset + rect.y + block_height * i, rect.w, rect.h, retWnd, hwnd);
		if (retWnd == HWND_INVALID) {
			db_error("create label false code:%x failed", config_code);
			return -1;
		}
		setLabelWindowAttr(retWnd, style_config, font);

		hwnds[i] = retWnd;
	}
	return 0;
}

HWND ListView::getGroupHwnd(int g) {
	return (mGroups && g >= 0 && g < mTotalSize) ? mGroups[g] : HWND_INVALID;
}

HWND ListView::getIconHwnd(int g, int index) {
	if (mIcons && g >= 0 && g < mTotalSize && index >= 0 && index < mIconSize) {
		return mIcons[mTotalSize * index + g];
	}
	return HWND_INVALID;
}

HWND ListView::getLabelHwnd(int g, int index) {
	if (mLabels && g >= 0 && g < mTotalSize && index >= 0 && index < mLabelSize) {
		return mLabels[mTotalSize * index + g];
	}
	return HWND_INVALID;
}


HWND ListView::getBottomHwnd() {
	if (!supportBottom()) {
		return HWND_INVALID;
	}
	return mBottomHwnd;
}

int ListView::supportBottom() {
	if (mBottom > 0) {
		return 1;
	}
	return 0;
}

int ListView::setLabelText(int g, int index, const char *text) {
	HWND hwnd = getLabelHwnd(g, index);
	db_debug("g:%d index:%d hwnd:%d text:%s", g, index, hwnd, text);
	if (IS_VALID_HWND(hwnd)) {
		SetWindowMText(hwnd, text);
		return 0;
	}
	return -1;
}

int ListView::setIconImage(int g, int index, BITMAP *pbitmap) {
	HWND hwnd = getIconHwnd(g, index);
	if (IS_VALID_HWND(hwnd)) {
		if (pbitmap) {
			db_debug("g:%d index:%d hwnd:%d bitmap:%d x %d", g, index, hwnd, pbitmap->bmWidth, pbitmap->bmHeight);
			SendMessage(hwnd, STM_SETIMAGE, (WPARAM) pbitmap, 0);
			InvalidateRect(hwnd, NULL, TRUE);
			ShowWindow(hwnd, SW_SHOWNORMAL);
		} else {
			ShowWindow(hwnd, SW_HIDE);
		}
		return 0;
	}
	return -1;
}
