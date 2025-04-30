#define LOG_TAG "CoinsWin"

#include "CoinsWin.h"
#include "minigui_comm.h"
#include "GuiMain.h"
#include "debug.h"
#include "storage_manager.h"
#include "wallet_proto.h"
#include "wallet_util_hw.h"
#include "PicDialog.h"

#define IDC_LIST_PARENT_CONTAINER 650
#define SIGNH_ITEM_SIZE 10

#define FLAG_HIDE 0x1
#define CLEAR_FLAG(x, flag)        ((x) &~(flag))

CoinsWin::CoinsWin() {
	mViewOffet = 0;
	mViewTotal = 0;
	mItemTotal = 0;
	mEditMode = 0;
	mListView = NULL;
	mBitmapGroup = new BitmapGroup();
	memset(&mItems, 0, sizeof(mItems));
    GLobal_Is_Coin_EVM_Category = 0;
}

int CoinsWin::onCreate(HWND hWnd) {
	createLayoutWidgets(hWnd);
	mListView = new ListView(hWnd, MK_coins_list_container);
	return 0;
}

int CoinsWin::onResume() {
	int refresh = 0;
	int ret = 0;
	int m = GLobal_CoinsWin_EditMode ? 1 : 0;
	//set_support_long_key(1);
	if (!mItemTotal || Global_Have_New_DBCoin || (mEditMode != m)) {
		mEditMode = m;
		updateItemTotal();
		refresh = 1;
	}

	Global_Have_New_DBCoin = 0;
	if (mItemTotal > 0) {
		if (refresh) {
			ret = refreshItemList();
			if(ret == -1){
				showNotItemTips();
				ret = -100;
			}
		}
	} else {
		//tips not sign
		showNotItemTips();
		ret = -100;
	}
	
	if(ret == -100){
		if(mEditMode){
			changeWindow(WINDOWID_SETTING);
		}else{
			changeWindow(WINDOWID_MAINPANEL);
		}
	}

	return 0;
}

int CoinsWin::onPause() {
	//set_support_long_key(0);
	return 0;
}

CoinsWin::~CoinsWin() {
	if (mListView) {
		mListView->clean();
		delete mListView;
		mListView = NULL;
	}
}

int CoinsWin::updateItemTotal() {
	if (mEditMode) {
		mItemTotal = storage_getCoinsCount(0);
	} else {
		mItemTotal = storage_getCoinsCount(1);
	}
	db_msg("item total:%d", mItemTotal);
	return mItemTotal;
}

