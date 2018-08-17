#ifndef _RTCSYNC_INCLUDED_
#define _RTCSYNC_INCLUDED_

// RTC content read/write
int rtc_read(struct tm *tm);
int rtc_write(const struct tm *tm);
// write system time to RTC
int rtc_sync();
// read RTC time and set as system time
int rtc_load();

#endif
