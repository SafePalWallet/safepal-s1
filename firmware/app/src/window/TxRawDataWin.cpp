#define LOG_TAG "TxRawDataWin"

#include <stdlib.h>
#include <CommonWindow.h>
#include <widgets.h>
#include "GuiMain.h"
#include "TxRawDataWin.h"
#include "debug.h"
#include "wallet_util_hw.h"
#include "coin_util_hw.h"

enum {
	TXS_ICON_COIN_TYPE = 0,
	TXS_ICON_NAVI_UP_DOWN,
	TXS_ICON_NAVI_CANCEL,
	TXS_ICON_NAVI_OK,
    TXS_ICON_MAXID,
};

enum {
	TXS_LABEL_COIN_SYMBOL,
	TXS_LABEL_COIN_NAME,
	TXS_LABEL_CANCEL,
	TXS_LABEL_MAXID,
};

#define MAX_SEGMENTS 7
#define MAX_SEG_LEN 290
#define FIRST_SEG_LEN 200

TxRawDataWin::TxRawDataWin() {
	int icon_mk_map[TXS_ICON_MAXID] = {
			MK_sign_icon_coin_type,
			MK_sign_icon_navi_up_down,
			MK_sign_icon_navi_cancel_1,
			MK_sign_icon_navi_ok,
    };

	int label_mk_map[TXS_LABEL_MAXID] = {
			MK_sign_label_coin_symbol,
            MK_cmsg_label_title1,
            MK_sign_label_cancel_1,
	};

	memset(mDView, 0, sizeof(DynamicViewCtx));
	memset(mTxp, 0, sizeof(TxPorcessData));

	mHwndNaviPanel = HWND_INVALID;
	mMsgFrom = MSG_FROM_QR_APP;
	mClientMessage = NULL;
	mShowRet = -1;
    mRawDataStr = NULL;
	mBitmapLogo = (BITMAP *) calloc(1, sizeof(BITMAP));
	initLayout(TXS_ICON_MAXID, icon_mk_map, TXS_LABEL_MAXID, label_mk_map);
}

TxRawDataWin::~TxRawDataWin() {
	if (mBitmapLogo) {
		res_unloadBmp(mBitmapLogo);
	}
    if (mRawDataStr) {
        free(mRawDataStr);
        mRawDataStr = NULL;
    }
}

PROC_RET TxRawDataWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case MSG_HISTORY_QR_RESULT:
        case MSG_TXSHOW_MSG: {
            mMsgFrom = (message == MSG_HISTORY_QR_RESULT) ? MSG_FROM_QR_HISTORY : MSG_FROM_QR_APP;
			onSignReqData((ProtoClientMessage *) lParam);
		}
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int TxRawDataWin::keyProc(int keyCode, int isLongPress) {
	switch (keyCode) {
		case INPUT_KEY_OK:
            db_msg("INPUT_KEY_OK");
            if (mMsgFrom == MSG_FROM_QR_APP && mShowRet == 0) {
                if (!gProcessing) {
                    gProcessing = 1;
                    doSignReq();
                    gProcessing = 0;
                }
            }
            break;
		case INPUT_KEY_UP:
			scrollWindow(-1);
			break;
		case INPUT_KEY_DOWN:
			scrollWindow(1);
			break;
		case INPUT_KEY_LEFT:
			return WINDOWID_TXSHOW;
		case INPUT_KEY_RIGHT:
			break;
	}
	return 0;
}

int TxRawDataWin::getIconState(int id) {
	switch (id) {
		case TXS_ICON_NAVI_CANCEL:
		case TXS_ICON_NAVI_OK:
			return 0;
	}
	return -1;
}

int TxRawDataWin::onCreate(HWND hWnd) {
	int ret;
	WIN_RECT rect;

	res_getPos(MK_sign_navi_panel, &rect);
	mHwndNaviPanel = CreateWindowEx(CTRL_STATIC, "",
	                                WS_CHILD | WS_VISIBLE,
	                                WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
	                                1,
	                                rect.x, rect.y, rect.w, rect.h,
	                                hWnd, (DWORD) this);

	if (mHwndNaviPanel == HWND_INVALID) {
		db_error("create status bar failed");
		return -1;
	}
	ShowWindow(mHwndNaviPanel, SW_SHOWNORMAL);

	HWND subs[3];
	subs[0] = mHwndNaviPanel;
	subs[1] = HWND_INVALID;
	subs[2] = HWND_INVALID;
	ret = createLayoutWidgets(hWnd, subs);
	if (ret != 0) {
		db_error("createLayoutWidgets ret:%d", ret);
		return ret;
	}
	return 0;
}

