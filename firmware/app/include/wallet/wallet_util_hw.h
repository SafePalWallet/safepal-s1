#ifndef WALLET_WALLET_UTIL_HW_H
#define WALLET_WALLET_UTIL_HW_H

#include "wallet_util.h"

#ifdef __cplusplus
extern "C" {
#endif

int get_mix_random_buffer(uint8_t *buf, size_t len);

int get_message_process_winid(const ProtoClientMessage *msg);

int get_coin_icon_path(int type, const char *uname, char *path, int size);

#ifdef __cplusplus
}
#endif
#endif
