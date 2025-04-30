#define LOG_TAG "GuideWin"

#include <stdlib.h>
#include "GuideWin.h"
#include "GuiMain.h"
#include "debug.h"
#include "device.h"
#include "wallet_proto.h"
#include "storage_manager.h"
#include "rand.h"
#include "key_event.h"
#include "Dialog.h"
#include "fileutil.h"
#include "secure_api.h"
#include "wallet_util.h"
#include <bip39.h>
#include <bip39_english.h>
#include <sha2.h>
#include "BackupSeedWord.h"
#include "ActionSheetDialog.h"
#include "ConfirmSeedWord.h"
#include "Stack.h"
#include <memzero.h>
#include "wallet_manager.h"
#include "PicDialog.h"
#include "loading_win.h"
#include "CommonQRShow.h"
#include "active_util.h"
#include "wallet_util_hw.h"

static int gMmenCntVals[MNEMNONIC_CNT_LEVEL_MAX] = {12, 15, 18, 21, 24};
static const char *gMmenCntStr[MNEMNONIC_CNT_LEVEL_MAX] = {"12", "15", "18", "21", "24"};

#define DEFAULT_SCREEN_SAVER_TIME 60
#define DEFAULT_HI_SCREEN_SAVER_TIME 600
#define MAX_MNEMONIC_CNT  24
#define MAX_MNEMONIC_BUFFSIZE  32
#define SUPPORT_SELECT_MNEMONIC_CNT 3
#define IS_VALID_MNEMONIC_LEN(l) ( (l)>=12 && (l)<=24 && ( ((l)%3)==0 ))
#define ACCOUNT_TYPE_NEW_GEN    (1)
#define ACCOUNT_TYPE_RECOVERY   (2)

typedef enum {
	GUIDE_INDEX_LANG = 1,
	GUIDE_DEVICE_ACTIVE,
	GUIDE_INDEX_GREET,
	GUIDE_INDEX_DOWNLOAD_APP_TIPS,
	GUIDE_INDEX_WALLET_NAME_TIPS,
	GUIDE_INDEX_INPUT_WALLET_NAME,
	GUIDE_INDEX_ACCOUNT_OP_SELECT,
	GUIDE_INDEX_ACCOUNT_RECOVERY,
	GUIDE_INDEX_MNEMONICS_DETAIL_TIPS,
	GUIDE_INDEX_MNEMONICS_CNT,
	GUIDE_INDEX_MNEMONICS_TIPS,
	GUIDE_INDEX_ACCOUNT_RECOVERY_TIPS,
	GUIDE_INDEX_MNEMONICS_GENERATE,
	GUIDE_INDEX_MNEMONICS_SHOW,
	GUIDE_INDEX_MNEMONICS_CONFIRM_TIPS,
	GUIDE_INDEX_MNEMONICS_CONFIRM,
	GUIDE_INDEX_PASSWD_TIPS,
	GUIDE_INDEX_ENTER_PASSWD,
	GUIDE_INDEX_KEY_INSTRUCTION,
	GUIDE_INDEX_CHECK_SECHIP,
	GUIDE_INDEX_MAX
} guideIndex;

static int getErrorCode(int code, int id) {
	return code * -1000 + id;
}

static int randomBufSize(int mlen) {
	if (!IS_VALID_MNEMONIC_LEN(mlen)) {
		db_error("mnemonic word count:%d false", mlen);
		return 0;
	}
	return (mlen * 11 - mlen / 3) / 8;
}

GuideWin::GuideWin() {
	isResume = 0;
}

GuideWin::~GuideWin() {

}

