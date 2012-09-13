#ifndef GEIGER_H
#define GEIGER_H

#include <stdint.h>

#define COUNTS_PER_SECOND 2
#define COUNTS_PER_MIN    120

class Geiger {

public:

  Geiger();
  void initialise();
  float get_cpm();
  float get_cpm_deadtime_compensated();
  float get_microsieverts();
  void  set_calibration(float c);

  float *get_cpm_last_min();
  void powerup  ();
  void powerdown();
  float cpm_last_min[COUNTS_PER_MIN];
  void update_last_min();
  bool is_cpm_valid();

  uint16_t last_min_position;
  uint16_t last_min[COUNTS_PER_MIN];
  uint16_t averaging_period;
  float    calibration_scaling;
  bool     m_acquire_and_log;
  uint16_t m_samples_collected;
};

#endif
