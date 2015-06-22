#ifndef GEIGER_H
#define GEIGER_H

#include <stdint.h>


// We measure pulses from the tube at 2 Hertz
#define WINDOWS_PER_SECOND 2
#define WINDOWS_PER_MIN    120
#define WINDOWS_STORED     600

#define MAX_CPM            350000

// Number of CPM values we keep in the history buffer
// Shall be less than or equal to 128 because we use
// this to draw the graph and the screen is 128 pixels wide
#define CPM_HISTORY_SIZE		128

// Constants for headphone pulse output
// (also used in Controller.cpp
#define PULSE_NONE   0
#define PULSE_10us   1
#define PULSE_1ms    2
#define PULSE_AUDIO  3

class Geiger {

public:

  Geiger();
  void initialise();
  float get_cpm_deadtime_compensated();
  float get_cpm30_deadtime_compensated();
  float get_microsieverts();
  float get_microsieverts_nocal();
  float get_microrems();
  void  set_calibration(float c);
  float get_calibration();
  void update_last_windows();
  float* get_cpm_history();
  void powerup  ();
  void powerdown();
  bool is_cpm_valid();
  bool is_cpm30_valid();
  void toggle_beep();
  void set_beep(bool b);
  bool is_beeping();
  void reset_total_count();
  uint32_t get_total_count();
  float get_becquerel();
  void  set_becquerel_eff(float v);
  void enable_micout();
  void disable_micout();
  void enable_headphones(bool en);
  void set_pulsewidth(uint32_t p);
  uint32_t get_pulsewidth();
  bool pulse_triggered();

  float    cpm_history[CPM_HISTORY_SIZE];
  float    calibration_scaling;

private:
  bool     m_cpm_valid;
  uint8_t  cpm_history_position;
  uint16_t last_windows_position;
  uint16_t last_windows[WINDOWS_STORED];
  uint16_t max_averaging_period;
  bool     m_acquire_and_log;
  uint16_t m_samples_collected;
  float    m_becquerel_eff;
  float    cpm_history_p[CPM_HISTORY_SIZE];

  void pulse_timer_init();
  float get_cpm30();
  float calc_cpm();  // Calculate the CPM and store in the object
  float calc_cpm_deadtime_compensated();
  float current_cpm_deadtime_compensated;

};

extern Geiger *system_geiger;

#endif