PROC_RET GuideWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int GuideWin::setupLang(void) {
	int support[LANG_MAXID] = {0};
	const char *langs[LANG_MAXID] = {0};
	int supportCnt = settings_get_all_langs(support);
	for (int j = 0; j < supportCnt; ++j) {
		langs[j] = res_getLangName(support[j]);
	}
	int curLang = settings_get_lang();
	int init_index = 0;
	for (int i = 0; i < supportCnt; ++i) {
		if (curLang == support[i]) {
			init_index = i;
			break;
		}
	}

	ActionSheetDialogConfig_t _config;
	ActionSheetDialogConfig_t *config = &_config;
	memset(config, 0, sizeof(ActionSheetDialogConfig_t));
	config->itemsCnt = supportCnt;
	config->items = langs;
	config->initIndex = init_index;
	config->title.data = "Language";
	//config->flag = ACTION_SHEET_DIALOG_FLAG_NAVI;

	int ret = showActionSheetDialog(mHwnd, config);
	if (is_key_event_value(ret)) {
		return ret;
	}
	if (ret < 0 || ret >= supportCnt) {
		db_error("setup lang action sheet false %d", ret);
		return -1;
	}
	int newLang = support[ret];
	db_msg("lang new:%d current:%d ret:%d", newLang, curLang, ret);
	if (newLang != curLang && IS_VALID_LANG_ID(newLang)) {
		if (settings_save(SETTING_KEY_LANGUAGE, newLang)) {
			db_msg("lang:%d save false", newLang);
			return -1;
		}
		res_updateLangAndFont(newLang);
	}
	return 0;
}


int GuideWin::activeDevice() {
	int ret;
	char url_buffer[128];
	int result = KEY_EVENT_BACK;
	int step = 0;
	if (Global_Active_step) {
		step = 2;
		Global_Active_step = 0;
	}
	do {
		if (step < 0) {
			break;
		}
		if (step == 0) {
			ret = picDialogFlag(mHwnd, "verify_wallet_security_tips", res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0, PIC_DLG_FLAG_RIGHT_AS_OK);
			if (ret == KEY_EVENT_BACK) {
				step--;
				continue;
			} else if (ret == 0) {
				step++;
			}
		}

		if (step == 1) {
			ret = active_get_url(url_buffer, sizeof(url_buffer));
			if (ret <= 0) {
				dialog_error3(mHwnd, -1, "Activate failed.");
				step--;
				continue;
			}
			ret = showCommonQR(mHwnd, (const unsigned char *) url_buffer, ret, res_getLabel(LANG_LABEL_NEXT_STEP), COMMON_QR_STYLE_BUTTON);
			db_msg("ret:%d", ret);
			if (ret == 0 || ret == KEY_EVENT_OK) {
				step++;
			} else {
				step--;
				continue;
			}
		}

		if (step == 2) {
			PicDialogConfig_t config;
			memset(&config, 0, sizeof(PicDialogConfig_t));
			const char *btnTitles[2] = {0, 0};
			config.name = "please_input_verify_code";
			config.total = 1;
			config.initIndex = 0;
			btnTitles[0] = res_getLabel(LANG_LABEL_NEXT_STEP);
			config.btnTitles = btnTitles;
			config.buttonCount = 1;
			config.initBtnIndex = 0;
			config.flag = PIC_DLG_FLAG_RIGHT_AS_OK;
			config.onInitCallback = active_init_vnum_cb;
			ret = showPicDialog(mHwnd, &config);
			if (ret == KEY_EVENT_BACK) {
				step--;
				continue;
			} else if (ret == 0) {
				step++;
			}
		}
		if (step == 3) {
			Global_Active_step = 1;
			result = 1;
			changeWindow(WINDOWID_SCAN);
			break;
		}
		result = 0;
		break;
	} while (1);
	active_free_vnum(1);
	return result;
}

int GuideWin::saveWalletName(void) {
	KeyBoardConfig_t kb;
	char result[WALLET_NAME_MAX_LEN + 1] = {0};
	int result_len = 0;
	char confirm_msg[DIALOG_MSG_MAX_LEN] = {0};
	int ret = 0;
	int done = 0;
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
		} else if (result_len) {
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
					return -1;
				}
				storage_set_account_name(wallet_AccountId(), kb.result, strlen(kb.result));
				done = 1;
			}
		}
		db_msg("input wallet name:%s", kb.result);
	} while (!done);

	return 0;
}

