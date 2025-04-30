#define LOG_TAG "Verify"

#include <stdlib.h>
#include "VerifyWin.h"
#include "debug.h"
#include "device.h"
#include "wallet_proto.h"
#include "storage_manager.h"
#include "key_event.h"
#include "Dialog.h"
#include "fileutil.h"
#include "secure_api.h"
#include "wallet_util.h"
#include <bip39.h>
#include <sha2.h>
#include "ActionSheetDialog.h"
#include "Stack.h"
#include <memzero.h>
#include "wallet_manager.h"
#include "PicDialog.h"
#include "loading_win.h"
#include "CommonQRShow.h"
#include "secp256k1.h"

static int gMmenCntVals[MNEMNONIC_CNT_LEVEL_MAX] = {12, 15, 18, 21, 24};
static const char *gMmenCntStr[MNEMNONIC_CNT_LEVEL_MAX] = {"12", "15", "18", "21", "24"};

#define DEFAULT_SCREEN_SAVER_TIME 60
#define DEFAULT_HI_SCREEN_SAVER_TIME 600
#define MAX_MNEMONIC_CNT  24
#define MAX_MNEMONIC_BUFFSIZE  32
#define SUPPORT_SELECT_MNEMONIC_CNT 3
#define IS_VALID_MNEMONIC_LEN(l) ( (l)>=12 && (l)<=24 && ( ((l)%3)==0 ))

typedef enum {
	OP_INDEX_VERIFY_TIPS = 1,
	OP_INDEX_PASSPHRASE_DETAIL_TIPS,
	OP_INDEX_ACTION_DETECTION,
	OP_INDEX_ENTER_PASSWD,
	OP_INDEX_SELECT_PASSPHRASE_TYPE,

	OP_INDEX_FIRST_USE_PASSPHRASE_TIPS,
	OP_INDEX_SELECT_MNEMONIC_CNT,
	OP_INDEX_VERIFY_MNEMONIC_TIPS,
	OP_INDEX_VERIFY_MNEMONIC,
	OP_INDEX_ENTER_PASSPHRASE,
	OP_INDEX_SAVE_PASSPHRASE,
	OP_INDEX_WALLET_NAME_TIPS,

	OP_INDEX_MAX
} guideIndex;

static int get_have_mnemonic() {
	sec_state_info info;
	info.mnemonic = 0;
	if (sapi_get_state_info(&info) != 0) {
		db_serr("get state info false");
		return -1;
	}
	if (info.seed_state != 1) {
		return -1;
	}
	return info.mnemonic;
}

VerifyWin::VerifyWin() {
	isResume = 0;
	mChangeWin = 0;
}

VerifyWin::~VerifyWin() {

}

PROC_RET VerifyWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int VerifyWin::onResume() {
	db_msg("onResume called");
	if (!mHwnd) {
		db_error("hMwd:%d is null", mHwnd);
		return -1;
	}
	isResume = 1;
	mChangeWin = 0;
	set_temp_screen_time(DEFAULT_SCREEN_SAVER_TIME);
	db_msg("guide start");
	guide();
	db_msg("guide end changed win:%d", mChangeWin);
	if (mChangeWin == 0) {
		changeWindow(WINDOWID_SETTING);
	} else if (mChangeWin > 0) {
		changeWindow(mChangeWin);
	}
	return 0;
}

int VerifyWin::onPause() {
	isResume = 0;
	set_temp_screen_time(0);
	return 0;
}

int VerifyWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d", keyCode);
	return 0;
}

int VerifyWin::selectMnemonicCnt() {
	int ret = 0;
	int count = -1;
	int supports[SUPPORT_SELECT_MNEMONIC_CNT] = {MNEMNONIC_CNT_LEVEL_24, MNEMNONIC_CNT_LEVEL_12, MNEMNONIC_CNT_LEVEL_18};
	const char *items[SUPPORT_SELECT_MNEMONIC_CNT];
	ActionSheetDialogConfig_t _config;
	ActionSheetDialogConfig_t *config = &_config;
	memset(config, 0, sizeof(ActionSheetDialogConfig_t));
	for (int i = 0; i < SUPPORT_SELECT_MNEMONIC_CNT; ++i) {
		items[i] = gMmenCntStr[supports[i]];
	}
	config->itemsCnt = SUPPORT_SELECT_MNEMONIC_CNT;
	config->items = items;
	config->title.data = res_getLabel(LANG_LABEL_MNEMONIC_CNT);
	config->initIndex = 0;

	ret = showActionSheetDialog(mHwnd, config);
	if (ret == KEY_EVENT_BACK) {
		return KEY_EVENT_BACK;
	}
	if (ret < 0) {
		db_error("select mnemonic action sheet false %d", ret);
		return ret;
	}
	if (ret >= SUPPORT_SELECT_MNEMONIC_CNT) {
		db_error("out of range for select mnemonic words");
		return -1;
	}
	count = gMmenCntVals[supports[ret]];
	db_msg("mnemonic cnt:%d ret:%d", count, ret);
	return count;
}

