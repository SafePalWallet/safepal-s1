#define LOG_TAG "showQRWindow"

#include "storage_manager.h"
#include "common_c.h"
#include "resource.h"
#include "key_event.h"
#include "widgets.h"
#include "qr.h"
#include "qr_util.h"
#include "fileutil.h"
#include "qr_pack.h"
#include "cstr.h"
#include <aes/aes.h>
#include <sha2.h>

#define ID_TIMER_FLUSH_QR  1002

static int auto_refresh_qr = 0;

typedef struct {
	int qr_index;
	int qr_total;
	int qr_type;
	qr_packet_chunk_info chunk_info;
	WIN_RECT qr_rect;
	BITMAP qr_image;
	WIN_RECT indicate_rect;
	WIN_RECT indicate_bg_rect;
	int indicate_style;
	BITMAP indicate_bg[2];
} qrState_t;

PROC_RET showQRWinProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam);

int genQrcodeBitmap(PBITMAP pBitmap, const unsigned char *src, int src_size, int w, int h, int separator, int qr_mode) {
	QRCode *qr;
	int errcode = QR_ERR_NONE;
	qr = qrInit(-1, qr_mode, QR_ECL_H, -1, &errcode);
	if (qr == NULL) {
		db_error("qrInit ret:%d %s", errcode, qrStrError(errcode));
		return -1;
	}
	if (!qrAddData2(qr, src, src_size, qr_mode)) {
		db_error("qrAddData2 false:%s", qrGetErrorInfo(qr));
		qrDestroy(qr);
		return -1;
	}

	if (!qrFinalize(qr)) {
		qrDestroy(qr);
		db_error("finalize error:%s", qrGetErrorInfo(qr));
		return -1;
	}
	int imgdim = qrGetSymbolImgDimension(qr, separator);
	if (imgdim < 21) {
		qrDestroy(qr);
		db_error("qr get dim false:%d", imgdim);
		return -1;
	}
	int mag = (w > h ? h : w) / imgdim;
	if (mag < 1) {
		mag = 1;
	} else if (mag > 6) {
		mag = 6;
	}

	int size = 0;
	qr_byte_t *buffer = qrSymbolToBMP(qr, separator, mag, &size);
	if (buffer == NULL || size < 0) {
		qrDestroy(qr);
		db_error("qrGetSymbol false:%s", qrGetErrorInfo(qr));
		return -1;
	}
	if (LoadBitmapFromMem(HDC_SCREEN, pBitmap, buffer, size, "bmp") != 0) {
		free(buffer);
		qrDestroy(qr);
		db_error("LoadBitmap:%s false", src);
		return -1;
	}
	db_verbose("gen qr size:%d sep:%d dim:%d mag:%d limit w:%d h:%d qr size:%d w:%d h:%d space:%d",
	           src_size, separator, imgdim, mag, w, h, size, pBitmap->bmWidth, pBitmap->bmHeight, h - pBitmap->bmHeight);
	free(buffer);
	qrDestroy(qr);
	return 0;
}

