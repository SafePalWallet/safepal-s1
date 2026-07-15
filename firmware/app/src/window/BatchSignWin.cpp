#define LOG_TAG "BatchSign"

#include <stdlib.h>
#include <string.h>
#include <CommonWindow.h>
#include <widgets.h>
#include "GuiMain.h"
#include "BatchSignWin.h"
#include "ListView.h"
#include "debug.h"
#include "wallet_util_hw.h"
#include "coin_util_hw.h"
#include "device.h"
#include "storage_manager.h"
#include "wallet_manager.h"
#include "common_util.h"
#include "qr_pack.h"
#include "protobuf_util.h"
#include "sha2.h"
#include "settings.h"
#include "coin_adapter_hw.h"
#include "tx_common.h"
#include "loading_win.h"

#define IDC_LIST_CONTAINER 100

enum {
    BS_ICON_COIN_TYPE = 0,      // detail: coin logo (top-left)
    BS_ICON_NAVI_UP_DOWN,       // detail: scroll indicator
    BS_ICON_NAVI_CANCEL,        // list/select: "<" Cancel (navi panel)
    BS_ICON_NAVI_OK,            // all: [OK] (navi panel)
    BS_ICON_NAVI_DETAILS,       // list/select: ">" Details (navi panel)
    BS_ICON_NAVI_CANCEL_1,      // detail: "<" Back (main window, bottom)
    BS_ICON_NAVI_RAW,           // detail: ">" Raw (main window, bottom)
    BS_ICON_MAXID,
};

enum {
    BS_LABEL_VERIFY_CODE_TIP = 0,
    BS_LABEL_VERIFY_CODE,
    BS_LABEL_COIN_SYMBOL,       // detail: coin symbol
    BS_LABEL_COIN_NAME,         // detail: coin name
    BS_LABEL_CANCEL,            // list/select: "Cancel" (navi panel)
    BS_LABEL_DETAILS,           // list/select: "Details" (navi panel)
    BS_LABEL_CANCEL_1,          // detail: "Back" (main window, bottom)
    BS_LABEL_RAW,               // detail: "Raw" (main window, bottom)
    BS_LABEL_MAXID,
};

BatchSignWin::BatchSignWin() {
    int icon_mk_map[BS_ICON_MAXID] = {
        MK_sign_icon_coin_type,
        MK_sign_icon_navi_up_down,
        MK_sign_icon_navi_cancel,
        MK_sign_icon_navi_ok,
        MK_sign_icon_navi_more,
        MK_sign_icon_navi_cancel_1,
        MK_sign_icon_navi_raw,
    };

    int label_mk_map[BS_LABEL_MAXID] = {
        MK_batch_sign_verify_code_tip,
        MK_batch_sign_verify_code_value,
        MK_sign_label_coin_symbol,
        MK_sign_label_coin_name,
        MK_sign_label_cancel,
        MK_sign_label_more,
        MK_sign_label_cancel_1,
        MK_sign_label_raw,
    };

    memset(mDView, 0, sizeof(DynamicViewCtx));
    memset(mTxp, 0, sizeof(TxPorcessData));
    memset(mTxEntries, 0, sizeof(mTxEntries));

    mState = STATE_LIST;
    mTxCount = 0;
    mHwndNaviPanel = HWND_INVALID;
    mListContainer = HWND_INVALID;
    mListView = NULL;
    memset(&mItemBgNormal, 0, sizeof(BITMAP));
    memset(&mItemBgSelected, 0, sizeof(BITMAP));
    mBatchMessage = NULL;
    mShowRet = -1;
    mVerifyCode = 0;
    memset(mVerifyCodeStr, 0, sizeof(mVerifyCodeStr));
    mBitmapLogo = (BITMAP *) calloc(1, sizeof(BITMAP));
    initLayout(BS_ICON_MAXID, icon_mk_map, BS_LABEL_MAXID, label_mk_map);
}

BatchSignWin::~BatchSignWin() {
    if (mListView) {
        mListView->clean();
        delete mListView;
        mListView = NULL;
    }
    if (IS_VALID_HWND(mListContainer)) {
        DestroyWindow(mListContainer);
        mListContainer = HWND_INVALID;
    }
    if (mBitmapLogo) {
        res_unloadBmp(mBitmapLogo);
        free(mBitmapLogo);
    }
    UnloadBitmap(&mItemBgNormal);
    UnloadBitmap(&mItemBgSelected);
}

