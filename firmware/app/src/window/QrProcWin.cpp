#define LOG_TAG "QrProcWin"

#include <stdlib.h>
#include <qr_pack.h>
#include <crypto/ecdsa.h>
#include "QrProcWin.h"
#include "GuiMain.h"
#include "debug.h"
#include "device.h"
#include "wallet_proto.h"
#include "storage_manager.h"
#include "secp256k1.h"
#include "rand.h"
#include "wallet_util.h"
#include "secure_api.h"
#include "coin_util.h"
#include "wallet_manager.h"
#include <crypto/bip32.h>
#include <curves.h>
#include "loading_win.h"
#include "PicDialog.h"
#include "active_util.h"
#include "fileutil.h"
#include "secure_util.h"

QrProcWin::QrProcWin() {
	mMessage = NULL;
	mQrErrorCode = 0;
}

QrProcWin::~QrProcWin() {

}

PROC_RET QrProcWin::winProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case MSG_QR_RESULT:
			onQrResult(wParam, (ProtoClientMessage *) lParam);
			break;
		case MSG_QR_ERROR:
			mQrErrorCode = wParam;
			break;
		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

int QrProcWin::onQrError(int error) {
	db_debug("error:%d", error);
	const char *tips = NULL;
	if (error == QR_DECODE_ACCOUNT_MISMATCH) {
		tips = res_getLabel(LANG_LABEL_ACCOUNT_MISMATCH);
	} else if (error == QR_DECODE_UNSUPPORT_MSG) {
		tips = res_getLabel(LANG_LABEL_UNSUPPORT_MSG);
	} else if (error == QR_DECODE_INVALID_MSG) {
		tips = res_getLabel(LANG_LABEL_UNSUPPORT_MSG_S);
	} else {
		dialog_system_error(mHwnd, error);
	}
	if (is_not_empty_string(tips)) {
		dialog_l(mHwnd, NULL, DIALOG_ICON_STYLE_ERR, tips, DIALOG_BUTTON_ALIGN_VERT, res_getLabel(LANG_LABEL_SUBMENU_OK), NULL, 0);
	}
	changeWindow(WINDOWID_MAINPANEL);
	return 0;
}

int QrProcWin::onQrResult(WPARAM wParam, ProtoClientMessage *msg) {
	if (mMessage != NULL) {
		proto_client_message_delete(mMessage);
		mMessage = NULL;
	}
	mMessage = msg;
	db_msg("type:0x%02X client:%d", msg->type, msg->client_id);
	return 0;
}

int QrProcWin::onResume() {
	int ret = 1;

	if (mQrErrorCode) {
		ret = mQrErrorCode;
		mQrErrorCode = 0;
		onQrError(ret);
		return 0;
	}

	if (!mMessage) {
		db_error("msg is NULL");
		return changeWindow(WINDOWID_MAINPANEL);
	}
	gProcessing = 1;
	switch (mMessage->type) {
		case QR_MSG_BIND_ACCOUNT_REQUEST:
			ret = confirmBindAccount();
			break;
		case QR_MSG_GET_PUBKEY_REQUEST:
			ret = confirmGetPubkey();
			break;
		case QR_MSG_USER_ACTIVE:
			ret = procUserActive();
			break;
		default:
			db_error("unkown msg:%d", mMessage->type);
			changeWindow(WINDOWID_MAINPANEL);
	}
	gProcessing = 0;
	Global_QR_Proc_Result = ret;
	db_msg("proce ret:%d", ret);
	return 0;
}

int QrProcWin::onPause() {
	if (mMessage != NULL) {
		proto_client_message_delete(mMessage);
		mMessage = NULL;
	}
	return 0;
}

int QrProcWin::keyProc(int keyCode, int isLongPress) {
	db_msg("code:%d gProcessing:%d", keyCode, gProcessing);
	if (gProcessing) {
		return 0;
	}

	switch (keyCode) {
		case INPUT_KEY_OK:
		case INPUT_KEY_LEFT:
			return WINDOWID_MAINPANEL;
	}
	return 0;
}

static int check_client_uniq_id(const char *client_unique_id) {
	if (is_empty_string(client_unique_id)) {
		db_error("empty uniqid");
		return -1;
	}
	size_t len = strlen(client_unique_id);
	if (len > CLIENT_UNIQID_MAX_LEN) {
		db_error("too long uniqid:%d -> %s", len, client_unique_id);
		return -2;
	}
	if (strchr(client_unique_id, '\'')) {
		db_error("invalid char uniqid:%d -> %s", len, client_unique_id);
		return -3;
	}
	return 0;
}

