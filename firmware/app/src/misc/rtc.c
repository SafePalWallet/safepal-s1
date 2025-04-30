#define LOG_TAG "Rtc"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <stdio.h>
#include "rtc.h"
#include "debug.h"
#include <errno.h>

long getRTCTime() {
	int rtc_handle;
	int ret = 0;
	struct rtc_time rtc_tm;
	rtc_handle = open("/dev/rtc0", O_RDWR);
	if (rtc_handle < 0) {
		db_error("open /dev/rtc0 fail");
		return -1;
	}
	memset(&rtc_tm, 0, sizeof(rtc_tm));
	ret = ioctl(rtc_handle, RTC_RD_TIME, &rtc_tm);
	close(rtc_handle);
	if (ret < 0) {
		db_error("getRTCTime ioctrl RTC_RD_TIME fail");
		return -2;
	}

	struct tm tm_time;
	memset(&tm_time, 0, sizeof(tm_time));

	tm_time.tm_sec = rtc_tm.tm_sec;
	tm_time.tm_min = rtc_tm.tm_min;
	tm_time.tm_hour = rtc_tm.tm_hour;
	tm_time.tm_mday = rtc_tm.tm_mday;
	tm_time.tm_mon = rtc_tm.tm_mon;
	tm_time.tm_year = rtc_tm.tm_year;
	tm_time.tm_wday = rtc_tm.tm_wday;
	tm_time.tm_yday = rtc_tm.tm_yday;
	tm_time.tm_isdst = rtc_tm.tm_isdst;

	return mktime(&tm_time);
}

long getClockTime() {
	struct timespec clktm;
	clock_gettime(CLOCK_MONOTONIC, &clktm);
	return clktm.tv_sec;
}

long long getMsClockTime() {
	struct timespec clktm;
	clock_gettime(CLOCK_MONOTONIC, &clktm);
	return clktm.tv_sec * 1000 + clktm.tv_nsec / 1000000;
}
