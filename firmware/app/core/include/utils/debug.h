#ifndef WALLET_DEBUG_H
#define WALLET_DEBUG_H

#ifndef LOG_TAG
#define LOG_TAG ""
#endif

//#define CONFIG_DEBUG_PERFORMANCE

#ifdef BUILD_FOR_RELEASE
#define CONFIG_LOG_TYPE 0
#define DB_LOG_LEVEL  0
#else
#define CONFIG_LOG_TYPE 2
#endif

/* if DB_LOG_LEVEL set to 1, then all the debug info is included */
#ifndef DB_LOG_LEVEL
#ifdef BUILD_FOR_DEV
#define DB_LOG_LEVEL  1
#else
#define DB_LOG_LEVEL  0
#endif
#endif //DB_LOG_LEVEL

#undef DB_VERBOSE
#undef DB_DEBUG
#undef DB_INFO
#undef DB_WARN
#undef DB_ERROR
#undef DB_SECURE

#if DB_LOG_LEVEL == 4
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 3
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 2
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#elif DB_LOG_LEVEL == 1
#define DB_VERBOSE
#define DB_DEBUG
#define DB_INFO
#define DB_WARN
#define DB_ERROR
#define DB_SECURE
#endif

#if DB_LOG_LEVEL > 0

#define DEBUG_ON

#ifdef __cplusplus
extern "C" {
#endif

const char *debug_ubin_to_hex(const unsigned char *bin_in, int size);

const char *debug_bin_to_hex(const char *bin_in, int size);

#ifndef DEBUG_NO_TIME
extern const char *DebugThreadTime();
#endif

#ifdef CONFIG_DEBUG_PERFORMANCE
long long debugGetClockTime(void);
#endif

void debug_show_long_bin_data(const char *msg, const unsigned char *bin_in, int size);

#ifdef DB_SECURE
#define secure_show_long_bin_data debug_show_long_bin_data
#else
#define secure_show_long_bin_data(...)
#endif

#ifdef __cplusplus
}
#endif

#else
#undef DEBUG_ON
#define DB_NO_DEBUG

#define debug_show_long_bin_data(...)
#define secure_show_long_bin_data(...)

#endif

#if CONFIG_LOG_TYPE == 2

#ifndef debug_print_func
#include <stdio.h>
#define debug_print_func printf
#endif

#ifndef DEBUG_NO_TIME

#ifndef ALOGV
#define ALOGV(fmt, arg...) debug_print_func("%s V %s: " fmt"\n", DebugThreadTime(), LOG_TAG, ##arg)
#endif

#ifndef ALOGD
#define ALOGD(fmt, arg...) debug_print_func("%s D %s: " fmt"\n", DebugThreadTime(), LOG_TAG, ##arg)
#endif

#ifndef ALOGI
#define ALOGI(fmt, arg...) debug_print_func("%s I %s: " fmt"\n", DebugThreadTime(), LOG_TAG, ##arg)
#endif

#ifndef ALOGW
#define ALOGW(fmt, arg...) debug_print_func("%s W %s: " fmt"\n", DebugThreadTime(), LOG_TAG, ##arg)
#endif

#ifndef ALOGE
#define ALOGE(fmt, arg...) debug_print_func("%s E %s: " fmt"\n", DebugThreadTime(), LOG_TAG, ##arg)
#endif

#else

#ifndef ALOGV
#define ALOGV(fmt, arg...) debug_print_func("V %s: " fmt"\n", LOG_TAG, ##arg)
#endif

#ifndef ALOGD
#define ALOGD(fmt, arg...) debug_print_func("D %s: " fmt"\n", LOG_TAG, ##arg)
#endif

#ifndef ALOGI
#define ALOGI(fmt, arg...) debug_print_func("I %s: " fmt"\n", LOG_TAG, ##arg)
#endif

#ifndef ALOGW
#define ALOGW(fmt, arg...) debug_print_func("W %s: " fmt"\n", LOG_TAG, ##arg)
#endif

#ifndef ALOGE
#define ALOGE(fmt, arg...) debug_print_func("E %s: " fmt"\n", LOG_TAG, ##arg)
#endif

#endif //DEBUG_NO_TIME

#endif //CONFIG_LOG_TYPE == 2

#ifdef DB_VERBOSE
#define db_verbose(fmt, arg...) ALOGV("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#undef ALOGV
#define ALOGV(fmt, ...)
#define db_verbose(fmt, ...)
#endif

#ifdef DB_DEBUG
#define db_debug(fmt, arg...) ALOGD("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#define db_msg(fmt, arg...)   ALOGD("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#undef ALOGD
#define ALOGD(fmt, ...)
#define db_debug(fmt, ...)
#define db_msg(fmt, ...)
#endif

#ifdef DB_INFO
#define db_info(fmt, arg...)  ALOGI("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#undef ALOGI
#define ALOGI(fmt, ...)
#define db_info(fmt, ...)
#endif

#ifdef DB_WARN
#define db_warn(fmt, arg...)  ALOGW("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#undef ALOGW
#define ALOGW(fmt, ...)
#define db_warn(fmt, ...)
#endif

#ifdef DB_ERROR
#define db_error(fmt, arg...) ALOGE("<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#undef ALOGE
#define ALOGE(fmt, ...)
#define db_error(fmt, ...)
#endif

#ifdef DB_SECURE
#define db_secure(fmt, arg...)  ALOGD("[secure]<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#define db_serr(fmt, arg...)    ALOGE("[serr]<L:%04d %s()> " fmt, __LINE__, __FUNCTION__, ##arg)
#else
#define db_secure(fmt, ...)
#define db_serr(fmt, ...)
#endif

#ifdef CONFIG_DEBUG_PERFORMANCE
extern long DEBUG_TESTTIME0;
extern long DEBUG_TESTTIME1;

#define DEBUG_PERFORMANCE() do{ \
DEBUG_TESTTIME1 = debugGetClockTime(); \
ALOGD("DEBUG_PERFORMANCE t0:%ld t1:%ld itv:%ld", DEBUG_TESTTIME0, DEBUG_TESTTIME1, DEBUG_TESTTIME1 - DEBUG_TESTTIME0); \
DEBUG_TESTTIME0 = DEBUG_TESTTIME1; \
 }while(0)
#else
#define DEBUG_PERFORMANCE()
#endif
#endif
