#include "rtc.h"
#include <stdint.h>
#include <time.h>

#define SECS_PER_DAY (60*60*24)
#define DAY_SECONDS(x) (x-SECS_PER_DAY*(x/(SECS_PER_DAY)))

uint32_t realtime_get_unixtime() {
    return rtc_get_time(RTC);
}

void realtime_set_unixtime(uint32_t time_in) {
    rtc_set_time(RTC, time_in);
}

void realtime_setdate(uint8_t hours,uint8_t min,uint8_t sec,uint8_t day,uint8_t month,uint16_t year) {
  
  struct tm time_tm;
  time_tm.tm_hour = hours;
  time_tm.tm_min  = min;
  time_tm.tm_sec  = sec;
  time_tm.tm_mday = day;
  time_tm.tm_mon  = month;
  time_tm.tm_year = year;

  time_t utime = mktime(&time_tm);
  realtime_set_unixtime(utime);
}

bool    utc_offset_set   = false;
int16_t utc_offset_mins  = 0;


void realtime_setutcoffset_mins(int16_t mins) {
  utc_offset_set   = true;
  utc_offset_mins  = mins;
}

bool realtime_getutcoffset_available() {
  return utc_offset_set;
}

int16_t realtime_getutcoffset_mins() {
  return utc_offset_mins;
}

void realtime_getdate_local(uint8_t &hours,uint8_t &min,uint8_t &sec,uint8_t &day,uint8_t &month,uint16_t &year) {
  int64_t unix_time_ll = realtime_get_unixtime();

  if(realtime_getutcoffset_available()) {
    int16_t offset = realtime_getutcoffset_mins();
    unix_time_ll += ((int64_t)offset)*60;
  }

  time_t current_time = unix_time_ll;

  struct tm *time;
  time = gmtime(&current_time);
 
  sec   = time->tm_sec;
  min   = time->tm_min;
  hours = time->tm_hour;
  day   = time->tm_mday;
  month = time->tm_mon;
  year  = time->tm_year;

//tm_sec	seconds after the minute	0-61*
//tm_min	minutes after the hour	0-59
//tm_hour	hours since midnight	0-23
//tm_mday	day of the month	1-31
//tm_mon	months since January	0-11
//tm_year	years since 1900	
//tm_wday	days since Sunday	0-6
//tm_yday	days since January 1	0-365
//tm_isdst	Daylight Saving Time flag
}

void realtime_getdate(uint8_t &hours,uint8_t &min,uint8_t &sec,uint8_t &day,uint8_t &month,uint16_t &year) {

  uint32_t unix_time = realtime_get_unixtime();
  time_t current_time = unix_time;

  struct tm *time;
  time = gmtime(&current_time);
 
  sec   = time->tm_sec;
  min   = time->tm_min;
  hours = time->tm_hour;
  day   = time->tm_mday;
  month = time->tm_mon;
  year  = time->tm_year;

//tm_sec	seconds after the minute	0-61*
//tm_min	minutes after the hour	0-59
//tm_hour	hours since midnight	0-23
//tm_mday	day of the month	1-31
//tm_mon	months since January	0-11
//tm_year	years since 1900	
//tm_wday	days since Sunday	0-6
//tm_yday	days since January 1	0-365
//tm_isdst	Daylight Saving Time flag
}

void realtime_initialise(void) {
    rtc_init(RTC);
}

int realtime_deinit() {
    return 0;
}