static int check_mnemonic_match(const unsigned char *passwd, const char *mnemonic, const char *passphrase) {
	uint8_t seed[512 / 8] = {0};
	mnemonic_to_seed(mnemonic, passphrase, seed, NULL);
	HDNode node[1];
	memset(node, 0, sizeof(HDNode));
	hdnode_gen_from_seed(seed, 512 / 8, &secp256k1_info, node);
	hdnode_private_ckd_prime(node, 44);
	hdnode_private_ckd_prime(node, 0);
	hdnode_private_ckd_prime(node, 0);
	hdnode_fill_public_key(node);
	db_secure("chaincode1:%s %s", passphrase, debug_ubin_to_hex(node->chain_code, 32));
	db_secure("public_key1:%s %s", passphrase, debug_ubin_to_hex(node->public_key, 33));

	PubHDNode node2;
	int ret = wallet_queryPubHDNode(CURVE_SECP256K1, "m/44h/0h/0h", passwd, &node2);
	if (ret != 0) {
		db_error("query BTC PubHDNode false");
		return ret;
	}
	if (memcmp(node->chain_code, node2.chain_code, 32) != 0) {
		db_error("ummatch chaincode:%s", debug_ubin_to_hex(node2.chain_code, 32));
		return 1;
	}
	if (memcmp(node->public_key, node2.public_key, 33) != 0) {
		db_error("ummatch public_key:%s", debug_ubin_to_hex(node2.public_key, 33));
		return 1;
	}
	return 0;
}

