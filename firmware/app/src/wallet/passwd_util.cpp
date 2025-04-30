#define LOG_TAG "wallet_util"

#include <math.h>
#include <crypto/curves.h>
#include "common.h"
#include "passwd_util.h"
#include "secure_api.h"
#include "sha2.h"
#include "sha3.h"
#include "resource.h"
#include "Dialog.h"
#include "bip39_english.h"
#include "key_event.h"
#include "GuiMain.h"
#include "wallet_manager.h"
#include "device.h"
#include "pbkdf2.h"
#include "PicDialog.h"
#include "rand.h"
#include "global.h"

static int check_input_passwd(const char *passwd, int len) {
	if (!passwd) {
		db_error("invalid paras null");
		return -1;
	}
	if (len != (int) strlen(passwd)) {
		db_error("invalid len:%d strlen:%d", len, strlen(passwd));
		return -1;
	}
	if (len < PASSWORD_MINI_LEN) {
		return USER_PASSWD_ERR_FORMAT;
	}
	if (len > PASSWORD_MAX_LEN) {
		return USER_PASSWD_ERR_FORMAT;
	}
	char n0 = *passwd;
	if (len == 6) {
		if (n0 == '1') {
			if (!strcmp(passwd, "123456")) return USER_PASSWD_ERR_WEAK;
			if (!strcmp(passwd, "123654")) return USER_PASSWD_ERR_WEAK;
			if (!strcmp(passwd, "111222")) return USER_PASSWD_ERR_WEAK;
		}
		if (n0 == '6' && !strcmp(passwd, "654321")) return USER_PASSWD_ERR_WEAK;
	}

	int i;
	for (i = 1; i < len; i++) {
		if (passwd[i] != n0) {
			break;
		}
	}
	if (i == len) { //all same
		return USER_PASSWD_ERR_WEAK;
	}

	for (i = 1; i < len; i++) {
		if (passwd[i] != n0 + i) {
			break;
		}
	}
	if (i == len) { //all ++
		return USER_PASSWD_ERR_WEAK;
	}

	for (i = 1; i < len; i++) {
		if (passwd[i] != n0 - i) {
			break;
		}
	}
	if (i == len) { //all --
		return USER_PASSWD_ERR_WEAK;
	}
	return 0;
}

static inline int ser_uint32(unsigned char *buf, uint32_t n) {
	buf[3] = (unsigned char) n;
	buf[2] = (unsigned char) (n >> 8);
	buf[1] = (unsigned char) (n >> 16);
	buf[0] = (unsigned char) (n >> 24);
	return sizeof(uint32_t);
}

//hash user original passwd for its sec chip
int hash_user_passwd(const char *passwd, int len, unsigned char hash[PASSWD_HASHED_LEN]) {
	int ret = 0;

	unsigned char digest[SHA256_DIGEST_LENGTH] = {0};
	const sec_base_info *binfo = wallet_getBaseInfo();
	if (!binfo || !binfo->chipid_len || !binfo->app_version || !binfo->chip_type) {
		ALOGE("get sec base info false");
		return -1;
	}
	ret = device_get_diviceid((char *) digest, sizeof(digest));
	if (ret <= 0) {
		ALOGE("get device info false");
		return -1;
	}
#ifdef DB_DEBUG
	long long t0 = getMsClockTime();
#endif
	unsigned char salt[] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31};
	//ALOGD("salt:%s", debug_ubin_to_hex(salt, sizeof(salt)));

	SHA256_CTX context;
	sha256_Init(&context);
	sha256_Update(&context, digest, ret); //device id
	sha256_Update(&context, (const unsigned char *) passwd, len);
	sha256_Update(&context, (const unsigned char *) &(binfo->chip_type), sizeof(binfo->chip_type));
	sha256_Update(&context, (const unsigned char *) &(binfo->app_version), sizeof(binfo->app_version));
	sha256_Update(&context, binfo->chipid, binfo->chipid_len);
	sha256_Update(&context, salt, sizeof(salt));
	memset(digest, 0, sizeof(digest));
	sha256_Final(&context, digest);
	//db_secure("digest 0:%s", debug_ubin_to_hex(digest, SHA256_DIGEST_LENGTH));
	pbkdf2_hmac_sha256((const unsigned char *) passwd, len, digest, SHA256_DIGEST_LENGTH, 2048, digest, SHA256_DIGEST_LENGTH);
	//db_secure("digest 1:%s", debug_ubin_to_hex(digest, SHA256_DIGEST_LENGTH));
	memcpy(hash, digest, PASSWD_HASHED_LEN);
	db_secure("hash:%s", debug_ubin_to_hex(hash, PASSWD_HASHED_LEN));
	memzero(digest, sizeof(digest));
	memzero(salt, sizeof(salt));
	memzero(&binfo, sizeof(binfo));
	db_debug("use time:%lld ms", getMsClockTime() - t0);
	return PASSWD_HASHED_LEN;
}