static int splitString(const char *input, int segment_length, char segments[MAX_SEGMENTS][MAX_SEG_LEN]) {
    int total_segments = 0;
    int len = input ? strlen(input) : 0;

    if ((segment_length <= 0) || (segment_length >= MAX_SEG_LEN)) {
        return -10;
    }

    if (len == 0) {
        return -11;
    }

    //memset
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        segments[i][0] = '\0';
    }

    //first segment
    int first_len = len > FIRST_SEG_LEN ? FIRST_SEG_LEN : len;
    strncpy(segments[0], input, first_len);
    segments[0][first_len] = '\0';
    total_segments = 1;
    if (len <= FIRST_SEG_LEN) {
        return total_segments;
    }

    //remaining segment
    int remaining_len = len - FIRST_SEG_LEN;
    const char* remaining_str = input + FIRST_SEG_LEN;
    for (int i = 0; i < remaining_len && total_segments < MAX_SEGMENTS; i += segment_length, total_segments++) {
        int seg_len = segment_length;
        if (i + seg_len > remaining_len) {
            seg_len = remaining_len - i;
        }

        if (seg_len > MAX_SEG_LEN - 1) {
            seg_len = MAX_SEG_LEN - 1;
        }

        strncpy(segments[total_segments], remaining_str + i, seg_len);
        segments[total_segments][seg_len] = '\0';

        if (total_segments == MAX_SEGMENTS - 1 && i + seg_len < remaining_len && seg_len >= 3) {
            int truncate_pos = seg_len - 3;
            if (truncate_pos > 0) {
                strcpy(segments[total_segments] + truncate_pos, "...");
            } else {
                strcpy(segments[total_segments], "...");
            }
        }
    }

    return total_segments;
}

static int GetExtHeaderLen(const ProtoClientMessage *msg) {
    size_t len = 0;
    if (msg && msg->data) {
        if (msg->flag & QR_FLAG_HAS_TIME) {
            len += 6;
            if (msg->data->len < len) { //error
                db_error("invalid data len:%d < time len:6", msg->data->len);
                return -1;
            }
        }
        if (msg->flag & QR_FLAG_EXT_HEADER) {
            if (msg->data->len < (len + 11)) {
                db_error("invalid data len:%d from len:%d", msg->data->len, len);
                return -2;
            }
            if (msg->data->str[len] != 0x7a) { //tag string 15
                return -3;
            }
            uint32_t low = 0;
            uint32_t hi = 0;
            len += 1;
            len += pb_decode((uint8_t *) (msg->data->str + len), &low, &hi); //varlen
            if (hi != 0 || low >= 0x4000) {
                db_error("invalid ext header var len:%d %d", low, hi);
                return -4;
            }
            len += low;
            if (msg->data->len < len) {
                db_error("invalid data len:%d < %d varlen:%d", msg->data->len, len, low);
                return -5;
            }
        }
    }
    return (int) len;
}