int CoinsWin::refreshItemList(int init_select) {
	const int icon_mk[] = {MK_coins_item_icon, MK_coins_item_navi};
	const int icon_mk1[] = {MK_coins_item_icon, MK_coins_item_switch};
	const int text_mk[] = {MK_coins_item_name};
	int i = 0, j = 0, cnt = 0, ret = 0;
	int show_switch = 0;

	if (mViewOffet < 0 || mViewOffet >= mItemTotal) mViewOffet = 0;
	if (mEditMode) {
		mViewTotal = storage_queryCoinInfo(&mItems[0], ITEM_BUFFER_SIZE, mViewOffet, 0);
	} else {
		mViewTotal = storage_queryCoinInfo(&mItems[0], ITEM_BUFFER_SIZE, mViewOffet, 1);
	}
	db_msg("view offset:%d total:%d txtotal:%d", mViewOffet, mViewTotal, mItemTotal);
	if (mViewTotal < 1) {
		if (mViewTotal < 0) {
			mViewTotal = 0;
		}
		mListView->clean();

		if (mItemTotal && mViewOffet) {
			mViewOffet = 0;
			updateItemTotal();
			if (mItemTotal > 0) {
				db_msg("reflush sign history list");
				refreshItemList();
			}
		}
		return -1;
	}
	db_msg("view total:%d", mViewTotal);

	BITMAP *navi_bitmap0 = NULL;
	BITMAP *navi_bitmap1 = NULL;
	BITMAP *navi_bitmap = NULL;
	if (mEditMode) {
		mListView->init(mViewTotal, ARRAY_SIZE(icon_mk1), icon_mk1, ARRAY_SIZE(text_mk), text_mk);
		navi_bitmap0 = mBitmapGroup->getBmp(MK_sign_history_switch, "icon0");
		navi_bitmap1 = mBitmapGroup->getBmp(MK_sign_history_switch, "icon1");
	} else {
		mListView->init(mViewTotal, ARRAY_SIZE(icon_mk), icon_mk, ARRAY_SIZE(text_mk), text_mk);
		navi_bitmap = mBitmapGroup->getBmp(MK_sign_history_navi, "icon0");
	}
	char str[64];
	DBCoinInfo *it;
    const CoinConfig *config = NULL;
    for (i = 0; i < mViewTotal; i++) {
		it = &mItems[i];
		memset(str, 0, sizeof(str));
        db_msg("it->type:%#x, it->flag:%#x", it->type, it->flag);
        if (IS_VALID_COIN_TYPE(it->type)) {
            get_coin_icon_path(it->type, it->uname, str, sizeof(str));
        } else {
            if (it->flag & DB_FLAG_UNIVERSAL_EVM) {
                ret = get_coin_icon_path(it->type + COIN_TYPE_UNIVERSAL_EVM_BASE, it->uname, str, sizeof(str));
                db_msg("ret:%d, str:%s", ret, str);
            }
        }

		// img
		mListView->setIconImage(i, 0, mBitmapGroup->getBmp(str));
		if (mEditMode) {
			if (it->flag & FLAG_HIDE) {
				mListView->setIconImage(i, 1, navi_bitmap0);
			} else {
				mListView->setIconImage(i, 1, navi_bitmap1);
			}
		} else {
			mListView->setIconImage(i, 1, navi_bitmap);
		}
        // label
        db_msg("it->uname:%s, it->name:%s, it->symbol:%s", it->uname, it->name, it->symbol);
        if (!strcmp(it->symbol, "USDC") || !strcmp(it->symbol, "USDT")) {
            if (it->type == COIN_TYPE_VICTION) {
                snprintf(it->name, sizeof(it->name), "%s%s", it->symbol, "(VRC20)");
            } else if (it->type == COIN_TYPE_VICTION_21) {
                snprintf(it->name, sizeof(it->name), "%s%s", it->symbol, "(VRC21)");
            } else if (it->type == COIN_TYPE_VICTION_25) {
                snprintf(it->name, sizeof(it->name), "%s%s", it->symbol, "(VRC25)");
            }
            db_msg("it->name:%s", it->name);
        }
        if (strlen(it->name) < 14) {
            config = getCoinConfig(it->type, it->uname);
            if (config) {
                if (strcmp(config->name, it->name) != 0) {
                    strncpy(it->name, config->name, sizeof(it->name));
                }
            }
			mListView->setLabelText(i, 0, it->name);
		} else {
			mListView->setLabelText(i, 0, it->symbol);
		}
	}

	if (init_select < mViewTotal) {
		mListView->select(init_select);
	} else {
		mListView->select(0);
	}
	updateNaviText();
	return 0;
}

int CoinsWin::updateNaviText() {
	return 0;
}


int CoinsWin::showNotItemTips() {
	int ret = 0;
	int flag = 0;

	PicDialogConfig_t config;
	memset(&config, 0, sizeof(PicDialogConfig_t));
	config.name = "no_assert_tips";
	config.total = 1;
	config.initIndex = 0;
	config.buttonCount = 0;
	config.initBtnIndex = 0;
	config.flag = 0;
	config.dync_text = res_getLabel(LANG_LABEL_NO_ASSET);
	config.dync_label = "coinswin_noassert_label";

	do {
		ret = showPicDialog(mHwnd, &config);
		if (ret == KEY_EVENT_BACK || !ret) {
			flag = 1;
		}
	} while (!flag);

	return ret;
}

