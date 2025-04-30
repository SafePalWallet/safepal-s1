#define LOG_TAG "comm"

#include "CommonWindow.h"
#include "minigui_comm.h"
#include "key_event.h"
#include "Dialog.h"
#include "GuiMain.h"
#include "win.h"

#define dbx_verbose(ft, arg...) db_verbose("%s " ft,win_get_name(mWindowId),##arg)
#define dbx_msg(ft, arg...) db_msg("%s " ft,win_get_name(mWindowId),##arg)
#define dbx_error(ft, arg...) db_error("%s " ft,win_get_name(mWindowId),##arg)

PROC_RET CommonWindowProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	CommonWindow *win = (CommonWindow *) GetWindowAdditionalData(hWnd);
	return win ? win->commonWinProc(hWnd, message, wParam, lParam) : DefaultWindowProc(hWnd, message, wParam, lParam);
}

CommonWindow::CommonWindow() {
	mHwnd = HWND_INVALID;
	mWindowId = 0;
	mIconSize = 0;
	mLabelSize = 0;
	mIcons = NULL;
	mLabels = NULL;
	mIconParams = NULL;
	mLabelParams = NULL;

	mScreenWidth = SCREEN_WIDTH;
	mScreenHeight = SCREEN_HEIGHT;
	mTotalHeight = mScreenHeight;
	mScrollSize = 0;
}

CommonWindow::~CommonWindow() {
	for (int i = 0; i < mIconSize; i++) {
		if (mIcons[i] != NULL) {
			delete mIcons[i];
		}
	}
	if (IS_VALID_HWND(mHwnd)) DestroyWindow(mHwnd);
}

int CommonWindow::createMainWindow(int winid, HWND hParentWnd) {
	mWindowId = winid;
	dbx_msg("create win:%d", winid);
	mHwnd = CreateWindowEx(WINDOW_COMMON, "",
	                       WS_CHILD,
	                       WS_EX_USEPARENTFONT,
	                       mWindowId, 0, 0, mScreenWidth, mScreenHeight, hParentWnd, (DWORD) this);
	if (!IS_VALID_HWND(mHwnd)) {
		dbx_error("create window:%d failed", winid);
		return -1;
	}
	return 0;
}

int CommonWindow::isHold() {
	return 1;
}

PROC_RET CommonWindow::commonWinProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	if (message != STBM_TIME_CLICK && message != MSG_IDLE) {
		dbx_verbose("recived msg:0x%x %s, wParam: 0x%x lParam:0x%x", message, Message2Str(message), (int) wParam, (int) lParam);
		if (message == MSG_LBUTTONDOWN || message == MSG_LBUTTONUP || message == MSG_NCMOUSEMOVE) {
			dbx_verbose("TPEVENT msg:0x%x %s x:%d y:%d", message, Message2Str(message), LOWORD(lParam), HIWORD(lParam));
		}
	}
	switch (message) {
		case MSG_INPUT_KEY:
			return keyProc(wParam, (int) lParam);
		case MSG_CREATE:
			if (onCreate(hWnd) < 0) {
				dbx_error("create sub widgets failed");
			}
			break;
		case MSG_SHOWWINDOW:
			if (wParam == SW_HIDE) {
				onPause();
			} else if (wParam == SW_SHOWNORMAL || wParam == SW_SHOW) {
				onResume();
			}
			break;
		case MSG_DESTROY:
			onDestory();
			break;
	}
	return winProc(hWnd, message, wParam, lParam);
}

int CommonWindow::initLayout(int icon_size, const int *icon_mk_map, int label_size, const int *label_mk_map) {
	int i;
	if (icon_size > 0) {
		mIcons = (PicObj **) malloc(sizeof(PicObj *) * icon_size);
		if (!mIcons) {
			dbx_error("memory false");
			return -1;
		}
		mIconParams = (layout_icon_param *) malloc(sizeof(layout_icon_param) * icon_size);
		if (!mIconParams) {
			dbx_error("memory false");
			return -1;
		}
		memset(mIcons, 0, sizeof(PicObj *) * icon_size);
		memset(mIconParams, 0, sizeof(layout_icon_param) * icon_size);
		for (i = 0; i < icon_size; i++) {
			mIconParams[i].config_code = icon_mk_map[i];
			mIconParams[i].state = STATUS_ALL;
		}
	}
	mIconSize = icon_size;

	if (label_size > 0) {
		mLabels = (HWND *) malloc(sizeof(HWND *) * label_size);
		if (!mLabels) {
			dbx_error("memory false");
			return -1;
		}
		mLabelParams = (layout_label_param *) malloc(sizeof(layout_label_param) * label_size);
		if (!mLabelParams) {
			dbx_error("memory false");
			return -1;
		}

		memset(mLabelParams, 0, sizeof(layout_label_param) * label_size);
		for (i = 0; i < label_size; i++) {
			mLabels[i] = HWND_INVALID;
			mLabelParams[i].config_code = label_mk_map[i];
			mLabelParams[i].state = STATUS_ALL;
			mLabelParams[i].style = LABEL_STYLE_CENTER;
		}
	}
	mLabelSize = label_size;

	return 0;
}