PROC_RET BatchSignWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case MSG_QR_RESULT:
            onBatchSignReqData((ProtoClientMessage *) lParam);
            break;
        default:
            break;
    }
    return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int BatchSignWin::onBatchSignReqData(ProtoClientMessage *req) {
    if (req) {
        db_msg("req:%p type:%d flag:%d", req, req->type, req->flag);
    }
    freeAllTx();
    if (mBatchMessage) {
        proto_client_message_delete(mBatchMessage);
        mBatchMessage = NULL;
    }
    mBatchMessage = req;
    return 0;
}

int BatchSignWin::getIconState(int id) {
    switch (id) {
        case BS_ICON_NAVI_CANCEL:
        case BS_ICON_NAVI_OK:
        case BS_ICON_NAVI_DETAILS:
            return 0;
    }
    return -1;
}

int BatchSignWin::onCreate(HWND hWnd) {
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
        db_error("create navi panel failed");
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

    res_getPos(MK_batch_sign_list_container, &rect);
    mListContainer = createWidgetWindow(hWnd, 0, rect.x, rect.y, rect.w, rect.h,
                                        IDC_LIST_CONTAINER, WIDGET_TYPE_NORMAL, 0, -1);
    if (IS_VALID_HWND(mListContainer)) {
        SetWindowBkColor(mListContainer, res_getBGColor());
        mListView = new ListView(mListContainer, MK_batch_sign_item_highlight);
    }

    const char *path;
    path = res_getString2(MK_action_sheet_dialog, "item_bg_img");
    if (path) LoadBitmapFromFile(HDC_SCREEN, &mItemBgNormal, path);
    path = res_getString2(MK_action_sheet_dialog, "item_sel_bg_img");
    if (path) LoadBitmapFromFile(HDC_SCREEN, &mItemBgSelected, path);

    return 0;
}

void BatchSignWin::moveNaviPanel() {
    WIN_RECT rect;
    res_getPos(MK_sign_navi_panel, &rect);
    int p = (mTotalHeight + mScreenHeight - 1) / mScreenHeight;
    rect.y += (p - 2) * mScreenHeight;
    MoveWindow(mHwndNaviPanel, rect.x, rect.y, rect.w, rect.h, FALSE);
}

int BatchSignWin::parseBatchMessage() {
    BatchSignParsedItem parsed[BATCH_MAX_TX_COUNT];
    int count = proto_parse_batch_sign_request(mBatchMessage, parsed, BATCH_MAX_TX_COUNT);
    if (count < 0) {
        db_error("proto_parse_batch_sign_request failed");
        return -1;
    }

    for (int i = 0; i < count; i++) {
        mTxEntries[i].msg = parsed[i].msg;
        strlcpy(mTxEntries[i].name, parsed[i].name, BATCH_TX_NAME_MAX_LEN);
    }
    mTxCount = count;
    db_msg("parsed %d batch items", mTxCount);
    return 0;
}

void BatchSignWin::restoreListView() {
    mTotalHeight = mScreenHeight;

    // Hide detail-only controls (coin logo/symbol/name, scroll indicator and
    // the detail Back/Raw buttons) when returning to the list/select view.
    showIcon(BS_ICON_COIN_TYPE, false);
    showIcon(BS_ICON_NAVI_UP_DOWN, false);
    showIcon(BS_ICON_NAVI_CANCEL_1, false);
    showIcon(BS_ICON_NAVI_RAW, false);
    setLabelText(BS_LABEL_COIN_SYMBOL, "", 0);
    setLabelText(BS_LABEL_COIN_NAME, "", 0);
    setLabelText(BS_LABEL_CANCEL_1, "", 0);
    setLabelText(BS_LABEL_RAW, "", 0);

    setLabelText(BS_LABEL_VERIFY_CODE_TIP, res_getLabel(LANG_LABEL_TX_VERIFY_CODE), 1);
    setLabelText(BS_LABEL_VERIFY_CODE, mVerifyCodeStr, 1);

    showIcon(BS_ICON_NAVI_CANCEL, true);
    setLabelText(BS_LABEL_CANCEL, res_getLabel(LANG_LABEL_TXS_CANCEL), 1);
    showIcon(BS_ICON_NAVI_OK, true);
    showIcon(BS_ICON_NAVI_DETAILS, true);
    setLabelText(BS_LABEL_DETAILS, res_getLabel(LANG_LABEL_TX_SHOW_DETAILS), 1);

    if (IS_VALID_HWND(mListContainer)) {
        ShowWindow(mListContainer, SW_SHOWNORMAL);
    }
    initListView();
    moveNaviPanel();
    InvalidateRect(mHwnd, NULL, TRUE);
}

