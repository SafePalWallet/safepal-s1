#define LOG_TAG "CameraManager"

#include <sys/ioctl.h>
#include "common.h"
#include "CameraManager.h"
#include "AppMain.h"
#include "storage_manager.h"

static AppMain *mAppMain = NULL;
static CameraAdapter *mAdapter;

static void qrdecodeCallback(const void *data, int size, void *caller) {
	if (caller != NULL) {
		((CameraManager *) caller)->onQrResult((const char *) data, size);
	}
}

CameraManager::CameraManager() {
	mQrEnable = false;
	mLastQrResult = NULL;
	init_qr_packet(&mQrResult, 0);
	memset(&mQrBuffer, 0, sizeof(mQrBuffer));
	mAppMain = AppMain::getInstance();
	mAdapter = new CameraAdapter();
}

CameraManager::~CameraManager() {
	if (mAdapter) {
		delete mAdapter;
		mAdapter = NULL;
	}
}

int CameraManager::init() {
	return mAdapter->init(qrdecodeCallback, this);
}

int CameraManager::camExist() {
	return mAdapter->camExist();
}

void CameraManager::startPreview() {
	Mutex::Autolock _l(mLock);
	clearQrDecode(0);
	mAdapter->startPreview();
	setQrDecode(true);
}

bool CameraManager::isPreviewing() {
	return false;
}

void CameraManager::stopPreview() {
	Mutex::Autolock _l(mLock);
	mAdapter->stopPreview();
	setQrDecode(false);
	clearQrDecode(1);
}

void CameraManager::stopPreviewNoLock() {
    mAdapter->stopPreview();
}

void CameraManager::qrChunk(int datalen, char *data) {
	Mutex::Autolock _l(mLock);
	mAdapter->qrChunk(datalen, data);
}

void CameraManager::onQrResult(const char *data, int size) {
	int finish = 0;
	int errcode = 0;
	Mutex::Autolock _l(mLock);
	device_set_last_active_time(getClockTime()); 
	if (!mQrEnable) {
		db_debug("not qr enable");
		return;
	}
	if (mLastQrResult != NULL) {
		if ((int) mLastQrResult->len == size && memcmp(mLastQrResult->str, data, size) == 0) {
			db_debug("skip same qr rs:%p", mLastQrResult);
			return;
		} else {
			db_debug("diff qr:%p old len:%d new sz:%d", mLastQrResult, mLastQrResult->len, size);
		}
	}

	if (mLastQrResult != NULL) {
		cstr_set_buf(mLastQrResult, data, size);
	} else {
		mLastQrResult = cstr_new_buf(data, size);
	}
	db_debug("last qr:%p sz:%d str_p:%p", mLastQrResult, mLastQrResult->len, mLastQrResult->str);

	qr_packet *qr = &mQrResult;
	if (is_bin_qr_packet(data, size)) {
		errcode = merge_qr_packet_buffer(&mQrBuffer, qr, data, (size_t) size);
		if (errcode == 0) {
			finish = 1;
		} else if (errcode == 1) {
			errcode = 0;
		}
	} else {
		db_debug("qr not isBinPacket");
		if (qr->p_total > 1) {
			db_error("not multi qrcode");
			errcode = -201;
		} else {
			finish = 1;
			free_qr_packet(qr);
			qr->p_total = 1;
			qr->data = cstr_new_buf(data, size);
		}
	}

	if (errcode == QR_DECODE_SUCCESS && finish) { //decode
		if (qr->client_id) {
			unsigned char aes256key[32];

			if (storage_getClientSeckey(qr->client_id, aes256key) > 0) {
				db_debug("client:%d aes256key:%s", qr->client_id, debug_ubin_to_hex(aes256key, 32));
				if (decrypt_qr_packet(qr, aes256key) != 0) {
					db_error("aes decrypt qr packet false");
					errcode = QR_DECODE_PACKET_FAILED;
				}
			} else {
				db_error("get client:%d seckey false", qr->client_id);
				errcode = QR_DECODE_UNKOWN_CLIENT;
			}
		}
		if (errcode == QR_DECODE_SUCCESS && verify_qr_packet(qr) != 0) {
			db_error("check packet hash false");
			errcode = QR_DECODE_INVALID_HASH_CHECK;
		}
	}

	if (errcode == QR_DECODE_SUCCESS) {
		if (finish) {
			db_msg("type:%d total:%d index:%d len:%d", qr->type, qr->p_total, qr->p_index, qr->data->len);
			setQrDecode(false);
            stopPreviewNoLock();
            mAppMain->sendUiEvent(UI_EVENT_QR_RESULT, qr->type, (long) qr);
		} else if (qr->p_total >= 1) {
            if ((qr->p_index + 1) == qr->p_total) {
                stopPreviewNoLock();
            }
            mAppMain->sendUiEvent(UI_EVENT_QR_CHUNK, qr->p_index, qr->p_total);
		}
	} else {
        stopPreviewNoLock();
        mAppMain->sendUiEvent(UI_EVENT_QR_ERROR, errcode);
	}
}

int CameraManager::clearQrDecode(int type) {
	db_msg("type:%d", type);
	if (mLastQrResult != NULL) {
		cstr_free(mLastQrResult);
		mLastQrResult = NULL;
	}
	if (type == 0) {
		free_qr_packet(&mQrResult);
	}
	free_qr_buffer(&mQrBuffer);
	return 0;
}

int CameraManager::setQrDecode(bool enable) {
	mQrEnable = enable;
	return mAdapter->setQrDecode(enable);
}