int VerifyWin::verifyMnemonic(const unsigned char *passwd, char *mnemonics, int size, int mlen) {
	if (NULL == mnemonics || size <= 0) {
		db_error("invalid paras mnemonics:%p, size:%d", mnemonics, size);
		return -1;
	}
	if (!IS_VALID_MNEMONIC_LEN(mlen)) {
		db_error("mnemonic word count:%d false", mlen);
		return -1;
	}
	db_secure("size:%d mlen:%d", size, mlen);
	int index = 0;
	int ret = 0;
	int err = 0;
	int checked_word = 0;
	char title[DIALOG_TITLE_MAX_LEN] = {0};
	char kb_result[MNEMONIC_MAX_LEN] = {0};
	char srcMnemonics[MNEMONIC_MAX_LEN * MAX_MNEMONIC_CNT] = {0};
	char passphrase_val[64] = {0};
	int sz = size;
	int offset = 0;
	int strLen = 0;
	int isBack = 0;
	KeyBoardConfig_t kb;
	memset(&kb, 0, sizeof(KeyBoardConfig_t));
	kb.result = kb_result;
	kb.result_limit = MNEMONIC_MAX_LEN - 1;
	kb.random = 0;
	kb.keyStyle = KEYBOARD_STYLE_ASSN;
	kb.editor_style = KEYBOARD_EDITOR_INPUT;
	index = 0;
	int lastKey = -1;
	memzero(mnemonics, sz);
	do {
		memcpy(kb_result, srcMnemonics + index * MNEMONIC_MAX_LEN, MNEMONIC_MAX_LEN);
		db_secure("index:%d str:%s", index, srcMnemonics + index * MNEMONIC_MAX_LEN);
		snprintf(title, sizeof(title), res_getLabel(LANG_LABEL_ENTER_MNEMONIC), index + 1);
		kb.title.data = title;
		kb.text_field.data = kb_result;
		db_secure("keyboard init result:%s", kb_result);
		if (strlen(kb_result)) {
			kb.initKeyIndex = (lastKey == KEY_EVENT_BACK) ? CHAR_KEYBOARD_BACK_KEY_INDEX : CHAR_KEYBOARD_OK_KEY_INDEX;
		} else if (lastKey == KEY_EVENT_BACK) {
			kb.initKeyIndex = CHAR_KEYBOARD_BACK_KEY_INDEX;
		} else {
			kb.initKeyIndex = get_random_keyboard_index(kb.keyStyle);
		}
		ret = showKeyBoard(mHwnd, &kb);
		lastKey = ret;
		if (is_key_event_value(ret)) {
			if (ret == KEY_EVENT_BACK) {
				if (index <= 0) {
					isBack = 1;
					break;
				}
				index--;
			} else if (ret == KEY_EVENT_ABORT) {
				//memcpy(srcMnemonics + index * MNEMONIC_MAX_LEN, kb_result, MNEMONIC_MAX_LEN);
			}
			continue;
		} else if (ret < 0) {
			err = 1;
			break;
		} else if (ret == 0) {
			continue;
		}
		db_secure("enter mnemonic:%s ret:%d", kb.result, ret);
		memzero(srcMnemonics + index * MNEMONIC_MAX_LEN, MNEMONIC_MAX_LEN);
		memcpy(srcMnemonics + index * MNEMONIC_MAX_LEN, kb_result, ret);
		if (index < (mlen - 1)) {
			index++;
		} else {
			memzero(mnemonics, sz);
			offset = 0;
			int i;
			for (i = 0; i < mlen; ++i) {
				strLen = strlen(srcMnemonics + i * MNEMONIC_MAX_LEN);
				if (!strLen) {
					break;
				}
				memcpy(mnemonics + offset, (srcMnemonics + i * MNEMONIC_MAX_LEN), strLen);
				offset += strLen;
				if (i < (mlen - 1)) {
					memcpy(mnemonics + offset, " ", 1);
					offset++;
				}
			}
			*(mnemonics + offset) = '\0';
			db_secure("mnemonics cat str:%s", mnemonics);
			if (i != mlen) {
				db_serr("have empty word? mlen:%d i:%d", mlen, i);
				err = 2;
				break;
			}
			ret = mnemonic_check(mnemonics);
			db_secure("mnemonic check ret:%d", ret);
			index = mlen - 1; //reset last
			if (!ret) {
				ret = picDialog(mHwnd, "verified_word_failed_alert", res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
				if (is_key_event_value(ret)) {
					//
				} else if (ret < 0) {
					err = 3;
					break;
				} else {
					lastKey = KEY_EVENT_BACK;
				}
			} else {
				memset(passphrase_val, 0, sizeof(passphrase_val));
				int verify_ret = check_mnemonic_match(passwd, mnemonics, "");
				db_secure("verify mnemonic ret:%d", verify_ret);
				if (verify_ret == 1) {
					ret = picDialog(mHwnd, "wallet_verify_has_ps", "YES", "NO", 0);
					if (ret == 0) {
						int pret = enterPassphrase(passphrase_val, sizeof(passphrase_val));
						if (pret == KEY_EVENT_BACK) {
							lastKey = KEY_EVENT_BACK;
							continue;
						}
						if (pret == 0) {
							verify_ret = check_mnemonic_match(passwd, mnemonics, passphrase_val);
							db_secure("verify mnemonic passphrase ret:%d", verify_ret);
						}
					} else if (ret == KEY_EVENT_BACK) {
						lastKey = KEY_EVENT_BACK;
						continue;
					}
				}
				if (verify_ret != 0) {
					if (verify_ret == 1) {
						ret = picDialog(mHwnd, passphrase_val[0] ? "wallet_verify_ps_fail" : "wallet_verify_fail", res_getLabel(LANG_LABEL_TRY_AGAIN), res_getLabel(LANG_LABEL_TX_EXIT), 0);
						if (ret == 1) {
							err = 88;
							break;
						}
					} else {
						dialog_error3(mHwnd, verify_ret, "Seed verify failed.");
					}
				} else {
					picDialogFlag(mHwnd, passphrase_val[0] ? "wallet_verify_ps_success" : "wallet_verify_success", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0, PIC_DLG_FLAG_LEFT_NOT_BACK);
					checked_word = 1;
					break;
				}
			}
		}
	} while (1);
	memzero(kb_result, sizeof(kb_result));
	memzero(srcMnemonics, sizeof(srcMnemonics));
	memzero(mnemonics, sz);
	memzero(passphrase_val, sizeof(passphrase_val));
	if (err) {
		return err;
	}
	if (isBack) {
		return KEY_EVENT_BACK;
	}
	if (!checked_word) {
		return -1;
	}
	return 0;
}

static int check_passphrase_valid(const char *result) {
	int len = strlen(result);
	if (len < 1) {
		return -1;
	}
	if (len < 1 || len > 60) {
		return 1;
	}
	if (*result == ' ') {
		return 2;
	}
	if (*(result + len - 1) == ' ') {
		return 2;
	}
	int bn = 0;
	while (*result) {
		if (*result == ' ') {
			bn++;
			if (bn >= 2) return 3;
		} else {
			bn = 0;
		}
		result++;
	}
	return 0;
}

int VerifyWin::enterPassphrase(char *passphrase, int size) {
	KeyBoardConfig_t kb;
	char result[64] = {0};
	int result_len = 0;
	int ret = 0;
	int done = 0;
	do {
		memset(&kb, 0, sizeof(KeyBoardConfig_t));
		kb.keyStyle = KEYBOARD_STYLE_FULL;
		kb.result = result;
		kb.title.data = res_getLabel(LANG_LABEL_ENTER_PASSPHRASE);
		kb.flag = KEYBOARD_FLAG_MULTI_LINE | KEYBOARD_FLAG_HIDE_MORE;
		kb.result_limit = 61;
		kb.editor_style = KEYBOARD_EDITOR_INPUT;

		kb.text_field.data = kb.result;
		kb.initKeyIndex = get_random_keyboard_index(kb.keyStyle);
		result_len = showKeyBoard(mHwnd, &kb);
		if (result_len == KEY_EVENT_ABORT) {
			return KEY_EVENT_BACK;
		} else if (result_len < 0) {
			return result_len;
		} else if (result_len) {
			done = 1;
		}
	} while (!done);
	if (strlen(result) > 0) {
		strlcpy(passphrase, result, size);
		ret = 0;
	} else {
		ret = -4;
	}
	memzero(result, sizeof(result));
	return ret;
}

int VerifyWin::guide() {
	int nextIndex;
	int passwd_ok = 0;
	unsigned char passhash[PASSWD_HASHED_LEN] = {0};
	char mnenonics[MNEMONIC_MAX_LEN * MAX_MNEMONIC_CNT] = {0};

	int mlen = 0;
	int ret = 0;
	int have_mnemonic = get_have_mnemonic();
	uint64_t old_account_id = wallet_AccountId();
	db_msg("have_mnemonic:%d old_account_id:%llx", have_mnemonic, old_account_id);
	if (have_mnemonic < 0 || !old_account_id) {
		dialog_error3(mHwnd, -401, "Seed verify failed.");
		return -1;
	}
	Stack *stack = newSlack(20);
	if (!stack) {
		return -1;
	}
	pushData(stack, OP_INDEX_VERIFY_TIPS);

	do {
		ret = 0;
		nextIndex = getStackTop(stack);
		db_msg("next index:%d", nextIndex);
		switch (nextIndex) {
			case OP_INDEX_VERIFY_TIPS: {
				ret = picDialog(mHwnd, "wallet_verify_tips", res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					nextIndex = OP_INDEX_MAX; //exit
				} else if (ret >= 0) {
					pushData(stack, OP_INDEX_ENTER_PASSWD);
				}
			}
				break;
			case OP_INDEX_ENTER_PASSWD: {
				popData(stack);
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				if (!passwd_ok || buffer_is_zero(passhash, sizeof(passhash))) {
					memzero(passhash, sizeof(passhash));
					ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, PASSKB_FLAG_RANDOM);
				} else {
					ret = 0;
				}
				if (ret == KEY_EVENT_ABORT) {
					memzero(passhash, sizeof(passhash));
				} else if (ret < 0) {
					memzero(passhash, sizeof(passhash));
					if (ret == USER_PASSWD_ERR_VERIFY) {
						mChangeWin = -1;
						nextIndex = OP_INDEX_MAX;
					}
					break;
				} else {
					passwd_ok = 1;
					pushData(stack, OP_INDEX_SELECT_MNEMONIC_CNT);
				}
			}
				break;
			case OP_INDEX_SELECT_MNEMONIC_CNT: {
				ret = selectMnemonicCnt();
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (IS_VALID_MNEMONIC_LEN(ret)) {
					mlen = ret;
					pushData(stack, OP_INDEX_VERIFY_MNEMONIC_TIPS);
				}
			}
				break;
			case OP_INDEX_VERIFY_MNEMONIC_TIPS: {
				ret = picDialog(mHwnd, "recovery_word_tips", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret == 0) {
					pushData(stack, OP_INDEX_VERIFY_MNEMONIC);
				}
			}
				break;
			case OP_INDEX_VERIFY_MNEMONIC: {
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				memzero(mnenonics, sizeof(mnenonics));
				ret = verifyMnemonic(passhash, mnenonics, MNEMONIC_MAX_LEN * mlen, mlen);
				memzero(mnenonics, sizeof(mnenonics));
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret == 0 || ret == 88) { //verify ok or exit
					popData(stack);
					nextIndex = OP_INDEX_MAX;
				}
			}
				break;
			default:
				break;
		}
		set_temp_screen_time(DEFAULT_SCREEN_SAVER_TIME);
		if (nextIndex != OP_INDEX_MAX && !is_key_event_value(ret) && ret < 0) {
			dialog_error3(mHwnd, -(nextIndex * 1000) + ret, "Seed verify failed.");
			nextIndex = OP_INDEX_VERIFY_TIPS;
		}
	} while (nextIndex != OP_INDEX_MAX);
	memzero(passhash, sizeof(passhash));
	memzero(mnenonics, sizeof(mnenonics));
	freeSlack(stack);
	return 0;
}