// Max chars of an item name shown in the list before it is abbreviated with
// "..." (via omit_tail_string). Fuzzy by char count -- the batch_sign_item_name
// label is 210px wide at the 20px list font (~18-20 Latin chars), and
// CTRL_STATIC has no pixel-accurate ellipsis, so exact width measurement isn't
// worth it here.
#define BATCH_NAME_DISPLAY_MAX 18

int BatchSignWin::initListView() {
    if (!mListView) return -1;

    mListView->clean();

    int icon_mk[] = {MK_batch_sign_item_bg};
    int label_mk[] = {MK_batch_sign_item_name};
    mListView->init(mTxCount, 1, icon_mk, 1, label_mk);

    for (int i = 0; i < mTxCount; i++) {
        mListView->setIconImage(i, 0, &mItemBgNormal);
        char shown[BATCH_TX_NAME_MAX_LEN];
        omit_tail_string(shown, mTxEntries[i].name, BATCH_NAME_DISPLAY_MAX);
        mListView->setLabelText(i, 0, shown);
    }

    return 0;
}

int BatchSignWin::showDetailView(int index) {
    if (index < 0 || index >= mTxCount || !mTxEntries[index].msg) {
        db_error("invalid detail index:%d", index);
        return -1;
    }

    freeDetailView();

    if (IS_VALID_HWND(mListContainer)) {
        ShowWindow(mListContainer, SW_HIDE);
    }

    int ret = tx_process_client_message(mTxEntries[index].msg, mTxp);
    if (ret != 0) {
        db_error("tx_process item %d ret:%d", index, ret);
        return ret;
    }

    if (!mTxp->onInit || !mTxp->onShow) {
        db_error("item %d no init/show func", index);
        return -1;
    }

    ret = mTxp->onInit(mTxp->session);
    if (ret != 0) {
        db_error("item %d init ret:%d", index, ret);
        return ret;
    }

    setLabelText(BS_LABEL_VERIFY_CODE_TIP, "", 0);
    setLabelText(BS_LABEL_VERIFY_CODE, "", 0);

    dwin_init(mDView, mHwnd, 10);
    mDView->msg_from = MSG_FROM_QR_APP;

    mShowRet = mTxp->onShow(mTxp->session, mDView);
    if (mShowRet < 0) {
        db_error("item %d show ret:%d", index, mShowRet);
        return mShowRet;
    }

    mTotalHeight = mDView->total_height > mScreenHeight ? mDView->total_height : mScreenHeight;

    // Switch navigation from the list buttons (Cancel/OK/Details in the navi
    // panel) to the detail buttons (Back/OK/Raw), matching TxShowWin's layout.
    showIcon(BS_ICON_NAVI_CANCEL, false);
    setLabelText(BS_LABEL_CANCEL, "", 0);
    showIcon(BS_ICON_NAVI_DETAILS, false);
    setLabelText(BS_LABEL_DETAILS, "", 0);

    showIcon(BS_ICON_NAVI_OK, true);
    // cancel_1/raw default to getIconState()==-1 (invalid image), so showIcon()
    // alone would no-op. updateIcon(state=0) loads icon0 (the arrow) and shows.
    updateIcon(BS_ICON_NAVI_CANCEL_1, 0, true);
    setLabelText(BS_LABEL_CANCEL_1, res_getLabel(LANG_LABEL_BACK), 1);
    updateIcon(BS_ICON_NAVI_RAW, 0, true);
    setLabelText(BS_LABEL_RAW, "Raw", 1);

    // Coin symbol + name (font sized by length), same as TxShowWin::onResume.
    char tmpbuf[128];
    if (is_not_empty_string(mDView->coin_symbol)) {
        int font = 1;
        if (mDView->flag & 0x1) { // small
            font = 8;
        }
        SetWindowFont(getLabelHwnd(BS_LABEL_COIN_SYMBOL), res_getFont(font));
        setLabelText(BS_LABEL_COIN_SYMBOL, mDView->coin_symbol, 1);
    } else {
        setLabelText(BS_LABEL_COIN_SYMBOL, "", 0);
    }
    if (is_not_empty_string(mDView->coin_name)) {
        if (strlen(mDView->coin_name) < 18) {
            snprintf(tmpbuf, sizeof(tmpbuf), "   %s", mDView->coin_name);
            setLabelText(BS_LABEL_COIN_NAME, tmpbuf, 1);
        } else {
            setLabelText(BS_LABEL_COIN_NAME, mDView->coin_name, 1);
        }
    } else {
        setLabelText(BS_LABEL_COIN_NAME, "", 0);
    }

    // Up/down scroll indicator (shown only when content exceeds one screen).
    if (mTotalHeight > mScreenHeight) {
        updateIcon(BS_ICON_NAVI_UP_DOWN, 1, true);
    } else {
        updateIcon(BS_ICON_NAVI_UP_DOWN, 0, false);
    }

    // Coin logo (top-left), same resolution logic as TxShowWin::onResume.
    PicObj *coin_icon = mIcons[BS_ICON_COIN_TYPE];
    if (coin_icon && mBitmapLogo) {
        tmpbuf[0] = 0;
        int coin_type = mDView->coin_type;
        if (mDView->db.flag & DB_FLAG_NFT) {
            coin_type += COIN_TYPE_NFT_BASE;
        } else if ((!IS_VALID_COIN_TYPE(mDView->db.coin_type)) && (mDView->db.flag & DB_FLAG_UNIVERSAL_EVM)) {
            coin_type += COIN_TYPE_UNIVERSAL_EVM_BASE;
        }
        get_coin_icon_path(coin_type, mDView->coin_uname, tmpbuf, sizeof(tmpbuf));
        res_unloadBmp(mBitmapLogo);
        if (tmpbuf[0]) {
            res_loadBmp(tmpbuf, mBitmapLogo);
            coin_icon->update(1, true, mBitmapLogo);
        } else {
            coin_icon->hide();
        }
    }

    moveNaviPanel();

    return 0;
}