int GuideWin::onResume() {
	db_msg("onResume called");
	if (!mHwnd) {
		db_error("hMwd:%d is null", mHwnd);
		return -1;
	}
	isResume = 1;
	set_temp_screen_time(DEFAULT_SCREEN_SAVER_TIME);
	startGuide();
	return 0;
}

int GuideWin::onPause() {
	isResume = 0;
	set_temp_screen_time(0);
	return 0;
}

int GuideWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d", keyCode);
	return 0;
}

int GuideWin::enterPassword(unsigned char passhash[PASSWD_HASHED_LEN]) {
	if (!passhash) {
		db_error("invalid paras passhash:%p", passhash);
		return -1;
	}
	memzero(passhash, PASSWD_HASHED_LEN);
	unsigned char new_passhash[PASSWD_HASHED_LEN];
	int ret = 0;
	int done = 0;
	int step = 0; // enter passwd again

	do {
		memzero(passhash, PASSWD_HASHED_LEN);
		memzero(new_passhash, PASSWD_HASHED_LEN);
		ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_CHECK, passhash, 0);
		if (ret == KEY_EVENT_ABORT) {
			memzero(passhash, PASSWD_HASHED_LEN);
			memzero(new_passhash, PASSWD_HASHED_LEN);
			return USER_PASSWD_ERR_ABORT;
		}
		if (ret == USER_PASSWD_ERR_FORMAT || ret == USER_PASSWD_ERR_WEAK || ret == USER_PASSWD_ERR_NOT_INPUT) {
			continue;
		}
		if (ret < 0) {
			db_error("set passwd false ret:%d", ret);
			memzero(passhash, PASSWD_HASHED_LEN);
			memzero(new_passhash, PASSWD_HASHED_LEN);
			return ret;
		}
		step = 1;
		do {
			ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_CONFIRM_PASSWD), PIN_CODE_NONE, new_passhash, 0);
			if (ret == KEY_EVENT_ABORT) {
				memzero(passhash, PASSWD_HASHED_LEN);
				memzero(new_passhash, PASSWD_HASHED_LEN);
				return KEY_EVENT_ABORT;
			}
			if (ret == USER_PASSWD_ERR_FORMAT || ret == USER_PASSWD_ERR_WEAK) {
				step = 0;
				break;
			}
			if (ret == USER_PASSWD_ERR_NOT_INPUT) {
				continue;
			}
			if (ret < 0) {
				db_error("confirm passwd false ret:%d", ret);
				memzero(passhash, PASSWD_HASHED_LEN);
				memzero(new_passhash, PASSWD_HASHED_LEN);
				return ret;
			}
			if (ret == USER_PASSWD_ERR_NONE) {
				break;
			}
		} while (1);

		if (!step) {
			continue;
		}
		if (memcmp(passhash, new_passhash, PASSWD_HASHED_LEN) != 0) {
			db_error("passswd hash compare false");
			picDialog(mHwnd, "inconsistent_pin_code", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
			continue;
		} else {
			done = 1;
		}
	} while (!done);
	memzero(new_passhash, PASSWD_HASHED_LEN);
	return 0;
}

