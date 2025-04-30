#define LOG_TAG "Passphrase"

#include <stdlib.h>
#include "Passphrase.h"
#include "debug.h"
#include "wallet_proto.h"
#include "storage_manager.h"
#include "key_event.h"
#include "Dialog.h"
#include "fileutil.h"
#include "secure_api.h"
#include "wallet_util.h"
#include <bip39.h>
#include "ActionSheetDialog.h"
#include "Stack.h"
#include <memzero.h>
#include "wallet_manager.h"
#include "PicDialog.h"
#include "loading_win.h"
#include "CommonQRShow.h"

static int gMmenCntVals[MNEMNONIC_CNT_LEVEL_MAX] = {12, 15, 18, 21, 24};
static const char *gMmenCntStr[MNEMNONIC_CNT_LEVEL_MAX] = {"12", "15", "18", "21", "24"};

#define DEFAULT_SCREEN_SAVER_TIME 60
#define DEFAULT_HI_SCREEN_SAVER_TIME 600
#define MAX_MNEMONIC_CNT  24
#define MAX_MNEMONIC_BUFFSIZE  32
#define SUPPORT_SELECT_MNEMONIC_CNT 3
#define IS_VALID_MNEMONIC_LEN(l) ( (l)>=12 && (l)<=24 && ( ((l)%3)==0 ))