PROC_RET CoinsWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_INITDIALOG:
			break;
		default:
			break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int CoinsWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d isLongPress:%d", keyCode, isLongPress);
	int selectindex;
	switch (keyCode) {
		case INPUT_KEY_LEFT:
			if (mEditMode) {
				return WINDOWID_SETTING;
			} else {
				return WINDOWID_MAINPANEL;
			}
			break;
		case INPUT_KEY_OK:
		case INPUT_KEY_RIGHT: {
			if (!mEditMode && (keyCode == INPUT_KEY_RIGHT)) {
			} else {
				onClickItem();
			}
		}
			break;
		case INPUT_KEY_DOWN: {
			if (isLongPress) {
				if (mViewOffet + mViewTotal < mItemTotal) {
					selectindex = mItemTotal % ITEM_BUFFER_SIZE;
					if (selectindex) {
						mViewOffet = (mItemTotal / ITEM_BUFFER_SIZE) * ITEM_BUFFER_SIZE;
					} else {
						mViewOffet = mItemTotal - ITEM_BUFFER_SIZE;
						if (mViewOffet < 0) mViewOffet = 0;
					}
					refreshItemList(-1);
				} else {
					mListView->select(-1);
					updateNaviText();
				}
				break;
			}
			selectindex = mListView->getSelectIndex();
			db_msg("selectindex:%d view total:%d", selectindex, mViewTotal);
			if ((selectindex + 1) == mViewTotal && mViewTotal > 0) {
				db_msg("select index:%d0 mViewOffet:%d mViewTotal:%d mItemTotal:%d try page down", selectindex, mViewOffet, mViewTotal, mItemTotal);
				if ((mViewOffet + mViewTotal) < mItemTotal) {
					mViewOffet += mViewTotal;
					refreshItemList();
				} else if ((mViewOffet + mViewTotal) == mItemTotal) {//circle
					mViewOffet = 0;
					refreshItemList();
				}
			} else {
				db_msg("select index:%d0 mViewOffet:%d mViewTotal:%d mItemTotal:%d try page down", selectindex, mViewOffet, mViewTotal, mItemTotal);
				mListView->move(1);
				updateNaviText();
			}
			break;
		}
		case INPUT_KEY_UP: {
			if (isLongPress) {
				if (mViewOffet > 0) {
					mViewOffet = 0;
					refreshItemList(0);
				} else {
					mListView->select(0);
					updateNaviText();
				}
				break;
			}
			selectindex = mListView->getSelectIndex();
			if (selectindex == 0) {
				if (mViewOffet > 0) {
					db_msg("select index0 offset:%d,try page up", mViewOffet);
					mViewOffet -= ITEM_BUFFER_SIZE;
					if (mViewOffet < 0) mViewOffet = 0;
					refreshItemList(-1);
				} else if (mViewOffet == 0) {//circle
					selectindex = mItemTotal % ITEM_BUFFER_SIZE;
					if (selectindex) {
						mViewOffet = (mItemTotal / ITEM_BUFFER_SIZE) * ITEM_BUFFER_SIZE;
					} else {
						mViewOffet = mItemTotal - ITEM_BUFFER_SIZE;
						if (mViewOffet < 0) mViewOffet = 0;
					}
					refreshItemList(-1);
				}
			} else {
				mListView->move(-1);
				updateNaviText();
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

int CoinsWin::onClickItem() {
	if (!mViewTotal) {
		db_error("invalid total:%d", mViewTotal);
		return -1;
	}
	int selectindex = mListView->getSelectIndex();
	if (selectindex < 0 || selectindex >= ITEM_BUFFER_SIZE) {
		db_error("invalid index:%d", selectindex);
		return -1;
	}

	if (mEditMode) {
		BITMAP *navi_bitmap;
		if (mItems[selectindex].flag & FLAG_HIDE) { //hide
			navi_bitmap = mBitmapGroup->getBmp(MK_sign_history_switch, "icon1");
			mItems[selectindex].flag = CLEAR_FLAG(mItems[selectindex].flag, FLAG_HIDE);
		} else {
			navi_bitmap = mBitmapGroup->getBmp(MK_sign_history_switch, "icon0");
			mItems[selectindex].flag |= FLAG_HIDE;
		}
		mListView->setIconImage(selectindex, 1, navi_bitmap);

		storage_set_coin_flag(mItems[selectindex].type, mItems[selectindex].uname, mItems[selectindex].flag);
	} else {
        DBCoinInfo *it = &mItems[selectindex];
        if ((!IS_VALID_COIN_TYPE(it->type)) && (it->flag & DB_FLAG_UNIVERSAL_EVM)) {
            GLobal_Is_Coin_EVM_Category = 1;
        } else {
            GLobal_Is_Coin_EVM_Category = 0;
        }
        db_msg("item type:%#x uname:%s, global_category:%d", it->type, it->uname, GLobal_Is_Coin_EVM_Category);
        int winid = WINDOWID_COIN_DETAIL;
        GuiMain::getInstance()->sendMessage(winid, MSG_SET_COIN_INFO, it->type, (LPARAM) it->uname);
        changeWindow(winid);
	}
	return 0;
}