void BatchSignWin::freeDetailView() {
    if (IS_VALID_HWND(mDView->hwnd)) {
        dwin_destory(mDView);
    }
    if (mTxp->onEnd) {
        mTxp->onEnd(mTxp->session);
    }
    memset(mTxp, 0, sizeof(TxPorcessData));
    if (mScrollSize) {
        resetScrollSize();
    }
}

int BatchSignWin::onResume() {
    db_msg("resume state:%d", mState);
    set_temp_screen_time(180);

    if (!mBatchMessage) {
        db_error("no batch message");
        return -1;
    }

    if (mState == STATE_DETAIL) {
        db_msg("returning from raw data view");
        return 0;
    }

    freeDetailView();
    mState = STATE_LIST;

    if (mTxCount == 0) {
        int ret = parseBatchMessage();
        if (ret != 0) {
            db_error("parse batch failed ret:%d", ret);
            dialog_error3(mHwnd, ret, "Invalid batch sign request.");
            PostMessage(GuiMain::getInstance()->getHwnd(), MSG_CHANGE_WINDOW, WINDOWID_MAINPANEL, 0);
            return ret;
        }
    }

    mVerifyCode = TxGetVerifyCode(mBatchMessage);
    if (mVerifyCode < 0) {
        db_error("TxGetVerifyCode error ret:%d", mVerifyCode);
        dialog_error3(mHwnd, mVerifyCode, "Failed to generate verification code.");
        PostMessage(GuiMain::getInstance()->getHwnd(), MSG_CHANGE_WINDOW, WINDOWID_MAINPANEL, 0);
        return mVerifyCode;
    }

    memset(mVerifyCodeStr, 0, sizeof(mVerifyCodeStr));
    for (int i = 0; i < 6; i++) {
        mVerifyCodeStr[5 - i] = mVerifyCode % 10 + 0x30;
        mVerifyCode /= 10;
    }
    restoreListView();

    return 0;
}

