#include "coin_adapter.h"
#include "win_comm.h"
#include "widgets.h"
#include "fileutil.h"
#include "settings.h"
#include "qr_pack.h"

int tx_common_show_sign_result(HWND hwnd, const ProtoClientMessage *msg, struct pbc_wmessage *sigmsg, int msgtype) {
    struct pbc_slice slice, slice_ext_head;
    int ret = 0;
    struct pbc_wmessage *wmsg_wrapper = NULL;

    wmsg_wrapper = proto_new_wmessage("Wallet.PacketRespHeaderWrapper");
    struct pbc_wmessage *ext_header = pbc_wmessage_message(wmsg_wrapper, "header");

    pbc_wmessage_integer(ext_header, "version", (uint32_t) DEVICE_APP_INT_VERSION, 0);
    pbc_wmessage_buffer(wmsg_wrapper, &slice_ext_head);

    pbc_wmessage_buffer(sigmsg, &slice);
    //db_msg("result sz:%d data:%s", slice.len, debug_bin_to_hex((const char *) slice.buffer, slice.len));

#ifdef BUILD_FOR_DEV
    char buff[4096];
    int n = snprintf(buff, sizeof(buff), "ctime:%u client:%d type:%d len:%d val:", msg->time, msg->client_id, msgtype,
                     slice.len);
    int l = slice.len;
    if (l + n > 4090) l = 4090 - n;
    bin_to_hex(slice.buffer, l / 2, buff + n);
    l = strlen(buff);
    buff[l++] = '\n';
    buff[l] = 0;
    FILE *fp = fopen(DATA_POINT"/resp.txt", "a+");
    if (fp != NULL) {
        fwrite(buff, l, 1, fp);
        fflush(fp);
        fclose(fp);
    }
#endif

    unsigned char *merge_buff = (unsigned char *) malloc(slice.len + slice_ext_head.len);
    if (!merge_buff) {
        proto_delete_wmessage(sigmsg);
        proto_delete_wmessage(wmsg_wrapper);
        return -1;
    }

    memcpy(merge_buff, slice_ext_head.buffer, slice_ext_head.len);
    memcpy(merge_buff + slice_ext_head.len, slice.buffer, slice.len);
    uint16_t flag = msg->flag | QR_FLAG_EXT_HEADER;
    db_msg("result sz:%d data:%s", slice.len+slice_ext_head.len, debug_bin_to_hex((const char *) merge_buff, slice.len+slice_ext_head.len));
    ret = showQRWindow(hwnd, msg->client_id, flag, msgtype, (const unsigned char *) merge_buff,
                       (int) (slice.len + slice_ext_head.len));
    if (ret < 0) {
        db_error("show msg:%d false,ret:%d", msgtype, ret);
    }

    free(merge_buff);
    proto_delete_wmessage(sigmsg);
    proto_delete_wmessage(wmsg_wrapper);

    return ret;
}

void tx_set_db_view(const CoinConfig *config, DBTxCoinInfo *db, DynamicViewCtx *view) {
	db->coin_type = config->type;
	strlcpy(db->coin_name, config->name, sizeof(db->coin_name));
	strlcpy(db->coin_symbol, config->symbol, sizeof(db->coin_symbol));
	strlcpy(db->coin_uname, config->uname, sizeof(db->coin_uname));

	view->coin_type = config->type;
	view->coin_uname = config->uname;
	view->coin_name = config->name;
	view->coin_symbol = config->symbol;
}

void tx_set_db_view_info(DBTxCoinInfo *db, DynamicViewCtx *view, int coin_type, const char *coin_uname, const char *coin_name, const char *coin_symbol) {
	db->coin_type = coin_type;
	strlcpy(db->coin_name, coin_name, sizeof(db->coin_name));
	strlcpy(db->coin_symbol, coin_symbol, sizeof(db->coin_symbol));
	strlcpy(db->coin_uname, coin_uname, sizeof(db->coin_uname));

	view->coin_type = coin_type;
	view->coin_uname = coin_uname;
	view->coin_name = coin_name;
	view->coin_symbol = coin_symbol;
}