int QrProcWin::genDeviceInfo(struct pbc_wmessage *device, int type) {
	char tmpbuf[128];
	memset(tmpbuf, 0, sizeof(tmpbuf));
	int ret = device_get_id(tmpbuf, 32);
	db_msg("deviceid:%d -> %s", ret, tmpbuf);
	pbc_wmessage_string(device, "id", tmpbuf, 0);

	memset(tmpbuf, 0, 32);
	get_device_name(tmpbuf, 32, 0);
	db_msg("name:%s", tmpbuf);
	pbc_wmessage_string(device, "name", tmpbuf, 0);

	memset(tmpbuf, 0, 32);
	if (wallet_getAccountSuffix(tmpbuf) > 0) {
		pbc_wmessage_string(device, "account_suffix", tmpbuf, 0);
	}

	pbc_wmessage_integer(device, "version", DEVICE_APP_INT_VERSION, 0);
	pbc_wmessage_integer(device, "se_version", SECHIP_APP_VERSION, 0);

	if (type == 0) {
		memset(tmpbuf, 0, 32);
		device_get_sn(tmpbuf, 32);
		pbc_wmessage_string(device, "product_sn", tmpbuf, 0);
		pbc_wmessage_string(device, "product_series", PRODUCT_SERIES_VALUE, 0);
		pbc_wmessage_string(device, "product_type", PRODUCT_TYPE_VALUE, 0);
		pbc_wmessage_string(device, "product_name", PRODUCT_NAME_VALUE, 0);
		pbc_wmessage_string(device, "product_brand", PRODUCT_BRAND_VALUE, 0);
		UserActiveInfo info;
		if (device_get_user_active_info(&info) == 0) {
			pbc_wmessage_integer(device, "active_time", (uint32_t) info.time, 0);
			pbc_wmessage_integer(device, "active_time_zone", (uint32_t) info.time_zone, 0);
		}
	}
	return 0;
}

