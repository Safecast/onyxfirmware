#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include <stdint.h>

class Controller {

public:

  Controller();
  void set_gui(GUI &g);
  void update_calibration();
  void initialise_calibration();
  void save_loginterval();
  void save_counterwindow();
  void save_time();
  void save_date();
  void receive_gui_event(const char *event,const char *value);
  void update();
  void save_warncpm();

  void event_save_calibration();
  void event_sleep(const char *event,const char *value);
  void event_totaltimer();
  void event_japanese(const char *event,const char *value);
  void event_english(const char *event,const char *value);
  void event_cpm_cps_auto(const char *event,const char *value);
  void event_geiger_beep(const char *event,const char *value);
  void event_usv(const char *event,const char *value);
  void event_rem(const char *event,const char *value);
  void event_clear_log(const char *event,const char *value);
  void event_save_brightness(const char *event,const char *value);
  void event_save_becq(const char *event,const char *value);
  void event_save_pulsewidth(const char *event,const char *value);
  void event_save_utcoff(const char *event,const char *value);
  void event_utcscreen(const char *event,const char *value);
  void event_timescreen(const char *event,const char *value);
  void event_becqscreen(const char *event,const char *value);
  void event_loginterval();
  void event_getcountwin();
  void event_warnscreen(const char *event,const char *value);
  void event_datescreen(const char *event,const char *value);
  void event_brightnessscn(const char *event,const char *value);
  void event_leftbrightness(const char *event,const char *value);
  void event_neverdim(const char *event,const char *value);
  void event_varnum_brightness(const char *event,const char *value);
  void event_varnum_time(const char *event,const char *value);
  void event_varnum_date(const char *event,const char *value);
  void event_audioxfer(const char *event,const char *value);
  void event_qrtweet(const char *event,const char *value);
  void check_warning_level();
  void do_logging();
  void check_sleep_switch();
  void do_dimming();
  void send_cpm_values();
  void send_graph_data();
  void send_total_timer();
  void send_svrem();
  void send_becq();
  void send_logstatus();

  GUI     *m_gui;
  bool     m_sleeping;             ///< this indicates display sleep
  bool     m_powerup;
  float    m_calibration_base;
  uint32_t m_log_interval_seconds;
  bool     m_alarm_log;
  uint32_t m_last_alarm_time;
  uint32_t m_counts_stored;
  uint32_t m_interval_stored;
  int16    m_accel_x_stored;
  int16    m_accel_y_stored;
  int16    m_accel_z_stored;
  int16    m_magsensor_stored;
  int32_t  m_warncpm;
  bool     m_last_switch_state;
  bool     m_keytrigger;
  bool     m_warning_raised;
  uint32   m_total_timer_start;
  bool     m_dim_off;
  bool     m_never_dim;
  uint32_t m_last_cpm_sent_to_gui;
  uint32_t m_count_timer_max;

  float    m_cpm_cps_threshold;
  float    m_cps_cpm_threshold;
  bool     m_cpm_cps_switch;

};

#endif
