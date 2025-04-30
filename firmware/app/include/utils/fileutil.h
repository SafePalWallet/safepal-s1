#ifndef WALLET_FILEUTIL_H
#define WALLET_FILEUTIL_H

#include <stdint.h>

#include "common_util.h"

#ifdef __cplusplus
extern "C" {
#endif

int file_get_int(const char *pathname);

int file_get_line(const char *pathname, char *buffer, int length);

int file_get_contents(const char *pathname, unsigned char *buffer, int length);

long file_size(const char *path);

int file_put_contents(const char *pathname, const void *data, int size);

int file_write_int(const char *pathname, uint32_t n);

#ifdef __cplusplus
}
#endif
#endif