int QrProcWin::confirmBindAccount() {
	db_msg("start");
	int client_id;
	curve_point pub;

	int ret;
	char tmpbuf[128];
	const ecdsa_curve *curve = &secp256k1;
	int isBack = 0;
	int err = 0;

	BindAccountReq req;
	if (proto_rmsg_BindAccountReq(mMessage->rmsg, &req) != 0) {
		db_error("decode BindAccountReq false");
		return -1;
	}

	const char *client_unique_id = req.client_unique_id;
	const char *client_name = req.client_name;
	const unsigned char *sec_random = req.sec_random.bytes;
	int bind_version = req.version;
	db_msg("req bind_version:%d client_unique_id:%s client_name:%s random:%d -> %s", bind_version, client_unique_id, client_name,
	       req.sec_random.size, debug_ubin_to_hex(sec_random, req.sec_random.size));

	int errcode = 0;
	if (is_empty_string(client_name)) { //invalid req
		errcode |= 0x1;
	}
	if (check_client_uniq_id(client_unique_id) != 0) {
		errcode |= 0x2;
	}
	if ((req.sec_random.size == 65 && sec_random[0] == 0x4) || (req.sec_random.size == 33 && (sec_random[0] == 0x2 || sec_random[0] == 0x3))) {
		ret = ecdsa_read_pubkey(curve, sec_random, &pub);
		if (ret != 1) {
			db_error("invalid random pubkey");
			errcode |= 0x8;
		}
	} else {
		errcode |= 0x4;
	}

	if (errcode) {
		db_error("invalid request errcode:%d", errcode);
		dialog_error3(mHwnd, errcode, "Pair failed.");
		return 0;
	}

	struct pbc_wmessage *wmsg = NULL;
	struct pbc_wmessage *wmsg_wrapper = NULL;
	unsigned char passhash[PASSWD_HASHED_LEN] = {0};

	do {
		ret = picDialog(mHwnd, "bind_alert", res_getLabel(LANG_LABEL_TXT_CANCEL), res_getLabel(LANG_LABEL_TXT_OK), 1);
		db_msg("dialog_confirm ret:%d", ret);
		if (ret == 0 || ret == KEY_EVENT_BACK) {
			isBack = 1;
			break;
		} else if (ret < 0) {
			err = 1;
			break;
		}
		ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, 1);
		if (ret == KEY_EVENT_ABORT) {
			continue;
		} else if (ret < 0) {
			memzero(passhash, PASSWD_HASHED_LEN);
			db_error("input passwd ret:%d", ret);
			err = 1;
			break;
		} else {
			break;
		}
	} while (1);
	if (err || isBack) {
		changeWindow(WINDOWID_MAINPANEL);
		return err ? -1 : 0;
	}

	do {
		ClientInfo client;
		memset(&client, 0, sizeof(ClientInfo));
		client_id = storage_queryClientId(client_unique_id);
		if (client_id > 0) {
			db_error("unique_id:%s have binded client_id:%d", client_unique_id, client_id);
			if (storage_getClientInfo(client_id, &client) != 0) {
				db_error("get client info false client_id:%d", client_id);
				memset(&client, 0, sizeof(ClientInfo));
				client_id = 0;
			}
		} else {
			client_id = 0;
		}
		unsigned char myrandkey[32];
		unsigned char *seckey = myrandkey; //reuse buffer
		bignum256 k;
		memset(myrandkey, 0, sizeof(myrandkey));
		memset(tmpbuf, 0, sizeof(tmpbuf));
		ret = device_get_cpuid(tmpbuf, sizeof(tmpbuf));
		if (ret > 32) {
			tmpbuf[32] = 0;
		}
		char salt[] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0};
		strcat(tmpbuf, salt);
		strcat(tmpbuf, client_unique_id);
		ret = strlen(tmpbuf);
		db_debug("myrandkey init buffer:%d -> %s", ret, tmpbuf);
		sha256_Raw((const uint8_t *) tmpbuf, (size_t) ret, myrandkey);
		db_debug("myrandkey R1:%s", debug_ubin_to_hex(myrandkey, 32));
		do {
			sha256_Raw(myrandkey, sizeof(myrandkey), myrandkey);
			db_debug("myrandkey:%s", debug_ubin_to_hex(myrandkey, 32));
			bn_read_be(myrandkey, &k);
		} while (bn_is_zero(&k) || !bn_is_less(&k, &curve->order));

		// compute k*pub -> pub
		point_multiply(curve, &k, &pub, &pub);
		memset(seckey, 0, 32);
		bn_write_be(&pub.x, seckey);
		if (client_id > 0 && memcmp(client.seckey, seckey, 32) == 0) {
			db_msg("same seckey:%s old clientid:%d", debug_ubin_to_hex(seckey, 32), client_id);
		} else {
			db_msg("new seckey:%s new client time:%d time_zone:%d", debug_ubin_to_hex(seckey, 32), mMessage->time, mMessage->time_zone);
			memcpy(client.seckey, seckey, 32);
			client.bind_time = mMessage->time + mMessage->time_zone;
			strncpy(client.unique_id, client_unique_id, CLIENT_UNIQID_MAX_LEN);
			strncpy(client.client_name, client_name, CLIENT_NAME_MAX_LEN);
			client_id = storage_saveClientInfo(&client);
			if (client_id <= 0) {
				db_error("save client ret:%d", client_id);
                dialog_error3(mHwnd, -1000 + client_id, "Pair failed.");
                break;
			}
		}

		//gen my curve_point
		// compute k*G -> R
		unsigned char *seckeybuff = (unsigned char *) tmpbuf; //reuse tmpbuf
		point_multiply(curve, &k, &curve->G, &pub);
		seckeybuff[0] = 4;
		bn_write_be(&pub.x, seckeybuff + 1);
		bn_write_be(&pub.y, seckeybuff + 33);

		db_msg("my pubkey:%s", debug_ubin_to_hex(seckeybuff, 65));

		wmsg = proto_new_wmessage("Wallet.BindAccountResp");
		if (!wmsg) {
            dialog_error3(mHwnd, -2000, "Pair failed.");
            break;
		}
		if (bind_version > 0) {
			wmsg_wrapper = proto_new_wmessage("Wallet.BindAccountRespWrapper");
		} else {
			wmsg_wrapper = wmsg;
		}

		pbc_wmessage_string(wmsg_wrapper, "client_unique_id", client_unique_id, 0);
		pbc_wmessage_integer(wmsg, "client_id", (uint32_t) client_id, 0);
		pbc_wmessage_string(wmsg_wrapper, "sec_random", (const char *) seckeybuff, 65);

		struct pbc_wmessage *device = pbc_wmessage_message(wmsg, "device_info");
		if (!device) {
            dialog_error3(mHwnd, -3000, "Pair failed.");
			break;
		}
		ret = genDeviceInfo(device, 0);
		//pbc_wmessage_string(device, "active_code", "", 0);
		syncAllCoins(wmsg, passhash, mMessage->rmsg);
		pbc_wmessage_integer(wmsg, "version", (uint32_t) bind_version, 0);
		if (wmsg_wrapper != wmsg) {
			pbc_wmessage_integer(wmsg_wrapper, "version", (uint32_t) bind_version, 0);
		}

		uint64_t account_id = wallet_AccountId();
		db_msg("account_id:%llx", account_id);
		pbc_wmessage_integer(wmsg, "account_id", (uint32_t) account_id, 0);
		if (account_id) {
			memset(tmpbuf, 0, 8);
			wallet_getAccountSuffix(tmpbuf);
			pbc_wmessage_string(wmsg, "account_suffix", tmpbuf, 0);
		}
        pbc_wmessage_integer(wmsg, "account_type", (uint32_t) gSettings->mAccountType, 0);

		struct pbc_slice result;
		pbc_wmessage_buffer(wmsg, &result);
		debug_show_long_bin_data("account qr:", (const unsigned char *) result.buffer, result.len);
		if (wmsg_wrapper != wmsg) {
			unsigned char *encode_buff = (unsigned char *) malloc(result.len);
			if (!encode_buff) {
                dialog_error3(mHwnd, -4000, "Pair failed.");
				break;
			}
			unsigned char *digest = myrandkey; //reuse
			sha256_Raw((const unsigned char *) result.buffer, result.len, digest);
			db_msg("encoded_digest:%s", debug_ubin_to_hex(digest, 8));
			if (aes256_encrypt((const unsigned char *) result.buffer, encode_buff, result.len, client.seckey) != 0) {
				free(encode_buff);
                dialog_error3(mHwnd, -5000, "Pair failed.");
				break;
			}
			pbc_wmessage_string(wmsg_wrapper, "encoded_info", (const char *) encode_buff, result.len);
			pbc_wmessage_string(wmsg_wrapper, "encoded_digest", (const char *) digest, 8);
			free(encode_buff);
			pbc_wmessage_buffer(wmsg_wrapper, &result);
		}
		memset(&client, 0, sizeof(ClientInfo)); //clean it
		showQRWindow(mHwnd, 0, mMessage->flag, QR_MSG_BIND_ACCOUNT_RESP, (const unsigned char *) result.buffer, result.len);
	} while (0);
	db_msg("done clean bufer");
	memzero(passhash, PASSWD_HASHED_LEN);
	if (wmsg) {
		proto_delete_wmessage(wmsg);
		if (wmsg_wrapper && wmsg_wrapper != wmsg) {
			proto_delete_wmessage(wmsg_wrapper);
		}
	}
	changeWindow(WINDOWID_MAINPANEL);
	return 0;
}

