#ifndef _WALLET_BATCH_SIGN_WIN_H
#define _WALLET_BATCH_SIGN_WIN_H

#include "CommonWindow.h"
#include "wallet_proto_qr.h"
#include "coin_adapter.h"
#include "dynamic_win.h"

class ListView;

typedef struct {
    ProtoClientMessage *msg;
    char name[BATCH_TX_NAME_MAX_LEN];
    int signed_ok;
    unsigned char *sign_result;
    int sign_result_size;
    uint16_t sign_msg_type;
    uint16_t sign_flag;
    int sign_client_id;
    DBTxCoinInfo db;
} BatchTxEntry;

class BatchSignWin : public CommonWindow {
public:
    BatchSignWin();
    ~BatchSignWin();

    PROC_RET winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);
    int keyProc(int keyCode, int isLongPress);

private:
    enum State {
        STATE_LIST = 0,
        STATE_SELECT,
        STATE_DETAIL,
        STATE_SIGNING,
    };

    int onCreate(HWND hWnd);
    int onResume();
    int onPause();
    int onScrollWindow(int scroll_size);
    int getIconState(int id);

    int onBatchSignReqData(ProtoClientMessage *req);
    int parseBatchMessage();
    int initListView();
    void selectItem(int index);
    void moveItem(int dir);
    int showDetailView(int index);
    int doSignAll();
    int showBatchResult();
    void restoreListView();
    void freeAllTx();
    void freeDetailView();
    void moveNaviPanel();

    State mState;
    int mTxCount;
    BatchTxEntry mTxEntries[BATCH_MAX_TX_COUNT];

    ListView *mListView;
    HWND mListContainer;
    BITMAP mItemBgNormal;
    BITMAP mItemBgSelected;

    TxPorcessData mTxp[1];
    DynamicViewCtx mDView[1];
    HWND mHwndNaviPanel;
    BITMAP *mBitmapLogo;
    int mShowRet;
    int mVerifyCode;
    char mVerifyCodeStr[8];

    ProtoClientMessage *mBatchMessage;
};

#endif