int BatchSignWin::onPause() {
    set_temp_screen_time(0);
    if (mState == STATE_DETAIL) {
        return 0;
    }
    gBatchSignCollector.active = 0;
    // Defensive reset: clear the loading hold in case the batch window leaves
    // the screen abnormally mid-signing, so future loading_win_stop() works.
    loading_win_set_hold(0);
    if (gBatchSignCollector.data) {
        free(gBatchSignCollector.data);
        gBatchSignCollector.data = NULL;
    }
    if (mListView) {
        mListView->clean();
    }
    freeAllTx();
    if (mBatchMessage) {
        proto_client_message_delete(mBatchMessage);
        mBatchMessage = NULL;
    }
    return 0;
}

int BatchSignWin::onScrollWindow(int scroll_size) {
    // Keep the detail-view scroll indicator and the bottom Back/Raw buttons
    // pinned to the screen edge as content scrolls, matching TxShowWin.
    WIN_RECT rect;
    HWND hwnd = getIconHwnd(BS_ICON_NAVI_UP_DOWN);
    if (IS_VALID_HWND(hwnd)) {
        int state = 0;
        if (mScrollSize + mScreenHeight != mTotalHeight) {
            if (mScrollSize + mScreenHeight < mTotalHeight) {
                state |= 1; // can scroll down
            }
            if (mScrollSize > 0) {
                state |= 2; // can scroll up
            }
        }
        if (state) {
            res_getPos(MK_sign_icon_navi_up_down, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
        updateIcon(BS_ICON_NAVI_UP_DOWN, state);
    }
    if (mTotalHeight > mScreenHeight) {
        hwnd = getIconHwnd(BS_ICON_NAVI_CANCEL_1);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_icon_navi_cancel_1, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
        hwnd = getLabelHwnd(BS_LABEL_CANCEL_1);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_label_cancel_1, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
        hwnd = getIconHwnd(BS_ICON_NAVI_RAW);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_icon_navi_raw, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
        hwnd = getLabelHwnd(BS_LABEL_RAW);
        if (IS_VALID_HWND(hwnd)) {
            res_getPos(MK_sign_label_raw, &rect);
            MoveWindow(hwnd, rect.x, rect.y, rect.w, rect.h, FALSE);
        }
    }
    return 0;
}

int BatchSignWin::doSignAll() {
    freeDetailView();

    unsigned char passhash[PASSWD_HASHED_LEN] = {0};
    int ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD),
                             PIN_CODE_VERITY, passhash, 1);
    if (ret < 0) {
        memzero(passhash, sizeof(passhash));
        mState = STATE_LIST;
        restoreListView();
        return 0;
    }

    gBatchSignCollector.active = 1;
    // Show a single "Signing" window for the whole batch and hold it, so each
    // item's onSign start/stop does not flicker the screen (see loading_win).
    loading_win_start(mHwnd, res_getLabel(LANG_LABEL_TX_SIGNING), NULL, 0);
    loading_win_set_hold(1);

    for (int i = 0; i < mTxCount; i++) {
        ret = tx_process_client_message(mTxEntries[i].msg, mTxp);
        if (ret != 0) {
            db_error("process item %d ret:%d", i, ret);
            goto sign_fail;
        }

        ret = mTxp->onInit(mTxp->session);
        if (ret != 0) {
            db_error("init item %d ret:%d", i, ret);
            goto sign_fail;
        }

        dwin_init(mDView, mHwnd, 10);
        mDView->msg_from = MSG_FROM_QR_APP;
        int showRet = mTxp->onShow(mTxp->session, mDView);
        memcpy(&mTxEntries[i].db, &mDView->db, sizeof(DBTxCoinInfo));
        dwin_destory(mDView);

        if (showRet < 0) {
            db_error("show item %d ret:%d", i, showRet);
            ret = showRet;
            goto sign_fail;
        }

        ret = mTxp->onSign(mTxp->session, mHwnd, passhash);
        if (ret != 0) {
            db_error("sign item %d ret:%d", i, ret);
            goto sign_fail;
        }

        mTxEntries[i].sign_result = gBatchSignCollector.data;
        mTxEntries[i].sign_result_size = gBatchSignCollector.size;
        mTxEntries[i].sign_msg_type = gBatchSignCollector.msg_type;
        mTxEntries[i].sign_flag = gBatchSignCollector.flag;
        mTxEntries[i].sign_client_id = gBatchSignCollector.client_id;
        mTxEntries[i].signed_ok = 1;
        gBatchSignCollector.data = NULL;

        if (mTxEntries[i].db.coin_type > 0) {
            tx_save_history(mTxEntries[i].msg, &mTxEntries[i].db);
        }

        if (mTxp->onEnd) mTxp->onEnd(mTxp->session);
        memset(mTxp, 0, sizeof(TxPorcessData));
    }

    gBatchSignCollector.active = 0;
    loading_win_set_hold(0);
    loading_win_stop();
    memzero(passhash, sizeof(passhash));

    showBatchResult();
    changeWindow(WINDOWID_MAINPANEL);
    return 0;

sign_fail:
    gBatchSignCollector.active = 0;
    loading_win_set_hold(0);
    loading_win_stop();
    if (gBatchSignCollector.data) {
        free(gBatchSignCollector.data);
        gBatchSignCollector.data = NULL;
    }
    memzero(passhash, sizeof(passhash));
    if (IS_VALID_HWND(mDView->hwnd)) {
        dwin_destory(mDView);
    }
    if (mTxp->onEnd) mTxp->onEnd(mTxp->session);
    memset(mTxp, 0, sizeof(TxPorcessData));
    if (ret < 0) dialog_error3(mHwnd, ret, "Sign tx failed.");
    changeWindow(WINDOWID_MAINPANEL);
    return ret;
}