int QrProcWin::genPubHDNode(struct pbc_wmessage *wmsg, const CoinInfo *info, unsigned char passhash[PASSWD_HASHED_LEN]) {
	int ret = get_coin_pubkey_wmsg(wmsg, info, passhash);
	if (ret == 0) {
		//save in coins db
		storage_save_coin(info->type, info->uname);
	}
	return ret;
}

int QrProcWin::syncAllCoins(struct pbc_wmessage *wmsg, unsigned char passhash[PASSWD_HASHED_LEN], struct pbc_rmessage *req) {
	int ret;
	if (gSettings->mCoinsVersion < COINS_INIT_VERSION) {
		loading_win_start(mHwnd, "", NULL, 0);
		wallet_initDefaultCoin(passhash);
		loading_win_stop();
	}

	int count = storage_getCoinsCount(0);
	db_msg("coins count:%d", count);
	if (count == 0) {
		loading_win_start(mHwnd, "", NULL, 0);
		wallet_initDefaultCoin(passhash);
		loading_win_stop();
		count = storage_getCoinsCount(0);
		db_msg("after init coins count:%d", count);
	}
	if (count < 1) {
		dialog_error3(mHwnd, -201, "Pair failed.");
		return 0;
	}

	int coin_number = pbc_rmessage_size(req, "coin");
	db_msg("coin_number:%d", coin_number);
	long long clock = getMsClockTime();
	long long clock2;
	if (coin_number > 0) {
		CoinInfo qinfo;
		int newc = 0;
		int coin_yes = 0;
		int coin_no = 0;
		for (int i = 0; i < coin_number; i++) {
			memset(&qinfo, 0, sizeof(qinfo));
			if (proto_rmsg_CoinInfo(req, &qinfo, "coin", i) != 0) {
				db_error("get coin info false");
				break;
			}
			if (coin_is_real_coin(qinfo.type, qinfo.uname)) {
				coin_yes++;
				if (!storage_isCoinExist(qinfo.type, qinfo.uname)) {
					if (!newc) {
						loading_win_start(mHwnd, "", NULL, 0);
						clock = getMsClockTime();
					}
					newc++;
					wallet_genDefaultPubHDNode(passhash, qinfo.type, qinfo.uname);
					clock2 = getMsClockTime();
					if (clock2 > (clock + 100)) {
						clock = clock2;
						loading_win_refresh();
					}
				}
			} else {
				coin_no++;
			}
		}
		if (newc) {
			loading_win_stop();
			count = storage_getCoinsCount(0);
			db_msg("after dsync add count:%d", count);
			if (count < 1) {
				dialog_error3(mHwnd, -202, "Pair failed.");
				return 0;
			}
		}
		if (coin_no) {
			//picDialog(mHwnd, "coin_some_not_supported", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0);
		}
	}

	int oknum = 0;
	int errnum = 0;
	DBCoinInfo coin;
	CoinInfo qinfo;
	for (int i = 0; i < count; i++) {
		memset(&coin, 0, sizeof(DBCoinInfo));
		memset(&qinfo, 0, sizeof(qinfo));
		if (storage_queryCoinInfo(&coin, 1, i, 0) != 1) {
			break;
		}
		if (!coin_is_real_coin(coin.type, coin.uname)) {
			db_msg("skip not real coin:%d %s", coin.type, coin.uname);
			continue;
		}
		qinfo.type = coin.type;
		qinfo.uname = coin.uname;
		ret = genPubHDNode(wmsg, &qinfo, passhash);
		if (ret != 0) {
			errnum++;
		} else {
			oknum++;
		}
	}

	if (errnum) {
		if (oknum) {
			dialog_error3(mHwnd, -(10000 + errnum), "Pair failed.");
		} else {
			dialog_error3(mHwnd, -(20000 + errnum), "Pair failed.");
		}
	}
	return 0;
}

