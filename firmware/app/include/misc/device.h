#ifndef __DEVICE_H__
#define __DEVICE_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char tag[2];
    unsigned char version;
    unsigned char sign_index;
    char sn[24];
    int time;
    int time_zone;
    unsigned char sign_data[64];
} UserActiveInfo;

int device_init(unsigned char *mask);

int device_get_cpuid(char *buf, int len);

const char *device_get_cpuid_p();

int device_get_diviceid(char *buf, int len);

const char *device_get_diviceid_p();

int device_get_id(char *buf, int len);

int device_is_inited();

int device_check_sn(const char *sn);

int device_get_sn(char *buf, int len);

int device_check_host();

int device_check_sechip();

int device_check_se_shake(const unsigned char *host_random, const unsigned char *se_sign);

int device_sign_se_shake(const unsigned char *se_random, unsigned char *host_sign);

int device_get_user_active_info(UserActiveInfo *info);

int device_get_active_time();

int device_user_active(UserActiveInfo *info, int do_active);

int device_get_hw_break_state(int trytime);

int device_boot_is_otp();

int device_set_boot_otp();

uint64_t device_read_seed_account();

int device_save_seed_account(uint64_t id);

int device_read_settings(unsigned char *data, int size);

int device_save_settings(const unsigned char *data, int size);

int device_clean_all_info();

int device_reformat_data_partition();

long device_data_partition_size();

#ifdef __cplusplus
}
#endif
#endif
