#ifndef GEIGER_H
#define GEIGER_H

#include <stdint.h>

#define WINDOWS_PER_SECOND 2
#define WINDOWS_PER_MIN    120
#define WINDOWS_STORED     600


class Geiger {

public:

  Geiger();
  void initialise();
  float get_cpm();
  float get_cpm30();
  float get_cpm_deadtime_compensated();
  float get_microsieverts();
  void  set_calibration(float c);

  float *get_cpm_last_windows();
  void powerup  ();
  void powerdown();
  float cpm_last_windows[WINDOWS_STORED];
  void update_last_windows();
  bool is_cpm_valid();
  bool is_cpm30_valid();
  void toggle_beep();
  void set_beep(bool b);
  bool is_beeping();
  void reset_total_count();
  uint32_t get_total_count();

  uint16_t last_windows_position;
  uint16_t last_windows[WINDOWS_STORED];
  uint16_t max_averaging_period;
  float    calibration_scaling;
  bool     m_acquire_and_log;
  uint16_t m_samples_collected;
  bool     m_cpm_valid;
};

extern Geiger *system_geiger;

#endif