int QrProcWin::confirmGetPubkey() {
	db_msg("start");
	struct pbc_rmessage *req = mMessage->rmsg;
	int coin_number = pbc_rmessage_size(req, "coin");
	if (coin_number < 1) {
		db_error("invalid coin size:%d", coin_number);
		return -1;
	}

	struct pbc_wmessage *wmsg = proto_new_wmessage("Wallet.GetPubkeyResp");
	if (!wmsg) {
		db_error("proto new wmessage failed");
		return 0;
	}

	unsigned char passhash[PASSWD_HASHED_LEN] = {0};
	int ret = passwdKeyboard(mHwnd, res_getLabel(LANG_LABEL_ENTER_PASSWD), PIN_CODE_VERITY, passhash, 1);
	if (ret < 0) {
		db_error("input passwd ret:%d", ret);
		proto_delete_wmessage(wmsg);
		if (gHaveSeed) {
			changeWindow(WINDOWID_MAINPANEL);
		}
		return -1;
	}

	CoinInfo qinfo;
	int oknum = 0;
	int errnum = 0;
	int i = 0;
	long long clock = getMsClockTime();
	long long clock2;
	loading_win_start(mHwnd, "", NULL, 0);
	for (; i < coin_number; i++) {
		memset(&qinfo, 0, sizeof(qinfo));
		if (proto_rmsg_CoinInfo(req, &qinfo, "coin", i) != 0) {
			db_error("get coin info false");
			break;
		}
		ret = genPubHDNode(wmsg, &qinfo, passhash);
		if (ret != 0) {
			errnum++;
		} else {
			oknum++;
		}
		clock2 = getMsClockTime();
		if (clock2 > (clock + 100)) {
			clock = clock2;
			loading_win_refresh();
		}
	}
	loading_win_stop();
	memzero(passhash, PASSWD_HASHED_LEN);
	if (!oknum) { //all false
		db_error("gen node false i:%d total:%d", i, coin_number);
		proto_delete_wmessage(wmsg);
		picDialog(mHwnd, "coin_all_not_supported", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0);
		changeWindow(WINDOWID_MAINPANEL);
		return -1;
	}
	if (errnum) { //some false
		picDialog(mHwnd, "coin_some_not_supported", res_getLabel(LANG_LABEL_TXT_OK), NULL, 0);
	}
	struct pbc_wmessage *device = pbc_wmessage_message(wmsg, "device_info");
	if (device) {
		genDeviceInfo(device, 1);
	}
    struct pbc_slice result, slice_ext_head;
    pbc_wmessage_buffer(wmsg, &result);

    struct pbc_wmessage *wmsg_wrapper = NULL;
    wmsg_wrapper = proto_new_wmessage("Wallet.PacketRespHeaderWrapper");
    struct pbc_wmessage *ext_header = pbc_wmessage_message(wmsg_wrapper, "header");
    pbc_wmessage_integer(ext_header, "version", (uint32_t) DEVICE_APP_INT_VERSION, 0);
    pbc_wmessage_buffer(wmsg_wrapper, &slice_ext_head);
    unsigned char *merge_buff = (unsigned char *) malloc(result.len + slice_ext_head.len);
    if (!merge_buff) {
        proto_delete_wmessage(wmsg);
        proto_delete_wmessage(wmsg_wrapper);
        changeWindow(WINDOWID_MAINPANEL);
        return -1;
    }
    memcpy(merge_buff, slice_ext_head.buffer, slice_ext_head.len);
    memcpy(merge_buff + slice_ext_head.len, result.buffer, result.len);
    uint16_t flag = mMessage->flag | QR_FLAG_EXT_HEADER;

    showQRWindow(mHwnd, mMessage->client_id, flag, QR_MSG_GET_PUBKEY_RESP, (const unsigned char *) merge_buff,
                 result.len + slice_ext_head.len);
    proto_delete_wmessage(wmsg);
    proto_delete_wmessage(wmsg_wrapper);
    free(merge_buff);
    changeWindow(WINDOWID_MAINPANEL);
	return 0;
}

