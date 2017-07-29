#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include "flashstorage.h"
#include "rtc.h"
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
  void init_dimdelay();

  void event_save_calibration();
  void event_sleep(const char *event,const char *value);
  void event_totaltimer();
  void event_japanese(const char *event,const char *value);
  void event_english(const char *event,const char *value);
  void event_cpm_cps_auto(const char *event,const char *value);
  void event_mute_alarm();
  void event_geiger_beep(const char *event,const char *value);
  void event_usv(const char *event,const char *value);
  void event_rem(const char *event,const char *value);
  void event_clear_log(const char *event,const char *value);
  void event_save_brightness(const char *event,const char *value);
  void event_save_becq(const char *event,const char *value);
  void event_save_utcoff(const char *event,const char *value);
  void event_utcscreen(const char *event,const char *value);
  void event_timescreen(const char *event,const char *value);
  void event_becqscreen(const char *event,const char *value);
  void event_loginterval();
  void event_getcountwin();
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
  void refresh_next_opmode_name();
  void send_mode_label();
  void event_next_opmode(bool jump);

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
  bool     m_last_switch_state;;
  bool     m_warning_raised;
  uint32   m_total_timer_start;
  bool     m_mute_alarm;
  bool     m_dim_off;
  bool     m_screen_dimmed;
  uint32_t m_last_cpm_sent_to_gui;
  uint32_t m_count_timer_max;

  uint32_t  m_cpm_cps_threshold;
  uint32_t  m_cps_cpm_threshold;
  bool 		m_displaying_cps;
  bool      m_cpm_cps_switch;

private:

  uint8_t	enabled_modes;	// What operating modes are enabled
  uint8_t   next_mode_label;
  uint32_t	qr_last_update;   // Used to limit the rate of QR code updates
  bool      m_neverdim;
  uint8_t 	dim_delay;		  // Delay before dimming
  void event_opmode(const char *event,uint8_t mode_val);
  void reset_alarm(int32_t warn_cpm);
  void event_warnscreen(const char *event,const char *value);
  void event_datescreen(const char *event,const char *value);
  void event_brightnessscn(const char *event,const char *value);
  void event_leftbrightness(const char *event,const char *value);
  void event_neverdim(const char *event,const char *value);
  void event_varnum_brightness(const char *event,const char *value);
  void event_varnum_time(const char *event,const char *value);
  void event_varnum_date(const char *event,const char *value);
  void event_audioxfer(const char *event,const char *value);
  void event_qrtweet();
  void event_pulse(uint16_t width);
  void event_sendcountwin();
  void event_screen_off();


};

extern Controller *system_controller;


#endif