typedef enum {
	OP_INDEX_PASSPHRASE_TIPS = 1,
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

Passphrase::Passphrase() {
	isResume = 0;
	mChangeWin = 0;
}

Passphrase::~Passphrase() {

}

PROC_RET Passphrase::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int Passphrase::saveWalletName(const char *passphrase) {
	KeyBoardConfig_t kb;
	char result[WALLET_NAME_MAX_LEN + 1] = {0};
	int result_len = 0;
	char confirm_msg[DIALOG_MSG_MAX_LEN] = {0};
	int ret = 0;
	int done = 0;
	char *wallet_name = confirm_msg;
	uint64_t account = wallet_AccountId();
	if (!account) {
		db_error("not account");
		return -11;
	}
	db_secure("passphrase:%s", passphrase);
	memzero(wallet_name, 40);
	if (storage_get_account_name(account, wallet_name, 32) > 0) {
		if (settings_set_device_name(wallet_name) < 0) {
			db_error("save wallet name:%s false", wallet_name);
			return -12;
		}

		PicDialogConfig_t config;
		memset(&config, 0, sizeof(PicDialogConfig_t));
		const char *btnTitles[2] = {0, 0};
		config.name = "passphrase_orgin_named";
		config.total = 1;
		config.initIndex = 0;
		btnTitles[0] = res_getLabel(LANG_LABEL_TXT_OK);
		btnTitles[1] = 0;
		config.btnTitles = btnTitles;
		config.buttonCount = 1;
		config.initBtnIndex = 0;
		config.flag = PIC_DLG_FLAG_LEFT_NOT_BACK;
		config.dync_text = wallet_name;
		config.dync_label = "passphrase_oldname_label";
		ret = showPicDialog(mHwnd, &config);
		return 0;
	}

	ret = picDialogFlag(mHwnd, "name_wallet_create_tips", res_getLabel(LANG_LABEL_NAME_WALLET), NULL, 0, PIC_DLG_FLAG_LEFT_NOT_BACK);

	memset(&kb, 0, sizeof(KeyBoardConfig_t));
	kb.keyStyle = KEYBOARD_STYLE_FULL;
	kb.result = result;
	kb.result_limit = WALLET_NAME_MAX_LEN;
	kb.editor_style = KEYBOARD_EDITOR_INPUT;
	kb.title.data = res_getLabel(LANG_LABEL_INPUT_WALLET_NAME);
	kb.initKeyIndex = get_random_keyboard_index(kb.keyStyle);
	do {
		kb.text_field.data = kb.result;
		result_len = showKeyBoard(mHwnd, &kb);
		if (result_len == KEY_EVENT_ABORT) {
			continue;
		} else if (result_len < 0) {
			return result_len;
		} else if (result_len > 0) {
			if (is_not_empty_string(passphrase) && strcmp(kb.result, passphrase) == 0) {
				dialog_error(mHwnd, res_getLabel(LANG_LABEL_NAME_EQUAL_PASSPHRASE));
				continue;
			}
			snprintf(confirm_msg, sizeof(confirm_msg), res_getLabel(LANG_LABEL_WALLET_NAME_CONFIRM), kb.result);
			ret = dialog_l(mHwnd,
			               NULL,
			               DIALOG_ICON_STYLE_SUCCESS,
			               confirm_msg,
			               DIALOG_BUTTON_ALIGN_VERT,
			               res_getLabel(LANG_LABEL_BACK),
			               res_getLabel(LANG_LABEL_SUBMENU_OK),
			               1);
			if (ret == KEY_EVENT_BACK) {
				continue;
			} else if (ret == 1) {
				if (settings_set_device_name(kb.result) < 0) {
					db_error("save wallet name:%s false", kb.result);
					return -13;
				}
				storage_set_account_name(account, kb.result, strlen(kb.result));
				snprintf(confirm_msg, sizeof(confirm_msg), res_getLabel(LANG_LABEL_WALLET_SWITCHED), kb.result);
				dialog_l(mHwnd,
				         NULL,
				         DIALOG_ICON_STYLE_SUCCESS,
				         confirm_msg,
				         DIALOG_BUTTON_ALIGN_VERT,
				         res_getLabel(LANG_LABEL_SUBMENU_OK),
				         NULL,
				         0);

				done = 1;
			}
		}
		db_msg("input wallet name:%s", kb.result);
	} while (!done);

	return 0;
}

int Passphrase::onResume() {
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

int Passphrase::onPause() {
	isResume = 0;
	set_temp_screen_time(0);
	return 0;
}

int Passphrase::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d", keyCode);
	return 0;
}

int Passphrase::selectMnemonicCnt() {
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

int Passphrase::verifyMnemonic(const unsigned char *passwd, char *mnemonics, int size, int mlen) {
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
				err = 1;
				break;
			}
			ret = mnemonic_check(mnemonics);
			db_secure("mnemonic check ret:%d", ret);
			if (!ret) {
				ret = picDialog(mHwnd, "verified_word_failed_alert", res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
				if (is_key_event_value(ret)) {
					index = mlen - 1;
				} else if (ret < 0) {
					err = 1;
					break;
				} else {
					lastKey = KEY_EVENT_BACK;
				}
			} else {
				loading_win_start(mHwnd, "", NULL, 0);
				int verify_ret = wallet_verify_mnemonic((const unsigned char *) mnemonics, strlen(mnemonics), passwd);
				loading_win_stop();
				db_secure("verify mnemonic ret:%d", verify_ret);
				if (verify_ret != 0) {
					if (verify_ret == -0x4f) {
						picDialog(mHwnd, "verify_mnemonic_failed", res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
					} else {
						dialog_error3(mHwnd, verify_ret, "Passphrase setting failed.");
					}
					index = mlen - 1;
				} else {
					picDialogFlag(mHwnd, "verify_mnemonic_ok", res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0, PIC_DLG_FLAG_LEFT_NOT_BACK);
					checked_word = 1;
					int have_m = get_have_mnemonic();
					if (have_m != 1) {
                        dialog_error3(mHwnd, -404, "Passphrase setting failed.");
					} else {
						break;
					}
				}
			}
		}
	} while (1);
	memzero(kb_result, sizeof(kb_result));
	memzero(srcMnemonics, sizeof(srcMnemonics));
	memzero(mnemonics, sz);
	if (err) {
		return -1;
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

static int backup_device_name() {
	char name[32];
	uint64_t account = wallet_AccountId();
	if (!account) {
		db_error("not account");
		return -1;
	}
	memzero(name, sizeof(name));
	if (storage_get_account_name(account, name, sizeof(name)) > 0) {
		return 0;
	}
	memzero(name, sizeof(name));
	if (get_device_name(name, 32, 0) < 1) {
		return -1;
	}
	return storage_set_account_name(account, name, strlen(name));
}

int Passphrase::enterPassphrase(char *passphrase, int size) {
	KeyBoardConfig_t kb;
	char result[64] = {0};
	char result2[64] = {0};
	int result_len = 0;
	int ret = 0;
	int done = 0;
	int input_time = 0;
	do {
		memset(&kb, 0, sizeof(KeyBoardConfig_t));
		kb.keyStyle = KEYBOARD_STYLE_FULL;
		if (input_time == 0) {
			kb.result = result;
			kb.title.data = res_getLabel(LANG_LABEL_ENTER_PASSPHRASE);
		} else {
			kb.result = result2;
			kb.title.data = res_getLabel(LANG_LABEL_ENTER_CONFIRM_PASSPHRASE);
		}
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
			if (!input_time) {
				ret = check_passphrase_valid(result);
				db_msg("check passphrase ret:%d", ret);
				if (ret) {
					dialog_l(mHwnd,
					         NULL,
					         DIALOG_ICON_STYLE_ERR,
					         ret > 0 ? res_getLabel(LANG_LABEL_PASSPHRASE_INVALID_TIPS0 + ret - 1) : "Invalid Passphrase",
					         DIALOG_BUTTON_ALIGN_VERT,
					         res_getLabel(LANG_LABEL_RE_ENTER),
					         NULL,
					         0);
					memzero(result2, sizeof(result2));
					continue;
				}
				input_time = 1;
			} else {
				if (strlen(result) <= 0 || strcmp(result, result2) != 0) {
					dialog_alert(mHwnd, DIALOG_ICON_STYLE_ERR, res_getLabel(LANG_LABEL_PASSPHRASE_DIFFERENT), res_getLabel(LANG_LABEL_RE_ENTER));
					input_time = 0;
					memzero(result, sizeof(result));
					memzero(result2, sizeof(result2));
					continue;
				}

				PicDialogConfig_t config;
				memset(&config, 0, sizeof(PicDialogConfig_t));
				const char *btnTitles[2] = {0, 0};
				config.name = "passphrase_confirm";
				config.total = 1;
				config.initIndex = 0;
				btnTitles[0] = res_getLabel(LANG_LABEL_TXT_OK);
				btnTitles[1] = res_getLabel(LANG_LABEL_BACK);
				config.btnTitles = btnTitles;
				config.buttonCount = 2;
				config.initBtnIndex = 0;
				config.flag = 0;
				config.dync_label = "passphrase_confirm_label";
				config.dync_text = result;
				ret = showPicDialog(mHwnd, &config);
				if (ret < 0 || ret == 1) {
					input_time = 0;
					memzero(result, sizeof(result));
					memzero(result2, sizeof(result2));
				} else if (ret == 0) {
					done = 1;
				}
			}
		}
	} while (!done);
	if (strlen(result) > 0 && strcmp(result, result2) == 0) {
		strlcpy(passphrase, result, size);
		ret = 0;
	} else {
		ret = -4;
	}
	memzero(result, sizeof(result));
	memzero(result2, sizeof(result2));
	return ret;
}

int Passphrase::guide() {
	int nextIndex;
	int passwd_ok = 0;
	unsigned char passhash[PASSWD_HASHED_LEN] = {0};
	char mnenonics[MNEMONIC_MAX_LEN * MAX_MNEMONIC_CNT] = {0};
	char passphrase_val[64] = {0};
	int mlen = 0;
	int passphrase_type = -1;
	int ret = 0;
	int have_mnemonic = get_have_mnemonic();
	uint64_t old_account_id = wallet_AccountId();
	db_msg("have_mnemonic:%d old_account_id:%llx", have_mnemonic, old_account_id);
	if (have_mnemonic < 0 || !old_account_id) {
		dialog_error3(mHwnd, -401, "Passphrase setting failed.");
		return -1;
	}
	Stack *stack = newSlack(20);
	if (!stack) {
		return -1;
	}
	pushData(stack, OP_INDEX_PASSPHRASE_TIPS);

	do {
		ret = 0;
		nextIndex = getStackTop(stack);
		db_msg("next index:%d", nextIndex);
		switch (nextIndex) {
			case OP_INDEX_PASSPHRASE_TIPS: {
				ret = picDialog(mHwnd, "passphrase_tips", res_getLabel(LANG_LABEL_SET_PASSPHRASE), res_getLabel(LANG_LABEL_WHATS_PASSPHRASE), 1);
				if (ret == KEY_EVENT_BACK) {
					nextIndex = OP_INDEX_MAX; //exit
				} else if (ret >= 0) {
					if (ret) {
						pushData(stack, OP_INDEX_PASSPHRASE_DETAIL_TIPS);
					} else {
						pushData(stack, OP_INDEX_ACTION_DETECTION);
					}
				}
			}
				break;
			case OP_INDEX_PASSPHRASE_DETAIL_TIPS: {
				ret = picDialogFlag(mHwnd, "what_is_passphrase", res_getLabel(LANG_LABEL_I_KNOW), NULL, 0, PIC_DLG_FLAG_MULTI_PIC);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret == 0) {
					pushData(stack, OP_INDEX_ACTION_DETECTION);
				}
			}
				break;
			case OP_INDEX_ACTION_DETECTION:
				popData(stack);
				if (have_mnemonic) {
					pushData(stack, OP_INDEX_SELECT_PASSPHRASE_TYPE);
				} else {
					passphrase_type = 0;
					pushData(stack, OP_INDEX_ENTER_PASSWD);
				}
				break;
			case OP_INDEX_SELECT_PASSPHRASE_TYPE: {
				ret = dialog(mHwnd,
				             "",
				             DIALOG_ICON_STYLE_NONE,
				             res_getLabel(LANG_LABEL_IS_ENABLE_PASSPHRASE),
				             DIALOG_BUTTON_ALIGN_VERT,
				             res_getLabel(LANG_LABEL_NOT_PASSPHRASE),
				             res_getLabel(LANG_LABEL_USE_PASSPHRASE),
				             1);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					passphrase_type = ret ? 2 : 1;
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
					backup_device_name();
					passwd_ok = 1;
					if (passphrase_type == 0) {
						pushData(stack, OP_INDEX_FIRST_USE_PASSPHRASE_TIPS);
					} else if (passphrase_type == 1) {
						memzero(passphrase_val, sizeof(passphrase_val));
						pushData(stack, OP_INDEX_SAVE_PASSPHRASE);
					} else {
						pushData(stack, OP_INDEX_ENTER_PASSPHRASE);
					}
				}
			}
				break;
			case OP_INDEX_FIRST_USE_PASSPHRASE_TIPS: {
				ret = picDialog(mHwnd, "first_use_passphrase", res_getLabel(LANG_LABEL_NEXT_STEP), res_getLabel(LANG_LABEL_BACK), 0);
				if (ret == KEY_EVENT_BACK || ret == 1) {
					popData(stack);
				} else if (ret == 0) {
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
				} else if (ret == 0) { //verify oK
					have_mnemonic = 1;
					passphrase_type = 2;
					while (popData(stack) > 0); //clean
					pushData(stack, OP_INDEX_PASSPHRASE_TIPS);
					pushData(stack, OP_INDEX_ENTER_PASSPHRASE);
				}
			}
				break;
			case OP_INDEX_ENTER_PASSPHRASE: {
				memzero(passphrase_val, sizeof(passphrase_val));
				ret = enterPassphrase(passphrase_val, sizeof(passphrase_val));
				if (ret == KEY_EVENT_BACK) {
					memzero(passphrase_val, sizeof(passphrase_val));
					popData(stack);
				} else if (ret < 0) {
					memzero(passphrase_val, sizeof(passphrase_val));
					db_error("enter passphrase failed ret:%d", ret);
					break;
				} else {
					pushData(stack, OP_INDEX_SAVE_PASSPHRASE);
				}
			}
				break;
			case OP_INDEX_SAVE_PASSPHRASE: {
				if ((passphrase_type == 1 && passphrase_val[0] != 0) || (passphrase_type != 1 && passphrase_val[0] == 0)) {
					popData(stack);
					dialog_error3(mHwnd, -403, "Passphrase setting failed.");
					break;
				}
				loading_win_start(mHwnd, res_getLabel(LANG_LABEL_INIT_WALLET_ING), res_getLabel(LANG_LABEL_INIT_WALLET_TIPS), 0);
				ret = wallet_store_passphrase((const unsigned char *) passphrase_val, strlen(passphrase_val), passhash);
				if (ret == 0) {
					loading_win_refresh();
					wallet_gen_exists_hdnode(passhash, old_account_id);
				}
				loading_win_stop();
				if (ret == 0) {
					popData(stack);
					pushData(stack, OP_INDEX_WALLET_NAME_TIPS);
				}
			}
				break;
			case OP_INDEX_WALLET_NAME_TIPS: {
				ret = saveWalletName(passphrase_val);
				if (ret == KEY_EVENT_BACK || ret == KEY_EVENT_ABORT) {
				} else if (ret == 0) {
					popData(stack);
					mChangeWin = WINDOWID_MAINPANEL;
					nextIndex = OP_INDEX_MAX;
				}
			}
				break;
			default:
				break;
		}
		set_temp_screen_time(DEFAULT_SCREEN_SAVER_TIME);
		if (nextIndex != OP_INDEX_MAX && !is_key_event_value(ret) && ret < 0) {
			dialog_error3(mHwnd, -(nextIndex * 1000) + ret, "Passphrase setting failed.");
			nextIndex = OP_INDEX_PASSPHRASE_TIPS;
		}
	} while (nextIndex != OP_INDEX_MAX);
	memzero(passhash, sizeof(passhash));
	memzero(mnenonics, sizeof(mnenonics));
	memzero(passphrase_val, sizeof(passphrase_val));
	freeSlack(stack);
	return 0;
}
