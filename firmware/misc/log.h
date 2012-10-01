#ifndef LOG_H
#define LOG_H

struct log_data_t {
  uint32_t time;
  uint32_t cpm;
  int16_t  accel_x_start;
  int16_t  accel_y_start;
  int16_t  accel_z_start;
  int16_t  accel_x_end;
  int16_t  accel_y_end;
  int16_t  accel_z_end;
  uint8_t  log_type;
};

#endif