int GuideWin::selectMnemonicCnt() {
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

int GuideWin::generateMnemonic(uint16_t *mnIndexes, int mlen, unsigned char *randomBuf) {
	int ret = 0;
	int random_sz = randomBufSize(mlen);
	unsigned char tmpbuf[MAX_MNEMONIC_BUFFSIZE] = {0};
	uint16_t tmpIndex[24] = {0};

	if (NULL == mnIndexes || NULL == randomBuf) {
		db_error("invalid mnIndexes:%p randomBuf:%p", mnIndexes, randomBuf);
		return -1;
	}
	if (!IS_VALID_MNEMONIC_LEN(mlen)) {
		db_error("mnemonic cnt:%d false", mlen);
		return -2;
	}
	db_secure("sapi get random start sz:%d", random_sz);
	ret = get_mix_random_buffer(tmpbuf, random_sz);
	db_secure("sapi get random:%s ret:%d", debug_ubin_to_hex(tmpbuf, random_sz), ret);
	if (ret != random_sz) {
		db_error("get random failed ret:%d", ret);
		memzero(tmpbuf, sizeof(tmpbuf));
		return -3;
	}
	ret = mnemonic_index_from_data(tmpbuf, random_sz, tmpIndex);
	if (ret != mlen) {
		db_error("get random index failed ret:%d", ret);
		memzero(tmpbuf, sizeof(tmpbuf));
		memzero(tmpIndex, sizeof(tmpIndex));
		return -4;
	}

	memcpy(mnIndexes, tmpIndex, sizeof(uint16_t) * mlen);
	memcpy(randomBuf, tmpbuf, random_sz);

	memzero(tmpbuf, sizeof(tmpbuf));
	memzero(tmpIndex, sizeof(tmpIndex));

	return 0;
}

int GuideWin::showMnemonicsWillChangeAlert(void) {
	int ret = 0;
	do {
		ret = picDialog(mHwnd,
		                "exit_word_phase_confirm_tips",
		                res_getLabel(LANG_LABEL_SUBMENU_OK),
		                res_getLabel(LANG_LABEL_SUBMENU_CANCEL), 1);
		if (ret == KEY_EVENT_BACK) {
			continue;
		} else if (ret < 0) {
			return ret;
		} else if (ret == 1) {
			return 0;
		} else if (ret == 0) {
			return KEY_EVENT_BACK;
		}
	} while (1);
	return 0;
}

int GuideWin::showMnemonic(const uint16_t *mnIndexes, int mlen) {
	if (NULL == mnIndexes) {
		db_error("invalid mnIndexes:%p", mnIndexes);
		return -1;
	}
	int ret = 0;
	BackupSeedWordConfig_t _config;
	BackupSeedWordConfig_t *config = &_config;
	config->seeds = mnIndexes;
	config->seedWordCnt = mlen;
	ret = showBackupSeedWord(mHwnd, config);
	return ret;
}

int GuideWin::confirmMnemonic(const uint16_t *mnIndexes, int mlen) {
	if (!mnIndexes || mlen <= 0) {
		db_error("invalid paras  indexes:%p, mlen:%d", mnIndexes, mlen);
		return -1;
	}
	int ret = 0;
	ConfirmSeedWordConfig_t _config;
	ConfirmSeedWordConfig_t *config = &_config;
	config->title.data = "";
	config->seedWordCnt = mlen;
	config->seeds = mnIndexes;
	ret = showConfirmSeedWord(mHwnd, config);
	return ret;
}

int GuideWin::showPasswdTips() {
	int ret = 0;
	int isAbort = 0;
	int err = 0;
	do {
		const char *name = "word_verified_success";
		ret = picDialog(mHwnd, name, res_getLabel(LANG_LABEL_START_SET_PASSWD),
		                res_getLabel(LANG_LABEL_PASSWD_USEAGE), 0);
		if (ret == KEY_EVENT_BACK) {
			return KEY_EVENT_BACK;
		}
		if (ret < 0) {
			db_error("show mnemonic finish dialog error ret:%d", ret);
			return ret;
		}
		if (!ret) {
			return ret;
		}
		name = "what_is_pin";
		ret = picDialogFlag(mHwnd, name, res_getLabel(LANG_LABEL_START_SET_PASSWD), NULL, 0, PIC_DLG_FLAG_MULTI_PIC);
		db_msg("what's pin dialog ret:%d", ret);
		if (ret == KEY_EVENT_BACK) {
			continue;
		}
		return ret;
	} while (1);

}

int GuideWin::enterRecoveryWord(char *mnemonics, int size, int mlen) {
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
		db_secure("mnemonics index:%d str:%s", index, srcMnemonics + index * MNEMONIC_MAX_LEN);
		snprintf(title, sizeof(title), res_getLabel(LANG_LABEL_ENTER_MNEMONIC), index + 1);
		kb.title.data = title;
		kb.text_field.data = kb_result;
		db_secure("keyboard init result:%s", kb_result);
		if (strlen(kb_result)) {
			kb.initKeyIndex = (lastKey == KEY_EVENT_BACK) ? CHAR_KEYBOARD_BACK_KEY_INDEX : CHAR_KEYBOARD_OK_KEY_INDEX;
		} else if (lastKey == KEY_EVENT_BACK) {
			kb.initKeyIndex = CHAR_KEYBOARD_BACK_KEY_INDEX;
		} else {
			kb.initKeyIndex = get_random_keyboard_index(kb.keyStyle);;
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
				checked_word = 1;
				break;
			}
		}
	} while (1);
	memzero(kb_result, sizeof(kb_result));
	memzero(srcMnemonics, sizeof(srcMnemonics));
	if (err) {
		memzero(mnemonics, sz);
		return -1;
	}
	if (isBack) {
		memzero(mnemonics, sz);
		return KEY_EVENT_BACK;
	}
	if (!checked_word) {
		memzero(mnemonics, sz);
		return -1;
	}
	return 0;
}