int TxRawDataWin::onResume() {
	char tmpbuf[128];
	db_msg("resume");
	set_temp_screen_time(60);
	if (!mClientMessage) {
		db_error("invalid client msg");
		return -1;
	}
	freeWinData();
	initDView();
	if (mScrollSize) {
		resetScrollSize();
	}
	int ret = tx_process_client_message(mClientMessage, mTxp);
	if (ret != 0) {
		dialog_system_error2(mHwnd, ret, "init", NULL);
		return 0;
	}
	if (!mTxp->onShow || !mTxp->onInit) {
		db_error("not show or init func");
		return -1;
	}
	mShowRet = mTxp->onInit(mTxp->session);
	if (mShowRet != 0) {
		db_error("TX init ret:%d", mShowRet);
		dialog_error3(mHwnd, mShowRet, "Init tx failed.");
		return -1;
	}
	mShowRet = mTxp->onShow(mTxp->session, mDView);
	db_msg("TX show ret:%d", mShowRet);
    if (mShowRet < 0) {
        if (mShowRet == -181) {
            dialog_error(mHwnd, res_getLabel(LANG_LABEL_WALLET_NO_SUPPORT_TOKEN));
        } else if (mShowRet == UNSUPPORT_MSG_UPGRADE_TRY_AGAIN) {
            dialog_error(mHwnd, res_getLabel(LANG_LABEL_UNSUPPORT_MSG));
        } else {
            dialog_error3(mHwnd, mShowRet, "Show tx info failed.");
        }
        return -1;
    }

    if (mMsgFrom == MSG_FROM_QR_HISTORY) {
        showIcon(TXS_ICON_NAVI_OK, false);
        setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_BACK));
    } else {
        showIcon(TXS_ICON_NAVI_OK, true);
        setLabelText(TXS_LABEL_CANCEL, res_getLabel(LANG_LABEL_BACK));
    }

    //symbol
	if (is_not_empty_string(mDView->coin_symbol)) {
		ret = 1; //font
		if (mDView->flag & 0x1) { //small
			ret = 8;
		}
		SetWindowFont(getLabelHwnd(TXS_LABEL_COIN_SYMBOL), res_getFont(ret));
		setLabelText(TXS_LABEL_COIN_SYMBOL, mDView->coin_symbol, 1);
	} else {
		setLabelText(TXS_LABEL_COIN_SYMBOL, "", 0);
	}

    //name
    setLabelText(TXS_LABEL_COIN_NAME, "Raw Data", 1);

	//logo
	PicObj *coin_icon = mIcons[TXS_ICON_COIN_TYPE];
	db_msg("coin_icon:%p mBitmapLogo:%p", coin_icon, mBitmapLogo);
	if (coin_icon && mBitmapLogo) {
		tmpbuf[0] = 0;
		int coin_type = mDView->coin_type;
        if (mDView->db.flag & DB_FLAG_NFT) {
            coin_type += COIN_TYPE_NFT_BASE;
        } else if ((!IS_VALID_COIN_TYPE(mDView->db.coin_type)) && (mDView->db.flag & DB_FLAG_UNIVERSAL_EVM)) {
            coin_type += COIN_TYPE_UNIVERSAL_EVM_BASE;
        }
		ret = get_coin_icon_path(coin_type, mDView->coin_uname, tmpbuf, sizeof(tmpbuf));
        db_msg("ret=%d, tmpbuf=%s,mDView->coin_type=%d",ret, tmpbuf,mDView->coin_type);
		res_unloadBmp(mBitmapLogo);
		if (tmpbuf[0]) {
			res_loadBmp(tmpbuf, mBitmapLogo);
			coin_icon->update(1, true, mBitmapLogo);
		} else {
			coin_icon->hide();
		}
	}

    //for saving history
    memcpy(&mDBTxCoinInfo, &mDView->db, sizeof(DBTxCoinInfo));

    //del mDView
    if (IS_VALID_HWND(mDView->hwnd)) {
        db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
        dwin_destory(mDView);
    }

    //RawData
    int head_len = GetExtHeaderLen(mClientMessage);
    int raw_len = mClientMessage->data->len - head_len;
    if (mClientMessage->p_total > 1) {
        raw_len -= QR_HASH_CHECK_LEN;
    }
    int m_len = raw_len * 2 + 8;
    db_msg("mClientMessage->data->len:%d, head_len:%d, raw_len:%d, mMsgFrom:%d", mClientMessage->data->len, head_len, raw_len, mMsgFrom);
    mRawDataStr = (char *) malloc(sizeof(char) * m_len);
    memzero(mRawDataStr, m_len);
    int len = format_data_to_hex((unsigned char *) (mClientMessage->data->str + head_len), raw_len, mRawDataStr, m_len);

    //page
    char segments[MAX_SEGMENTS][MAX_SEG_LEN];
    int count = splitString(mRawDataStr, MAX_SEG_LEN - 1, segments);
    if(count < 1 || count > MAX_SEGMENTS){
        dialog_error3(mHwnd, len, "Show raw data failed.");
        return -20;
    }
    db_msg("seg_len:%d, count:%d", MAX_SEG_LEN - 1, count);
    for (int i = 0; i < count; i++) {
        db_msg("seg:%d str:%s", i, segments[i]);
    }
    mTotalHeight = count * SCREEN_HEIGHT;
    initDView();
    int viewid = 100;
    dwin_add_txt_offset(mDView, MK_eth_label_raw_data_value, viewid++, (const char *) segments[0], 0);
    if (count > 1) {
        for (int i = 1; i < count; i++) {
            dwin_add_txt_offset(mDView, MK_eth_label_raw_data_value1, viewid++, (const char *) segments[i], (i - 1) * SCREEN_HEIGHT);
        }
    }

    if (mTotalHeight > mScreenHeight) {
        updateIcon(TXS_ICON_NAVI_UP_DOWN, 1, true);
    } else {
        updateIcon(TXS_ICON_NAVI_UP_DOWN, 0, false);
    }
    showIcon(TXS_ICON_NAVI_CANCEL, true);

    WIN_RECT rect;
	res_getPos(MK_sign_navi_panel, &rect); //default 2 screen
	int p = (mTotalHeight + mScreenHeight - 1) / mScreenHeight;
	rect.y += (p - 2) * mScreenHeight;
	MoveWindow(mHwndNaviPanel, rect.x, rect.y, rect.w, rect.h, FALSE);
	return 0;
}