int BatchSignWin::showBatchResult() {
    BatchSignRespEntry entries[BATCH_MAX_TX_COUNT];
    int resp_count = 0;
    for (int i = 0; i < mTxCount; i++) {
        if (!mTxEntries[i].signed_ok) continue;
        entries[resp_count].msg_type = mTxEntries[i].sign_msg_type;
        entries[resp_count].data = mTxEntries[i].sign_result;
        entries[resp_count].size = mTxEntries[i].sign_result_size;
        resp_count++;
    }

    struct pbc_wmessage *resp = NULL;
    int ret = proto_build_batch_sign_response(&resp, entries, resp_count);
    if (ret != 0 || !resp) {
        db_error("proto_build_batch_sign_response failed");
        return -1;
    }

    struct pbc_slice resp_slice;
    pbc_wmessage_buffer(resp, &resp_slice);

    struct pbc_wmessage *wmsg_wrapper = proto_new_wmessage("Wallet.PacketRespHeaderWrapper");
    struct pbc_wmessage *ext_header = pbc_wmessage_message(wmsg_wrapper, "header");
    pbc_wmessage_integer(ext_header, "version", (uint32_t) DEVICE_APP_INT_VERSION, 0);

    struct pbc_slice slice_ext_head;
    pbc_wmessage_buffer(wmsg_wrapper, &slice_ext_head);

    unsigned char *merge_buff = (unsigned char *) malloc(resp_slice.len + slice_ext_head.len);
    if (!merge_buff) {
        db_error("malloc merge_buff failed");
        proto_delete_wmessage(resp);
        proto_delete_wmessage(wmsg_wrapper);
        return -1;
    }

    memcpy(merge_buff, slice_ext_head.buffer, slice_ext_head.len);
    memcpy(merge_buff + slice_ext_head.len, resp_slice.buffer, resp_slice.len);

    uint16_t flag = mBatchMessage->flag | QR_FLAG_EXT_HEADER;
    ret = showQRWindow(mHwnd, mBatchMessage->client_id, flag, QR_MSG_BATCH_SIGN_RESP, merge_buff, (int)(resp_slice.len + slice_ext_head.len));

    free(merge_buff);
    proto_delete_wmessage(resp);
    proto_delete_wmessage(wmsg_wrapper);

    if (ret < 0) {
        db_error("showQRWindow batch result ret:%d", ret);
    }
    return ret;
}