int GuideWin::checkSechip(int type) {
	const sec_base_info *baseInfo = wallet_getBaseInfo();
	if (!baseInfo) {
		db_error("wallet not base info,error");
        return -1;
	}
	return 0;
}

int GuideWin::clean_data() {
	if (access(DB_PATH, F_OK) != 0) {
		return 0;
	}
	db_msg("clean all data");
	loading_win_start(mHwnd, "", NULL, 0);
	storage_cleanAllData();
	loading_win_refresh();
	AppMain::getInstance()->doFactoryReset();
	loading_win_refresh();
	int ret = device_reformat_data_partition();
	db_msg("reformat data ret:%d", ret);
	loading_win_stop();
	if (gSettings->mLang != 0) {
		settings_save(SETTING_KEY_LANGUAGE, settings_get_lang());
	}
	return ret;
}

int GuideWin::startGuide(void) {
	db_msg("startGuide");
	int nextIndex = GUIDE_INDEX_LANG;
	uint16_t mIndexes[MAX_MNEMONIC_CNT] = {0};
	unsigned char randomBuf[MAX_MNEMONIC_BUFFSIZE] = {0};
	char mnenonics[MNEMONIC_MAX_LEN * MAX_MNEMONIC_CNT] = {0};
	char tmpbuf[256];
	int mlen = 0;
	int recovery_flag = 0;
	int ret = 0;
	int guideDone = 0;
	Stack *stack = newSlack(20);
	if (!stack) {
		return -1;
	}
	pushData(stack, GUIDE_INDEX_LANG);
	if (Global_Active_step == 1) {
		pushData(stack, GUIDE_DEVICE_ACTIVE);
	} else if (Global_Active_step == 2) {
		Global_Active_step = 0;
		pushData(stack, GUIDE_INDEX_KEY_INSTRUCTION);
	} else {
		Global_Active_step = 0;
	}

	const char *picName = NULL;
	do {
		ret = 0;
		nextIndex = getStackTop(stack);
		Global_Guide_Index = nextIndex;

		switch (nextIndex) {
			case GUIDE_INDEX_LANG: {
				ret = setupLang();
				if (ret == 0) {
					if (device_get_active_time() == 0) {
						pushData(stack, GUIDE_DEVICE_ACTIVE);
					} else {
						pushData(stack, GUIDE_INDEX_KEY_INSTRUCTION);
					}
				}
			}
				break;
			case GUIDE_DEVICE_ACTIVE: {
				popData(stack);
				if (device_get_active_time() == 0) {
					ret = activeDevice();
					if (ret == 0) {
						pushData(stack, GUIDE_INDEX_KEY_INSTRUCTION);
					} else if (ret == 1) { //jump qr
						db_error("jump to qr scan,break");
						return 0;
					}
				} else {
					pushData(stack, GUIDE_INDEX_KEY_INSTRUCTION);
				}
			}
				break;
			case GUIDE_INDEX_KEY_INSTRUCTION: {
				ret = picDialog(mHwnd, "key_instruction", NULL, NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (is_key_event_value(ret) || ret >= 0) {
					pushData(stack, GUIDE_INDEX_GREET);
				}
			}
				break;
			case GUIDE_INDEX_GREET: {
				picName = "greeting";
				ret = picDialog(mHwnd, picName, res_getLabel(LANG_LABEL_START_USE), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret >= 0) {
					pushData(stack, GUIDE_INDEX_ACCOUNT_OP_SELECT);
				}
			}
				break;
			case GUIDE_INDEX_ACCOUNT_OP_SELECT: {
				ret = dialog(mHwnd,
				             NULL,
				             DIALOG_ICON_STYLE_NONE,
				             NULL,
				             DIALOG_BUTTON_ALIGN_CENTER,
				             res_getLabel(LANG_LABEL_RECOVERY_ACCOUNT),
				             res_getLabel(LANG_LABEL_CREATE_ACCOUNT),
				             1);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					recovery_flag = ret ? 0 : 1;
					pushData(stack, recovery_flag ? GUIDE_INDEX_MNEMONICS_CNT : GUIDE_INDEX_MNEMONICS_TIPS);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_TIPS: {
				picName = "word_create_tips";
				ret = picDialog(mHwnd, picName, res_getLabel(LANG_LABEL_CREATE_NOW), res_getLabel(LANG_LABEL_MNEMONIC_IS_WHAT), 1);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					pushData(stack, ret ? GUIDE_INDEX_MNEMONICS_DETAIL_TIPS : GUIDE_INDEX_MNEMONICS_CNT);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_DETAIL_TIPS: {
				picName = "what_is_mnemonic_phase";
				ret = picDialogFlag(mHwnd, picName, res_getLabel(LANG_LABEL_KNOWN_AND_CREATE), NULL, 0, PIC_DLG_FLAG_MULTI_PIC);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					pushData(stack, GUIDE_INDEX_MNEMONICS_CNT);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_CNT: {
				ret = selectMnemonicCnt();
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret > 0) {
					memzero(mIndexes, sizeof(mIndexes));
					memzero(randomBuf, sizeof(randomBuf));
					memzero(mnenonics, sizeof(mnenonics));
					mlen = ret;
					if (device_get_hw_break_state(3) != 0) {
						dialog_error3(mHwnd, -801, "The device is broken, please contact the SafePal team for help.");
						break;
					}
					if (!device_is_inited()) {
						dialog_error3(mHwnd, -802, "Init failed.");
						break;
					}
					if (clean_data() != 0) {
						dialog_error3(mHwnd, -803, "Init failed.");
						break;
					}
					pushData(stack, GUIDE_INDEX_CHECK_SECHIP);
				} else {
					break;
				}
			}
				break;
			case GUIDE_INDEX_CHECK_SECHIP: {
				popData(stack);
				ret = checkSechip(0);
				if (ret != 0) {
					db_error("check sechip false ret:%d", ret);
					ret = 0;//force reset ret
				} else {
					db_msg("check sechip OK");
					pushData(stack, recovery_flag ? GUIDE_INDEX_ACCOUNT_RECOVERY_TIPS : GUIDE_INDEX_MNEMONICS_GENERATE);
				}
			}
				break;
			case GUIDE_INDEX_ACCOUNT_RECOVERY_TIPS: {
				picName = "recovery_word_tips";
				ret = picDialog(mHwnd, picName, res_getLabel(LANG_LABEL_RECOVER_AT_ONCE), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					pushData(stack, GUIDE_INDEX_ACCOUNT_RECOVERY);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_GENERATE: {
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				memzero(mIndexes, sizeof(mIndexes));
				memzero(randomBuf, sizeof(randomBuf));
				memzero(mnenonics, sizeof(mnenonics));
				ret = generateMnemonic(mIndexes, mlen, randomBuf);
				if (ret < 0) {
					break;
				} else if (ret == 0) {
					popData(stack);
					pushData(stack, GUIDE_INDEX_MNEMONICS_SHOW);
                    settings_save(SETTING_KEY_ACCOUNT_TYPE, ACCOUNT_TYPE_NEW_GEN);
                }
			}
				break;
			case GUIDE_INDEX_MNEMONICS_SHOW: {
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				ret = showMnemonic(mIndexes, mlen);
				if (ret == KEY_EVENT_BACK) {
					ret = showMnemonicsWillChangeAlert();
					if (ret == KEY_EVENT_BACK) {
						popData(stack);
					} else if (ret < 0) {
						break;
					}
				} else if (ret < 0) {
					break;
				} else {
					pushData(stack, GUIDE_INDEX_MNEMONICS_CONFIRM_TIPS);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_CONFIRM_TIPS: {
				picName = "word_confirm_tips";
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				ret = picDialog(mHwnd, picName, res_getLabel(LANG_LABEL_NEXT_STEP), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					break;
				} else {
					pushData(stack, GUIDE_INDEX_MNEMONICS_CONFIRM);
				}
			}
				break;
			case GUIDE_INDEX_MNEMONICS_CONFIRM: {
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				ret = confirmMnemonic(mIndexes, mlen);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					db_error("confirm menemonic error ret:%d", ret);
					break;
				} else {
					pushData(stack, GUIDE_INDEX_PASSWD_TIPS);
				}
			}
				break;
			case GUIDE_INDEX_PASSWD_TIPS: {
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				ret = showPasswdTips();
				if (ret == KEY_EVENT_BACK) {
					ret = picDialog(mHwnd, "re_confirm_word_phase_tips", res_getLabel(LANG_LABEL_SUBMENU_OK), res_getLabel(LANG_LABEL_SUBMENU_CANCEL), 1);
					if (!ret) {
						popData(stack);
					}
				} else if (ret < 0) {
					db_error("show dialog passwd tips error ret:%d", ret);
					break;
				} else {
					pushData(stack, GUIDE_INDEX_ENTER_PASSWD);
				}
			}
				break;
			case GUIDE_INDEX_ENTER_PASSWD: {
				set_temp_screen_time(DEFAULT_HI_SCREEN_SAVER_TIME);
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				unsigned char passhash[PASSWD_HASHED_LEN] = {0};
				unsigned char seedbin[64] = {0};
				ret = enterPassword(passhash);
				if (ret == KEY_EVENT_ABORT) {
					popData(stack);
				} else if (ret < 0) {
					memzero(passhash, sizeof(passhash));
					break;
				} else {
					if (recovery_flag) {
					} else {
						if (buffer_is_zero(randomBuf, randomBufSize(mlen))) {
							db_serr("random is zero");
							break;
						}
						memzero(mnenonics, sizeof(mnenonics));
						if (mnemonic_words_from_data(randomBuf, randomBufSize(mlen), mnenonics) != mlen) {
							db_serr("invalid random");
							break;
						}
					}
					if (mnenonics[0] == 0) {
						db_secure("empty Mnemonics");
						break;
					}
					if (!mnemonic_check(mnenonics)) {
						db_serr("invalid Mnemonics:%s", mnenonics);
						break;
					}
					mnemonic_to_seed(mnenonics, "", seedbin, NULL);
					db_secure("Mnemonics: %s", mnenonics);
					db_secure("seed  hex: %s", debug_ubin_to_hex(seedbin, 64));
					loading_win_start(mHwnd, res_getLabel(LANG_LABEL_INIT_WALLET_ING), res_getLabel(LANG_LABEL_INIT_WALLET_TIPS), 0);
					ret = wallet_storeSeed(seedbin, 64, passhash);
                    db_secure("save seed ret:%d", ret);
					if (ret == 0) {
						loading_win_refresh();
						ret = wallet_verify_mnemonic((const unsigned char *) mnenonics, strlen(mnenonics), passhash);
                        memzero(mnenonics, sizeof(mnenonics));
                        db_secure("verify mnemonic ret:%d", ret);
						if (ret != 0) {
                            ret = -500 + ret;
							settings_set_have_seed(0);
						}
					}
					if (ret == 0) {
						loading_win_refresh();
						ret = wallet_verify_seed_xpub(seedbin, 64);
						db_secure("verify seed ret:%d", ret);
						if (ret != 0) {
                            ret = -600 + ret;
							settings_set_have_seed(0);
						}
					}
					loading_win_stop();
					memzero(passhash, sizeof(passhash));
					memzero(seedbin, sizeof(seedbin));

					if (ret) {
						break;
					} else {
						GLobal_PIN_Passed = 1;
						storage_syncData2Disk();
						pushData(stack, GUIDE_INDEX_WALLET_NAME_TIPS);
					}
				}
			}
				break;
			case GUIDE_INDEX_WALLET_NAME_TIPS: {
				if (!recovery_flag) {
					picName = "name_wallet_create_tips";
				} else {
					picName = "name_wallet_recovery_tips";
				}
				ret = picDialog(mHwnd, picName, res_getLabel(LANG_LABEL_NAME_WALLET), NULL, 0);
				if (ret == KEY_EVENT_BACK) {
				} else if (ret < 0) {
					db_error("confirm backup word error ret:%d", ret);
				} else {
					pushData(stack, GUIDE_INDEX_INPUT_WALLET_NAME);
				}
			}
				break;
			case GUIDE_INDEX_INPUT_WALLET_NAME: {
				ret = saveWalletName();
				if (ret == KEY_EVENT_ABORT) {
					popData(stack);
				} else if (ret < 0) {
					db_error("input wallet name failed ret:%d", ret);
					break;
				} else {
					pushData(stack, GUIDE_INDEX_DOWNLOAD_APP_TIPS);
				}
			}
				break;
			case GUIDE_INDEX_DOWNLOAD_APP_TIPS: {
				ret = picDialog(mHwnd, "download_app_qr", NULL, NULL, 0);
				nextIndex = GUIDE_INDEX_MAX;
				guideDone = 1;
			}
				break;
			case GUIDE_INDEX_ACCOUNT_RECOVERY: {
				if (!IS_VALID_MNEMONIC_LEN(mlen)) {
					break;
				}
				int size = MNEMONIC_MAX_LEN * mlen;
				memzero(mIndexes, sizeof(mIndexes));
				memzero(randomBuf, sizeof(randomBuf));
				memzero(mnenonics, sizeof(mnenonics));
				ret = enterRecoveryWord(mnenonics, size, mlen);
				if (ret == KEY_EVENT_BACK) {
					popData(stack);
				} else if (ret < 0) {
					memzero(mnenonics, size);
					db_error("recovery account failed!");
					break;
				} else if (ret == 0) { //force check == 0
					pushData(stack, GUIDE_INDEX_PASSWD_TIPS);
                    settings_save(SETTING_KEY_ACCOUNT_TYPE, ACCOUNT_TYPE_RECOVERY);
                }
			}
				break;
			default:
				break;
		}

		set_temp_screen_time(DEFAULT_SCREEN_SAVER_TIME);
		if (!is_key_event_value(ret) && ret < 0) {
			const char *format = (res_getLabel(LANG_LABEL_WALLET_INIT_FAILED_TIPS));
			int code = getErrorCode(nextIndex, ret);
			snprintf(tmpbuf, sizeof(tmpbuf), format, code);
			dialog_error2(mHwnd, tmpbuf, res_getLabel(LANG_LABEL_RETRY));
			nextIndex = GUIDE_INDEX_LANG;
		}

	} while (nextIndex != GUIDE_INDEX_MAX);

	memzero(mIndexes, sizeof(mIndexes));
	memzero(randomBuf, sizeof(randomBuf));
	memzero(mnenonics, sizeof(mnenonics));

	freeSlack(stack);
	if (guideDone) {
		changeWindow(WINDOWID_MAINPANEL);
	}
	return 0;
}
