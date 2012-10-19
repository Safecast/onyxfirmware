#ifndef __REALTIME_H__
#define __REALTIME_H__

#include <stdint.h>
uint32_t realtime_get_unixtime();
void realtime_set_unixtime(uint32_t time_in);
void realtime_setdate(uint8_t hours,uint8_t min,uint8_t sec,uint8_t day,uint8_t month,uint16_t year);
void realtime_getdate(uint8_t &hours,uint8_t &min,uint8_t &sec,uint8_t &day,uint8_t &month,uint16_t &year);
void realtime_getdate_local(uint8_t &hours,uint8_t &min,uint8_t &sec,uint8_t &day,uint8_t &month,uint16_t &year);

void    realtime_setutcoffset_mins(int16_t mins);
bool    realtime_getutcoffset_available();
int16_t realtime_getutcoffset_mins();

void realtime_initialise(void);
int  realtime_deinit();

#endif