int TxRawDataWin::onPause() {
    set_temp_screen_time(0);
    freeWinData();
    return 0;
}

int TxRawDataWin::onScrollWindow(int scroll_size) {
    WIN_RECT rect;
    HWND hwnd = getIconHwnd(TXS_ICON_NAVI_UP_DOWN);
    db_msg("current ScrollSize:%d TotalHeight:%d scroll_size:%d", mScrollSize, mTotalHeight, scroll_size);
    if (IS_VALID_HWND(hwnd)) {
        int state = 0;
        if (mScrollSize + mScreenHeight != mTotalHeight) {
            if (mScrollSize + mScreenHeight < mTotalHeight) {
                state |= 1;
            }
            if (mScrollSize > 0) {
                state |= 2;
            }
        }
        db_msg("state:%d", state);
        if (state) {
            res_getPos(MK_sign_icon_navi_up_down, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
        updateIcon(TXS_ICON_NAVI_UP_DOWN, state);
    }

    if (mTotalHeight > mScreenHeight) {
        hwnd = getIconHwnd(TXS_ICON_NAVI_CANCEL);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_icon_navi_cancel_1, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }

        hwnd = getLabelHwnd(TXS_LABEL_CANCEL);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_label_cancel_1, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
    }
    return 0;
}

int TxRawDataWin::initDView() {
	if (!IS_VALID_HWND(mDView->hwnd)) {
		dwin_init(mDView, mHwnd, 10);
		mDView->msg_from = mMsgFrom;
	}
	return 0;
}

int TxRawDataWin::freeWinData() {
	if (!mTxp->onShow) {
		return 0;
	}
	if (IS_VALID_HWND(mDView->hwnd)) {
		db_msg("tx type:%d coin:%s", mDView->coin_type, mDView->coin_uname);
		dwin_destory(mDView);
	}
	if (mTxp->onEnd) {
		db_msg("call tx end");
		mTxp->onEnd(mTxp->session);
	}
	memset(mTxp, 0, sizeof(TxPorcessData));
	return 0;
}

int TxRawDataWin::onSignReqData(ProtoClientMessage *req) {
	if (req) {
		db_msg("req:%p type:%d flag:%d", req, req->type, req->flag);
	} else {
		db_msg("empey req");
	}
    db_msg("mClientMessage:%p", mClientMessage);
    freeWinData();
	mShowRet = -1;
	mClientMessage = req;
	if (mClientMessage) {
		initDView();
	}
	return 0;
}

static int tx_save_history(const ProtoClientMessage *msg, DBTxCoinInfo *db) {
	DBTxInfo tx[1];
	tx->msg_type = msg->type;
	tx->time = msg->time;
	tx->time_zone = msg->time_zone;
	tx->client_id = msg->client_id;

	memcpy(&tx->flag, db, sizeof(DBTxCoinInfo)); //struct is same,hack copy

	tx->data = proto_client_message_serialize(msg);
	if (!tx->data) {
		db_error("serialize msg false");
		return -1;
	}
	int ret = storage_saveTxsInfo(tx);
	cstr_free(tx->data);
	if (ret != 0) {
		db_error("saveTxsInfo false");
		return -1;
	}
	return 0;
}

int TxRawDataWin::doSignReq() {
	db_msg("doSignReq");
	if (mShowRet != 0 || !mTxp->onSign) {
		db_error("invalid state");
		return -1;
	}
	unsigned char passhash[PASSWD_HASHED_LEN] = {0};
	int ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, 1);
	if (ret < 0) {
		db_error("input passwd ret:%d", ret);
		return 0;
	}
	db_secure("passhash:%s", debug_ubin_to_hex(passhash, sizeof(passhash)));
	ret = mTxp->onSign(mTxp->session, mHwnd, passhash);
	memzero(passhash, sizeof(passhash));
	if (ret == 0) {
		db_error("TX sign Success");
        if (mDBTxCoinInfo.coin_type > 0) {
            tx_save_history(mClientMessage, &mDBTxCoinInfo);
        }
		changeWindow(WINDOWID_MAINPANEL);
	} else {
		db_error("TX sign false,ret:%d", ret);
		if (ret < 0) {
			dialog_error3(mHwnd, ret, "Sign tx failed.");
		}
	}
	return ret;
}
