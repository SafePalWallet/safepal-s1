#ifndef WALLET_PROTOBUF_UTIL_H
#define WALLET_PROTOBUF_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int pb_encode32(uint32_t number, uint8_t buffer[10]);

int pb_decode(uint8_t buffer[10], uint32_t *low, uint32_t *hi);

#ifdef __cplusplus
}
#endif
#endif