int CommonWindow::createLayoutWidgets(HWND hParent, const HWND *subHwnds) {
	HWND retWnd;
	WIN_RECT rect;

	HWND hwnd;
	HWND hwnds[4];
	hwnds[0] = hParent;
	hwnds[1] = hParent;
	hwnds[2] = hParent;
	hwnds[3] = hParent;
	if (subHwnds) {
		if (IS_VALID_HWND(subHwnds[0])) hwnds[1] = subHwnds[0];
		if (IS_VALID_HWND(subHwnds[1])) hwnds[2] = subHwnds[1];
		if (IS_VALID_HWND(subHwnds[2])) hwnds[3] = subHwnds[2];
	}

	int id;
	int retval;
	const char *configstr;
	//create icons
	for (int i = 0; i < mIconSize; i++) {
		layout_icon_param *p = &mIconParams[i];
		id = i;
		if (!p->config_code) {
			continue;
		}
		retval = res_getPos(p->config_code, &rect);
		if (retval < 0 || rect.w <= 1) {
			dbx_error("get rect id:%d code:0x%x failed ret:%d,w:%d", id, p->config_code, retval, rect.w);
			continue;
		}
		configstr = res_getString2(p->config_code, "state");
		if (configstr != NULL && strlen(configstr) > 0) {
			sscanf(configstr, "%d %d", &(p->hwnd_index), &(p->state));
		}

		if (p->hwnd_index < 0 || p->hwnd_index > 3 || p->state == 0) {
			dbx_error("skip create icon,id:%d code:0x%x hwnd_index:%d state:%d", i, p->config_code, p->hwnd_index, p->state);
			continue;
		}
		hwnd = hwnds[p->hwnd_index];
		if (!IS_VALID_HWND(hwnd)) {
			continue;
		}
		mIcons[id] = new PicObj(hwnd, id, ICON_KEY(p->config_code));
	}
	//create string label
	for (int i = 0; i < mLabelSize; i++) {
		layout_label_param *p = &mLabelParams[i];
		id = i;
		if (!p->config_code) {
			continue;
		}
		retval = res_getPos(p->config_code, &rect);
		if (retval < 0 || rect.w <= 1) {
			dbx_error("get rect id:%d code:0x%x failed ret:%d,w:%d", id, p->config_code, retval, rect.w);
			continue;
		}

		configstr = res_getString2(p->config_code, "style");
		if (configstr != NULL && strlen(configstr) > 0) {
			sscanf(configstr, "%d %d", &(p->style), &(p->font));
		}

		configstr = res_getString2(p->config_code, "state");
		if (configstr != NULL && strlen(configstr) > 0) {
			sscanf(configstr, "%d %d", &(p->hwnd_index), &(p->state));
		}

		if (p->hwnd_index < 0 || p->hwnd_index > 3) {
			dbx_error("skip create label,id:%d code:0x%x hwnd_index:%d state:%d", i, p->config_code, p->hwnd_index, p->state);
			continue;
		}

		hwnd = hwnds[p->hwnd_index];
		if (!IS_VALID_HWND(hwnd)) {
			continue;
		}
		DWORD style = SS_SIMPLE;
		if (p->style) {
			if (p->style & LABEL_STYLE_LEFT) {
				style |= SS_LEFT;
			} else if (p->style & LABEL_STYLE_CENTER) {
				style |= SS_CENTER;
			} else if (p->style & LABEL_STYLE_RIGHT) {
				style |= SS_RIGHT;
			}
		}
		retWnd = CreateWindowEx(CTRL_STATIC, "",
		                        WS_CHILD | WS_VISIBLE | style,
		                        WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
		                        id,
		                        rect.x, rect.y, rect.w, rect.h,
		                        hwnd, 0);
		if (retWnd == HWND_INVALID) {
			dbx_error("create label false id:%x code:%x failed", id, p->config_code);
			return -1;
		}
		setLabelWindowAttr(retWnd, p->style, p->font);
		mLabels[id] = retWnd;
	}

	//init icons
	for (id = 0; id < mIconSize; id++) {
		if (mIcons[id]) {
			int state = getIconState(id);
			mIcons[id]->update(state, false);
			onUpdateIcon(id, state);
		}
	}
	return 0;
}

int CommonWindow::destoryLayoutWidgets() {
	return 0;
}

HWND CommonWindow::getIconHwnd(int id) {
	if (id >= 0 && id < mIconSize && mIcons[id]) {
		return mIcons[id]->getHwnd();
	}
	return HWND_INVALID;
}

HWND CommonWindow::getLabelHwnd(int id) {
	if (id >= 0 && id < mLabelSize && IS_VALID_HWND(mLabels[id])) {
		return mLabels[id];
	}
	return HWND_INVALID;
}

int CommonWindow::updateIcon(int id, bool show) {
	return updateIcon(id, getIconState(id), show);
}