static void flushQrImage(HWND hDlg, qrState_t *qrstate) {
	int index = qrstate->qr_index;
	int total = qrstate->qr_total;
	int n;
	if (!(index >= 0 && index < total)) {
		db_debug("Invalid index:%d total:%d", index, total);
		return;
	}
	if (qrstate->qr_image.bmBits) {
		UnloadBitmap(&qrstate->qr_image);
	}

	if (is_printable_str((const char *) qrstate->chunk_info.chunks[index].data)) {
		db_verbose("index:%d str qr:%d=>%s", index, qrstate->chunk_info.chunks[index].size, qrstate->chunk_info.chunks[index].data);
	} else {
		db_verbose("index:%d bin qr:%d=>%s", index, qrstate->chunk_info.chunks[index].size,
		           debug_ubin_to_hex(qrstate->chunk_info.chunks[index].data, qrstate->chunk_info.chunks[index].size));
	}

	genQrcodeBitmap(&qrstate->qr_image, qrstate->chunk_info.chunks[index].data,
	                qrstate->chunk_info.chunks[index].size, qrstate->qr_rect.w, qrstate->qr_rect.h, total > 1 ? 2 : 1, QR_EM_8BIT);

	int bw = qrstate->qr_image.bmWidth;
	int bh = qrstate->qr_image.bmHeight;
	int x, y;
	x = qrstate->qr_rect.x + (qrstate->qr_rect.w - bw) / 2;
	y = qrstate->qr_rect.y + (qrstate->qr_rect.h - bh) / 2;
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	db_verbose("index:%d total:%d qr rect x:%d y:%d w:%d h:%d; img x:%d y:%d w:%d h:%d ", index, total,
	           qrstate->qr_rect.x, qrstate->qr_rect.y, qrstate->qr_rect.w, qrstate->qr_rect.h, x, y, bw, bh);

	HDC hdc = BeginPaint(hDlg);
	FillBoxWithBitmap(hdc, x, y, bw, bh, &qrstate->qr_image);

	//navi pannel indicate_bg_rect.y  is itemN and itemN+1 's space
	if (qrstate->indicate_style == 1) {
		n = ((qrstate->indicate_bg_rect.w + qrstate->indicate_bg_rect.y) * total - qrstate->indicate_bg_rect.y); //items len
		n = (qrstate->indicate_rect.w - n) / 2; //x
		if (n < 0) n = 0;
		n = qrstate->indicate_rect.x + n; // n is x0
		for (int i = 0; i < total; i++) {
			FillBoxWithBitmap(hdc, n + ((qrstate->indicate_bg_rect.w + qrstate->indicate_bg_rect.y) * i),
			                  qrstate->indicate_rect.y, qrstate->indicate_bg_rect.w, qrstate->indicate_bg_rect.h, &qrstate->indicate_bg[i == index ? 1 : 0]);
		}
	} else if (qrstate->indicate_style == 2) {
		n = (int) (qrstate->indicate_rect.w * ((index + 1) / (double) total));
		if (n < 1) n = 1;
		FillBoxWithBitmap(hdc, qrstate->indicate_rect.x, qrstate->indicate_rect.y,
		                  n, qrstate->indicate_bg_rect.h, &qrstate->indicate_bg[1]);

		//db_debug("index:%d total:%d n:%d rect.w:%d img1 x:%d y:%d w:%d h:%d ", index, total, n, qrstate->indicate_rect.w, qrstate->indicate_rect.x, qrstate->indicate_rect.y,
		//		 n, qrstate->indicate_bg_rect.h);

		if (n < qrstate->indicate_rect.w) {
			FillBoxWithBitmap(hdc, qrstate->indicate_rect.x + n, qrstate->indicate_rect.y,
			                  qrstate->indicate_rect.w - n, qrstate->indicate_bg_rect.h, &qrstate->indicate_bg[0]);
		}
	}
	EndPaint(hDlg, hdc);
}

static void onKeyDown(HWND hDlg, qrState_t *qrstate, int keyCode) {
	db_verbose("MSG_KEYDOWN qrstate %p key:%d", qrstate, keyCode);
	if (keyCode == INPUT_KEY_OK) {
		if (!auto_refresh_qr) {
			EndDialog(hDlg, 0);
		}
	} else if (keyCode == INPUT_KEY_LEFT) {
		if (qrstate->qr_index > 0) {
			qrstate->qr_index--;
			InvalidateRect(hDlg, NULL, TRUE);
		} else if (qrstate->qr_total > 1) {
			qrstate->qr_index = qrstate->qr_total - 1;
			InvalidateRect(hDlg, NULL, TRUE);
		}
	} else if (keyCode == INPUT_KEY_RIGHT) {
		if (qrstate->qr_index + 1 < qrstate->qr_total) {
			qrstate->qr_index++;
			InvalidateRect(hDlg, NULL, TRUE);
		} else if (qrstate->qr_total > 1) {
			qrstate->qr_index = 0;
			InvalidateRect(hDlg, NULL, TRUE);
		}
	}
	if (auto_refresh_qr) {
		auto_refresh_qr = 0;
		KillTimer(hDlg, ID_TIMER_FLUSH_QR);
	}
}

static void autoRefreshQR(HWND hDlg, qrState_t *qrstate) {
	if (qrstate->qr_index + 1 < qrstate->qr_total) {
		qrstate->qr_index++;
	} else {
		qrstate->qr_index = 0;
	}
	InvalidateRect(hDlg, NULL, TRUE);
}

