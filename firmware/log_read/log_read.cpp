/* place holder code for log_read API */
#include "log.h"
#include "log_read.h"
#include "flashstorage.h"
#include <stdio.h>
#include <string.h>

int log_position = 0;

extern "C" {

void log_read_start() {
  log_position=0;
}

// return 0 on failure.
// return size of data written
int log_read_block(char *buf) {
 
  buf[0]=0;
 
  char *buf_start = buf;

  log_data_t *flash_log = (log_data_t *) flashstorage_log_get();
  uint32_t logsize = flashstorage_log_size()/sizeof(log_data_t);

  if(log_position==0) {
    sprintf(buf,"{\"log_size\":%u,\r\n\"log_data\":[\r\n",logsize);
    buf += strlen(buf);
  }

  if(log_position<logsize) {
    sprintf(buf,"{\"unixtime\":%u,\"cpm\":%u,",flash_log[log_position].time,flash_log[log_position].cpm);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_start\":%d,\"accel_y_start\":%d,\"accel_z_start\":%d,",flash_log[log_position].accel_x_start,flash_log[log_position].accel_y_start,flash_log[log_position].accel_z_start);
    buf += strlen(buf);
    sprintf(buf,"\"accel_x_end\":%d,\"accel_y_end\":%d,\"accel_z_end\":%d}\r\n",flash_log[log_position].accel_x_end,flash_log[log_position].accel_y_end,flash_log[log_position].accel_z_end);
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
    buf[2] = 0;
    buf+=3;
  }
  return strlen(buf_start);
}

}
