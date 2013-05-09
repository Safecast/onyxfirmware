#ifndef LOG_H
#define LOG_H

#include <stdint.h>

struct log_data_t {
  uint32_t time;
  uint32_t cpm;		// FIXME: this may need to be re-evaluated
//  uint32_t interval;
//  uint32_t counts;
  int16_t  accel_x_start;
  int16_t  accel_y_start;
  int16_t  accel_z_start;
  int16_t  accel_x_end;
  int16_t  accel_y_end;
  int16_t  accel_z_end;
//  int16_t  magsensor_start;
//  int16_t  magsensor_end;
  uint32_t log_type;
};

#endif