USER_PASSWD_ERR passwdKeyboard(HWND hParent, const char *title, const int opType, unsigned char passhash[PASSWD_HASHED_LEN], unsigned int flag) {
	if (!IS_VALID_HWND(hParent)) {
		db_error("invalid hwmd:%d", hParent);
		return USER_PASSWD_ERR_INVALID_PARAS;
	}
	if (NULL == passhash) {
		db_error("invalid paras passhash");
		return USER_PASSWD_ERR_INVALID_PARAS;
	}
	char kb_result[PASSWORD_MAX_LEN + 1] = {0};
	int ret = 0;
	int err = 0;
	int passwdLen = 0;
	char str[64] = {0};
	KeyBoardConfig_t kb;
	memset(&kb, 0, sizeof(KeyBoardConfig_t));
	kb.result = (char *) kb_result;
	kb.result_limit = PASSWORD_MAX_LEN;
    if (gSettings->mRandPinKeypad == 0) {//ON
        kb.random = (flag & PASSKB_FLAG_RANDOM) ? 1 : 0;
    } else {
        kb.random = 0;
    }
	kb.keyStyle = KEYBOARD_STYLE_PIN;
	kb.editor_style = KEYBOARD_EDITOR_HIDECODE;
	kb.text_field.data = "";
    kb.initKeyIndex = get_random_keyboard_index(kb.keyStyle);
    if (NULL == title) {
		kb.title.data = res_getLabel(LANG_LABEL_ENTER_PASSWD);
	} else {
		kb.title.data = title;
	}

	do {
		memset(kb_result, 0, sizeof(kb_result));
		passwdLen = showKeyBoard(hParent, &kb);
		db_msg("enter len:%d", passwdLen);
		if (passwdLen == KEY_EVENT_ABORT) {
			memset(kb_result, 0, sizeof(kb_result));
			return USER_PASSWD_ERR_ABORT;
		} else if (passwdLen < 0) {
			db_error("passwd input error ret:%d", passwdLen);
			memset(kb_result, 0, sizeof(kb_result));
			dialog_error3(hParent, passwdLen, "Password error.");
			err = 1;
			break;
		}

		if (opType == PIN_CODE_VERITY) {
			ret = hash_user_passwd((const char *) kb_result, passwdLen, passhash);
			if (ret < 0) {
				memset(kb_result, 0, sizeof(kb_result));
				memzero(passhash, PASSWD_HASHED_LEN);
				dialog_error3(hParent, ret, "Password error.");
				continue;
			}
			ret = sapi_check_passwd(passhash, PASSWD_HASHED_LEN);
			int subcode = sapi_subcode;
			if (ret == 0) {
				GLobal_PIN_Passed = 1;
				if (flag & PASSKB_FLAG_RAW_PASSWD) {
					memzero(passhash, PASSWD_HASHED_LEN);
					memcpy(passhash, kb_result, PASSWORD_MAX_LEN);
				}
				memset(kb_result, 0, sizeof(kb_result));
				return USER_PASSWD_ERR_NONE;
			}
			memset(kb_result, 0, sizeof(kb_result));
			memzero(passhash, PASSWD_HASHED_LEN);
			switch (ret) {
				case ERROR_SERVICE_DENY:
				case ERROR_PASSWD_ERROR_MUCH: {
					dialog_error(hParent, res_getLabel(LANG_LABEL_PASSWD_ERR_TOO_MUCH));
					GLobal_PIN_Passed = 0;
					wallet_destorySeed(0);
					if (!(flag & PASSKB_FLAG_NOT_SWITCH_GUIDE)) {
						GuiMain::getInstance()->changeWindow(WINDOWID_GUIDE);
					}
					return USER_PASSWD_ERR_VERIFY;
				}
					break;
				case ERROR_PASSWD_NO_MATCH: {
					char state = (subcode >> 8) & 0XFF;
					char errTimes = subcode & 0xFF;
					db_msg("err times:%d state:%d", errTimes, state);
					int retain = wallet_getPasswdAllowErrorTimes() - errTimes;
					if (retain <= 0) {
						dialog_error(hParent, res_getLabel(LANG_LABEL_PASSWD_ERR_TOO_MUCH));
						GLobal_PIN_Passed = 0;
						wallet_destorySeed(0);
						if (!(flag & PASSKB_FLAG_NOT_SWITCH_GUIDE)) {
							GuiMain::getInstance()->changeWindow(WINDOWID_GUIDE);
						}
						return USER_PASSWD_ERR_VERIFY;
					} else {
						if (retain <= 3) {
							snprintf(str, sizeof(str), res_getLabel(LANG_LABEL_PASSWD_ERROR), retain);
						} else {
							snprintf(str, sizeof(str), "%s", res_getLabel(LANG_LABEL_PASSWD_ERROR_SIMPLE));
						}
						dialog_error2(hParent, str, res_getLabel(LANG_LABEL_TRY_AGAIN));
					}
				}
					break;
				default: {
					dialog_error3(hParent, sapi_subcode, "Password error");
					err = 1;
				}
					break;
			}
		} else {
			if (opType == PIN_CODE_CHECK) {
				ret = check_input_passwd((const char *) kb_result, passwdLen);
				db_secure("check passwd ret:%d", ret);
				if (ret != 0) {
					switch (ret) {
						case USER_PASSWD_ERR_WEAK: {
							picDialog(hParent, "weak_passwd_tips", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
						}
							break;
						case USER_PASSWD_ERR_FORMAT:
						case USER_PASSWD_ERR_NOT_INPUT: {
							picDialog(hParent, "pin_format_err", res_getLabel(LANG_LABEL_RE_ENTER), NULL, 0);
						}
							break;
						default:
							err = 1;
							break;
					}
					continue;
				}
			}
			ret = hash_user_passwd((const char *) kb_result, passwdLen, passhash);
			if (ret < 0) {
				memset(kb_result, 0, sizeof(kb_result));
				memzero(passhash, PASSWD_HASHED_LEN);
				dialog_error3(hParent, ret, "Password error");
				db_error("hash user passwd false ret:%d", ret);
				return USER_PASSWD_ERR_SYSTEM;
			}
			if (flag & PASSKB_FLAG_RAW_PASSWD) {
				memzero(passhash, PASSWD_HASHED_LEN);
				memcpy(passhash, kb_result, PASSWORD_MAX_LEN);
			}
			memset(kb_result, 0, sizeof(kb_result));
			return USER_PASSWD_ERR_NONE;
		}
	} while (!err);
	memset(kb_result, 0, sizeof(kb_result));
	memzero(passhash, PASSWD_HASHED_LEN);
	return USER_PASSWD_ERR_SYSTEM;
}

USER_PASSWD_ERR checkPasswdKeyboard(HWND hParent, const char *title, unsigned int flag) {
	unsigned char passhash[PASSWD_HASHED_LEN];
	USER_PASSWD_ERR ret = passwdKeyboard(hParent, title, PIN_CODE_VERITY, passhash, flag | PASSKB_FLAG_RANDOM);
	memzero(passhash, sizeof(passhash));
	return ret;
}
