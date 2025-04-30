#ifndef WALLET_COIN_UTIL_HW_H
#define WALLET_COIN_UTIL_HW_H

#ifdef __cplusplus
extern "C" {
#endif

#define UNSUPPORT_MSG_UPGRADE_TRY_AGAIN   (-10300)

int is_coin_address_long(int coinid, const char *uname);

int format_data_to_hex(const unsigned char *bytes, int size, char *tmpbuf, int bufflen);

int format_data_to_hex_b(const unsigned char *bytes, int size, char *tmpbuf, int bufflen);

int bignum2double(const unsigned char *bytes, int size, uint8_t decimals, double *value, char *value_str, size_t value_str_size);

int bignum_print(const unsigned char *bytes, int size, uint8_t decimals, const char *prefix, char *value_str, size_t value_str_size);

#ifdef __cplusplus
}
#endif
#endif
