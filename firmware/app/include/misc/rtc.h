#ifndef _RTC_H_
#define _RTC_H_

#include <sys/time.h>
#include <linux/rtc.h>
#include <time.h>

#define MINI_SYSTEM_UTC_TIME    1546300800L  //UTC 2019-01-01
#define VALID_SYSTEM_UTC_TIME   1564617600L  //UTC 2019-08-01

#ifdef __cplusplus
extern "C" {
#endif

long getRTCTime();

long getClockTime();

long long getMsClockTime();

#ifdef __cplusplus
}
#endif
#endif    //_RTC_H_