int CommonWindow::updateIcon(int id, int state, bool show) {
	if (id < 0 || id >= mIconSize) {
		dbx_msg("error icon id:%d", id);
		return -1;
	}
	if (!mIcons[id]) {
		dbx_msg("not init icon id:%d", id);
		return -1;
	}
	if (state >= 0) {
		onUpdateIcon(id, state);
		return mIcons[id]->update(state, show);
	} else {
		return mIcons[id]->hideInValid(state);
	}
}

int CommonWindow::showIcon(int id, bool on) {
	if (id < 0 || id >= mIconSize) {
		dbx_msg("error icon id:%d", id);
		return -1;
	}
	if (!mIcons[id]) {
		dbx_msg("not init icon id:%d", id);
		return -1;
	}
	if (on) {
		return mIcons[id]->show();
	} else {
		return mIcons[id]->hide();
	}
}

int CommonWindow::setLabelText(int id, const char *text, int show) {
	if (id < 0 || id >= mLabelSize) {
		dbx_msg("error label id:%d", id);
		return -1;
	}
	if (!IS_VALID_HWND(mLabels[id])) {
		dbx_msg("not init label id:%d", id);
		return -1;
	}
	SetWindowMText(mLabels[id], text);
	if (show == 0) {
		ShowWindow(mLabels[id], SW_HIDE);
	} else if (show == 1) {
		ShowWindow(mLabels[id], SW_SHOWNORMAL);
	}
	return 0;
}

void CommonWindow::flushLayoutWidget(int state) {
	dbx_msg("state:%d", state);
	bool show;
	int id;
	//flush icons
	for (int i = 0; i < mIconSize; i++) {
		layout_icon_param *p = &mIconParams[i];
		id = i;
		if (mIcons[id]) {
			show = HAVE_ANY_STATUS(state, p->state);
			if (show) {
				mIcons[id]->show();
			} else {
				mIcons[id]->hide();
			}
		}
	}

	//flush string label
	for (int i = 0; i < mLabelSize; i++) {
		layout_label_param *p = &mLabelParams[i];
		id = i;
		if (IS_VALID_HWND(mLabels[id])) {
			show = HAVE_ANY_STATUS(state, p->state);
			if (show) {
				if (!IsWindowVisible(mLabels[id])) {
					ShowWindow(mLabels[id], SW_SHOWNORMAL);
				}
			} else {
				if (IsWindowVisible(mLabels[id])) {
					ShowWindow(mLabels[id], SW_HIDE);
				}
			}
		}
	}
}

// -1 up 1 down
int CommonWindow::scrollWindow(int direction) {
	int screen_h = mScreenHeight;
#if 0
	screen_h = mScreenHeight - 5;
#endif
	int move_h = 0;
	if (direction == -1) {
		if (mScrollSize > 0) {
			move_h = mScrollSize % screen_h;
			if (!move_h) move_h = screen_h;
			ScrollWindow(mHwnd, 0, move_h, NULL, NULL);
			mScrollSize -= move_h;
			onScrollWindow(-move_h);
			InvalidateRect(mHwnd, NULL, TRUE);
			dbx_msg("up current scroll:%d -> %d move_h:%d", mScrollSize, mScrollSize + move_h, move_h);
		} else {
			dbx_msg("up current scroll:%d skip...", mScrollSize);
		}
	} else if (direction == 1) {
		if (mScrollSize + screen_h > mTotalHeight) {
			dbx_msg("down end ScrollSize:%d TotalHeight:%d", mScrollSize, mTotalHeight);
			return 0;
		}
		move_h = mTotalHeight - (mScrollSize + screen_h);
		if (move_h > screen_h) move_h = screen_h;
		ScrollWindow(mHwnd, 0, -move_h, NULL, NULL);
		mScrollSize += move_h;
		onScrollWindow(move_h);
		InvalidateRect(mHwnd, NULL, TRUE);
		dbx_msg("down current scroll:%d -> %d move_h:%d", mScrollSize - move_h, mScrollSize, move_h);
	} else {
		return 0;
	}
	return direction * move_h;
}

int CommonWindow::resetScrollSize() {
	RECT rect;
	GetWindowRect(mHwnd, &rect);
	dbx_msg("rect x:%d y:%d w:%d h:%d mScrollSize:%d", rect.left, rect.top, RECTW(rect), RECTH(rect), mScrollSize);
	if (mScrollSize != 0) {
		int move_h = mScrollSize;
		ScrollWindow(mHwnd, 0, mScrollSize, NULL, NULL);
		mScrollSize = 0;
		onScrollWindow(-move_h);
		InvalidateRect(mHwnd, NULL, TRUE);
		GetWindowRect(mHwnd, &rect);
		dbx_msg("rect x:%d y:%d w:%d h:%d", rect.left, rect.top, RECTW(rect), RECTH(rect));
	}
	return 0;
}

int CommonWindow::changeWindow(int windowID) {
	GuiMain::getInstance()->changeWindow(windowID);
	return 0;
}