void active_show_time_cb(HWND hParent) {
	db_msg("hwnd:%d", hParent);
	char str[32];
	label_set_param set;
	res_getLabelSetParam(&set, MK_system, "active_time_label");
	HWND hwnd = createWidgetWindow(hParent, 0, set.x, set.y, set.w, set.h, 0, WIDGET_TYPE_TEXT, set.style, set.font);
	int atime = device_get_active_time();
	format_time(str, sizeof(str), atime, 0, 2);
	SetWindowText(hwnd, str);
}

int QrProcWin::procUserActive() {
	if (!mMessage || !mMessage->data || mMessage->data->len < sizeof(user_active_info)) {
		db_error("invalid request");
		changeWindow(WINDOWID_GUIDE);
		return -1;
	}
	if (memcmp(mMessage->data->str, "UA:", 3) != 0) {
		db_error("invalid request");
		changeWindow(WINDOWID_GUIDE);
		return -1;
	}
	user_active_info info;
	int ret = 0;
	loading_win_start(mHwnd, "", NULL, 0);
	if (active_decode_info(&info, (const unsigned char *) (mMessage->data->str + 3), mMessage->data->len - 3) != 0) {
		db_error("decode active info false");
		loading_win_stop();
		changeWindow(WINDOWID_GUIDE);
		return 0;
	}
	loading_win_refresh();
	ret = device_user_active((UserActiveInfo *) &info, 1);
	if (ret != 0) {
		db_debug("device user active ret:%d", ret);
		loading_win_stop();
		changeWindow(WINDOWID_GUIDE);
		return -1;
	}
	loading_win_stop();
	Global_Active_step = 2;

	PicDialogConfig_t config;
	memset(&config, 0, sizeof(PicDialogConfig_t));
	const char *btnTitles[2] = {0, 0};
	config.name = "wallet_active_success_tip";
	config.total = 1;
	config.initIndex = 0;
	btnTitles[0] = res_getLabel(LANG_LABEL_SUBMENU_OK);
	config.btnTitles = btnTitles;
	config.buttonCount = 1;
	config.initBtnIndex = 0;
	config.flag = PIC_DLG_FLAG_RIGHT_AS_OK;
	config.onInitCallback = active_show_time_cb;
	ret = showPicDialog(mHwnd, &config);
	changeWindow(WINDOWID_GUIDE);
	return 0;
}
