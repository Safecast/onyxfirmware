/* place holder code for log_read API */
#include "log.h"
#include "log_read.h"
#include "flashstorage.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int log_position = 0;

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
    sprintf(buf,"{\"log_size\":%u,\"onyx_version\":%s,\"log_data\":[",logsize,OS100VERSION);
    buf += strlen(buf);
  }

  if(log_position<logsize) {
    time_t current_time = flash_log[log_position].time;

    struct tm *time;
    time = gmtime(&current_time);
    char timestr[50];
    sprintf(timestr,"%u-%02u-%02uT%02u:%02u:%02u",time->tm_year+1900,time->tm_mon,time->tm_mday,time->tm_hour,time->tm_min,time->tm_sec);

    // time is iso8601, with no timezone.
    sprintf(buf,"{\"time\":\"%s\",",timestr);
    buf += strlen(buf);
    sprintf(buf,"\"cpm\":%u,\"duration\":30,",flash_log[log_position].cpm);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_start\":%d,\"accel_y_start\":%d,\"accel_z_start\":%d,",flash_log[log_position].accel_x_start,flash_log[log_position].accel_y_start,flash_log[log_position].accel_z_start);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_end\":%d,\"accel_y_end\":%d,\"accel_z_end\":%d}",flash_log[log_position].accel_x_end,flash_log[log_position].accel_y_end,flash_log[log_position].accel_z_end);
    buf += strlen(buf);

    // add trailing comma on all but last log entry.
    if(log_position != (logsize-1)) {
      buf[0] = ',';
      buf[1] = 0;
      buf+=2;
    }
  }

  log_position++;
  if((log_position==logsize) || ((log_position==1) && (logsize == 0))) {
    buf[0] = ']';
    buf[1] = '}';
    buf[2] = '\n';
    buf[3] = 0;
    buf+=4;
  }
  return strlen(buf_start);
}

}