PROC_RET showQRWinProc(HWND hDlg, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	qrState_t *qrstate;
	db_verbose("recived msg:0x%x %s, wParam:%u lParam:%ld", message, Message2Str(message), wParam, lParam);
	switch (message) {
		case MSG_INITDIALOG: {
			if (!lParam) {
				db_error("invalid appQrData");
				return -1;
			}
			qrstate = (qrState_t *) lParam;
			SetWindowAdditionalData(hDlg, (DWORD) lParam);
			//SetWindowBkColor(hDlg, RGBA2Pixel(HDC_SCREEN, 0xFF, 0xFF, 0xFF, 0xFF));
			//SetWindowBkColor(hDlg, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0xFF));
			SetWindowBkColor(hDlg, res_getBGColor());
			if (qrstate->qr_total > 1) {
				auto_refresh_qr = 1;
				SetTimer(hDlg, ID_TIMER_FLUSH_QR, 40);//400ms
			} else {
				auto_refresh_qr = 0;
			}
			break;
		}
		case MSG_PAINT: {
			qrstate = (qrState_t *) GetWindowAdditionalData(hDlg);
			if (!qrstate) {
				db_error("invalid qrstate");
				break;
			}
			flushQrImage(hDlg, qrstate);
			break;
		}
		case MSG_KEYDOWN: {
			qrstate = (qrState_t *) GetWindowAdditionalData(hDlg);
			if (!qrstate) {
				db_error("invalid qrstate");
				break;
			}
			onKeyDown(hDlg, qrstate, wParam);
			break;
		}
		case MSG_TIMER: {
			qrstate = (qrState_t *) GetWindowAdditionalData(hDlg);
			if (!qrstate) {
				db_error("invalid qrstate");
				break;
			}
			if (wParam == ID_TIMER_FLUSH_QR && auto_refresh_qr) {
				autoRefreshQR(hDlg, qrstate);
			}
			break;
		}
		case MSG_DESTROY: {
			if (auto_refresh_qr) {
				auto_refresh_qr = 0;
				KillTimer(hDlg, ID_TIMER_FLUSH_QR);
			}
		}
	}
	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int showQRWindow(HWND hParent, int client_id, unsigned int flag, int msgtype, const unsigned char *qrdata, int size) {

	DLGTEMPLATE dlgtpl;
	qrState_t qrstate;
	WIN_RECT win_rect;
	WIN_RECT ind_rect;
	int ret;
	int show_raw = (flag & SHOW_QR_FLAG_RAW_DATA) ? 1 : 0;
	int aes_encode = client_id > 0 ? 1 : 0;
	unsigned char seckey[CLIENT_SECKEY_SIZE] = {0};
	qr_packet_chunk_slice singal_chunk_slice;
	qr_packet_chunk_info *chunk_info = &qrstate.chunk_info;
	WIN_RECT *qr_rect = &qrstate.qr_rect;
	//testqrlen();


	memset(&dlgtpl, 0, sizeof(dlgtpl));
	memset(&qrstate, 0, sizeof(qrstate));
	int qrtype = QR_TYPE_BIN;

	qrstate.qr_type = qrtype;
	db_debug("client:%d msg:%d size:%d", client_id, msgtype, size);

	if (res_getPos(MK_qr_window, &win_rect) != 0) {
		db_error("get qr win_rect false");
		return -1;
	}

	if (res_getRect(MK_qr_window, "qr_pos", &qrstate.qr_rect) != 0) {
		db_error("get qrwin rect false");
		return -1;
	}

	if (res_getRect(MK_qr_window, "ind_pos", &qrstate.indicate_rect) != 0) {
		db_error("get indicate rect false");
		return -1;
	}

	if (res_getRect(MK_qr_window, "ind_bg_rect", &qrstate.indicate_bg_rect) != 0) {
		db_error("get ind_bg_rect false");
		return -1;
	}

	if (qr_rect->w < 1 || qr_rect->h < 1) {
		db_error("get qrwin rect w:%d h:%d", qr_rect->w, qr_rect->h);
		return -1;
	}

	if (res_getRect(MK_qr_window, "qr_pos", &ind_rect) != 0) {
		db_error("get ind_rect false");
		return -1;
	}

	if (client_id > 0 && aes_encode) {
		if (storage_getClientSeckey(client_id, seckey) <= 0) {
			db_error("get client:%d seckey false", client_id);
			return -1;
		}
	}

    //#define QR_MAX_CHUNK_SIZE 58      //5   
	//#define QR_MAX_CHUNK_SIZE 98      //4
	//#define QR_MAX_CHUNK_SIZE 194     //3   
	//#define QR_MAX_CHUNK_SIZE 461     //2   
	int max_chunk_size = res_getInt(MK_qr_window, "chunk_size", 194);
    if (aes_encode == 1 && msgtype == QR_MSG_MESSAGE_SIGN_RESP) {//add for iphone13
        max_chunk_size = 58;
    }
	db_msg("max_chunk_size=%d,qrtype=%d,show_raw=%d,aes_encode=%d",max_chunk_size,qrtype,show_raw,aes_encode);
	if (max_chunk_size < 34) {
		max_chunk_size = 34;
	} else if (max_chunk_size > 461) {
		max_chunk_size = 461;
	}
	if (max_chunk_size == 194 && qrtype == QR_TYPE_BIN) { 
		int one_page_size = size + QRCODE_PREFIX_LEN + get_qr_packet_header_len(0); // == 14
		if (one_page_size > 194 && one_page_size <= 220) {
			max_chunk_size = 220;
		}
	}
	if (show_raw) {
		chunk_info->total = 1;
		singal_chunk_slice.data = (unsigned char *) qrdata;
		singal_chunk_slice.size = size;
		chunk_info->chunks = &singal_chunk_slice;
		ret = 0;
	} else {
		int qrflag = aes_encode ? QR_FLAG_CRYPT_AES : 0;
		if (flag & QR_FLAG_EXT_HEADER) {
			qrflag |= QR_FLAG_EXT_HEADER;
		}
		ret = split_qr_packet(chunk_info, qrdata, size, qrtype, msgtype, qrflag, client_id, seckey, max_chunk_size);
	}

	if (ret != 0) {
		db_error("split_qr_packet false ret:%d", ret);
		return -1;
	}

	qrstate.qr_total = chunk_info->total;
	if (qrstate.qr_total > 1) {
		if ((qrstate.indicate_bg_rect.w + qrstate.indicate_bg_rect.y) * qrstate.qr_total - qrstate.indicate_bg_rect.y > qrstate.indicate_rect.w) {
			qrstate.indicate_style = 2;
			res_getBmp(MK_qr_window, "ind_bg_p_0", &qrstate.indicate_bg[0]);
			res_getBmp(MK_qr_window, "ind_bg_p_1", &qrstate.indicate_bg[1]);
		} else {
			qrstate.indicate_style = 1;
			res_getBmp(MK_qr_window, "ind_bg_0", &qrstate.indicate_bg[0]);
			res_getBmp(MK_qr_window, "ind_bg_1", &qrstate.indicate_bg[1]);
		}
	} else {
		qrstate.indicate_style = 0;
		//use all win_rect
		memcpy(&qrstate.qr_rect, &win_rect, sizeof(WIN_RECT));
	}

	dlgtpl.dwStyle = WS_VISIBLE;
	dlgtpl.dwExStyle = WS_EX_NONE;
	dlgtpl.x = win_rect.x;
	dlgtpl.y = win_rect.y;
	dlgtpl.w = win_rect.w;
	dlgtpl.h = win_rect.h;
	dlgtpl.caption = "";

	int sst = set_temp_screen_time(180);//screenon
	ret = DialogBoxIndirectParam(&dlgtpl, hParent, showQRWinProc, (LPARAM) (&qrstate));
	set_temp_screen_time(sst);
	if (qrstate.qr_image.bmBits != NULL) {
		UnloadBitmap(&qrstate.qr_image);
	}

	if (qrstate.qr_total > 1) {
		UnloadBitmap(&qrstate.indicate_bg[0]);
		UnloadBitmap(&qrstate.indicate_bg[1]);
	}
	if (!show_raw) {
		free_qr_packet_chunk(chunk_info);
	}
	db_debug("end show qr ret:%d", ret);
	return ret;
}