void BatchSignWin::freeAllTx() {
    for (int i = 0; i < mTxCount; i++) {
        if (mTxEntries[i].msg) {
            proto_client_message_delete(mTxEntries[i].msg);
            mTxEntries[i].msg = NULL;
        }
        if (mTxEntries[i].sign_result) {
            free(mTxEntries[i].sign_result);
            mTxEntries[i].sign_result = NULL;
        }
    }
    memset(mTxEntries, 0, sizeof(mTxEntries));
    mTxCount = 0;

    freeDetailView();
}

void BatchSignWin::selectItem(int newIndex) {
    if (!mListView) return;
    int oldIndex = mListView->getSelectIndex();
    if (oldIndex >= 0) {
        mListView->setIconImage(oldIndex, 0, &mItemBgNormal);
    }
    mListView->select(newIndex);
    if (newIndex >= 0) {
        mListView->setIconImage(newIndex, 0, &mItemBgSelected);
    }
}

void BatchSignWin::moveItem(int dir) {
    if (!mListView) return;
    int oldIndex = mListView->getSelectIndex();
    if (oldIndex >= 0) {
        mListView->setIconImage(oldIndex, 0, &mItemBgNormal);
    }
    mListView->move(dir);
    int newIndex = mListView->getSelectIndex();
    if (newIndex >= 0) {
        mListView->setIconImage(newIndex, 0, &mItemBgSelected);
    }
}

int BatchSignWin::keyProc(int keyCode, int isLongPress) {
    switch (mState) {
        case STATE_LIST:
            switch (keyCode) {
                case INPUT_KEY_OK:
                    if (!gProcessing) {
                        gProcessing = 1;
                        mState = STATE_SIGNING;
                        doSignAll();
                        gProcessing = 0;
                    }
                    break;
                case INPUT_KEY_LEFT:
                    return WINDOWID_MAINPANEL;
                case INPUT_KEY_RIGHT:
                    mState = STATE_SELECT;
                    selectItem(0);
                    break;
                case INPUT_KEY_UP:
                case INPUT_KEY_DOWN:
                    break;
            }
            break;

        case STATE_SELECT:
            switch (keyCode) {
                case INPUT_KEY_UP:
                    moveItem(-1);
                    break;
                case INPUT_KEY_DOWN:
                    moveItem(1);
                    break;
                case INPUT_KEY_RIGHT: {
                    int idx = mListView ? mListView->getSelectIndex() : -1;
                    if (idx >= 0) {
                        mState = STATE_DETAIL;
                        int ret = showDetailView(idx);
                        if (ret != 0) {
                            dialog_error3(mHwnd, ret, "Show tx detail failed.");
                            mState = STATE_SELECT;
                            restoreListView();
                            selectItem(idx);
                        }
                    }
                    break;
                }
                case INPUT_KEY_OK:
                    if (!gProcessing) {
                        gProcessing = 1;
                        mState = STATE_SIGNING;
                        doSignAll();
                        gProcessing = 0;
                    }
                    break;
                case INPUT_KEY_LEFT:
                    mState = STATE_LIST;
                    initListView();
                    break;
            }
            break;

        case STATE_DETAIL:
            switch (keyCode) {
                case INPUT_KEY_LEFT: {
                    int prevIdx = mListView ? mListView->getSelectIndex() : 0;
                    freeDetailView();
                    mState = STATE_SELECT;
                    restoreListView();
                    selectItem(prevIdx);
                    break;
                }
                case INPUT_KEY_OK:
                    if (!gProcessing) {
                        gProcessing = 1;
                        mState = STATE_SIGNING;
                        doSignAll();
                        gProcessing = 0;
                    }
                    break;
                case INPUT_KEY_UP:
                    scrollWindow(-1);
                    break;
                case INPUT_KEY_DOWN:
                    scrollWindow(1);
                    break;
                case INPUT_KEY_RIGHT: {
                    int idx = mListView ? mListView->getSelectIndex() : -1;
                    if (idx >= 0 && idx < mTxCount) {
                        GuiMain::getInstance()->sendMessage(WINDOWID_TX_RAW_DATA,
                            MSG_HISTORY_QR_RESULT, WINDOWID_BATCH_SIGN, (LPARAM) mTxEntries[idx].msg);
                        return WINDOWID_TX_RAW_DATA;
                    }
                    break;
                }
            }
            break;

        case STATE_SIGNING:
            break;
    }
    return 0;
}
