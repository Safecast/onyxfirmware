/* place holder code for log_read API */
#include "log.h"
#include "log_read.h"
#include "flashstorage.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "realtime.h"
#include "Geiger.h"
#include <inttypes.h>

uint32_t log_position = 0;
extern Geiger *system_geiger;


extern "C" {

void log_read_start() {
  log_position=0;
}

// return 0 on failure.
// return size of data written, data contains no linefeeds, and single trailing \n
int log_read_block(char *buf) {
 
  buf[0]=0;
 
  char *buf_start = buf;

  log_data_t *flash_log = (log_data_t *) flashstorage_log_get();
  uint32_t logsize = flashstorage_log_size()/sizeof(log_data_t);

  if(log_position==0) {
    int16_t offset_mins = realtime_getutcoffset_mins();

    bool offset_is_valid = realtime_getutcoffset_available();

    char offset_string[10];
    bool offset_is_negative=false;
    if(offset_mins < 0) {
      offset_is_negative=true;
      offset_mins = 0-offset_mins;
    }

    if(offset_is_valid) {
      if(offset_is_negative) offset_string[0] = '-'; else offset_string[0] = '+';
      sprintf(offset_string+1,"%02u",(int)offset_mins/60);
      sprintf(offset_string+3,"%02u",(int)offset_mins%60);
      offset_string[5] = 0;
    } else {
      offset_string[0] = 'u';
      offset_string[1] = 'n';
      offset_string[2] = 'd';
      offset_string[3] = 'e';
      offset_string[4] = 'f';
      offset_string[5] = 0;
    }

    sprintf(buf,"{\"log_size\":%"PRIu32",\"onyx_version\":\"%s\",\"UTC_offset\":\"%s\",\"user_calibration\":%f,\"log_data\":[",logsize,OS100VERSION,offset_string,system_geiger->calibration_scaling);
    buf += strlen(buf);
  }

  if(log_position<logsize) {
    int64_t current_time = flash_log[log_position].time;
    

    struct tm *time;
    time_t current_time_u32 = current_time;
    time = gmtime(&current_time_u32);
    char timestr[200];
    sprintf(timestr,"%u-%02u-%02uT%02u:%02u:%02uZ",time->tm_year+1900,time->tm_mon+1,time->tm_mday,time->tm_hour,time->tm_min,time->tm_sec);

    // time is iso8601, with no timezone.
    sprintf(buf,"{\"time\":\"%s\",",timestr);
    buf += strlen(buf);
    sprintf(buf,"\"cpm\":%"PRIu32",\"duration\":30,",flash_log[log_position].cpm);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_start\":%d,\"accel_y_start\":%d,\"accel_z_start\":%d,",flash_log[log_position].accel_x_start,flash_log[log_position].accel_y_start,flash_log[log_position].accel_z_start);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_end\":%d,\"accel_y_end\":%d,\"accel_z_end\":%d}",flash_log[log_position].accel_x_end,flash_log[log_position].accel_y_end,flash_log[log_position].accel_z_end);
    buf += strlen(buf);

    // add trailing comma on all but last log entry.
    if(log_position != (logsize-1)) {
      buf[0] = ',';
      buf[1] = 0;
      buf+=1;
    }
  }
  buf[0]=0;

  log_position++;
  if((log_position==logsize) || ((log_position==1) && (logsize == 0))) {
    buf[0] = ']';
    buf[1] = '}';
    buf[2] = '\n';
    buf[3] = 0;
    buf+=3;
  }
  return strlen(buf_start);
}

// return size of data written on success or 0 on failure.
int log_read_csv(char *buf) {

  buf[0]=0;

  char *buf_start = buf;

  log_data_t *flash_log = (log_data_t *) flashstorage_log_get();
  uint32_t logsize = flashstorage_log_size()/sizeof(log_data_t);

  if (log_position==0) {	// print the header
    int64_t offset_mins = realtime_getutcoffset_mins();

    bool offset_is_valid = realtime_getutcoffset_available();

    char offset_string[10];
    bool offset_is_negative=false;
    if(offset_mins < 0) {
      offset_is_negative=true;
      offset_mins = 0-offset_mins;
    }

    if(offset_is_valid) {
      offset_string[0] =  (offset_is_negative) ? '-' : '+';
      sprintf(offset_string+1,"%02u",(int)offset_mins/60);
      sprintf(offset_string+3,"%02u",(int)offset_mins%60);
      offset_string[5] = 0;
    } else {
      offset_string[0] = 'u';
      offset_string[1] = 'n';
      offset_string[2] = 'd';
      offset_string[3] = 'e';
      offset_string[4] = 'f';
      offset_string[5] = 0;
    }

    sprintf(buf,"# Onyx CSV log: firmware: %s, records: %"PRIu32", UTC_offset: %s, user_calibration: %f\r\n# timestamp,cpm\r\n",OS100VERSION,logsize,offset_string,system_geiger->calibration_scaling);
    buf += strlen(buf);
  }

  if(log_position<logsize) {
    int64_t current_time = flash_log[log_position].time;

    struct tm *time;
    time_t current_time_u32 = current_time;
    time = gmtime(&current_time_u32);
    char timestr[200];
    sprintf(timestr,"%u-%02u-%02uT%02u:%02u:%02uZ",time->tm_year+1900,time->tm_mon+1,time->tm_mday,time->tm_hour,time->tm_min,time->tm_sec);

    // time is iso8601, with no timezone.
    sprintf(buf,"%s,%"PRIu32"\r\n",timestr,flash_log[log_position].cpm);
    buf += strlen(buf);
  }
  buf[0]=0;

  log_position++;
  return strlen(buf_start);
}

}
