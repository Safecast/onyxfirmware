#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include <stdint.h>

class Controller {

public:

  Controller(Geiger &g);
  void set_gui(GUI &g);
  void update_calibration();
  void save_calibration();
  void initialise_calibration();
  void save_time();
  void save_date();
  void receive_gui_event(char *event,char *value);
  void update();
  void save_warncpm();


  GUI     *m_gui;
  Geiger  &m_geiger;
  bool     m_sleeping;             ///< this indicates display sleep
  bool     m_powerup;
  float    m_calibration_base;
  uint32_t m_log_interval_seconds;
  bool     m_alarm_log;
  uint32_t m_last_alarm_time;
  int16    m_accel_x_stored;
  int16    m_accel_y_stored;
  int16    m_accel_z_stored;
  int32_t  m_warncpm;
  bool     m_last_switch_state;
  bool     m_keytrigger;
  bool     m_warning_raised;
  uint32   m_total_timer_start;
  bool     m_changing_brightness;

  float    m_cpm_cps_threshold;
  float    m_cps_cpm_threshold;
  bool     m_cpm_cps_switch;
  int      m_current_units;
};

#endif
