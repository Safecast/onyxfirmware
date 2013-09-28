#include "Controller.h"
#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"
#include "realtime.h"
#include "flashstorage.h"
#include "rtc.h"
#include "power.h"
#include <stdio.h>
#include "accel.h"
#include "log.h"
#include "switch.h"
#include "qr_xfer.h"
#include "buzzer.h"
#include <string.h>
#include <limits.h>
#include "modem.h"
#include <stdint.h>
#include <inttypes.h>
#include "gpio.h"
#include "safecast_config.h"

#define UNITS_CPS 1
#define UNITS_CPM 2

Controller *system_controller;

/**
 * Initialize the system controller. In particular:
 *   - Load settings
 *   - Setup the log interval alarm.
 */
Controller::Controller() {

  m_sleeping=false;
  m_powerup=false;
  m_log_interval_seconds = 60*30;
  rtc_clear_alarmed();
  rtc_enable_alarm(RTC);
  m_interval_stored = rtc_get_time(RTC);
  m_counts_stored = 0;
  m_alarm_log = false;
  system_controller = this;
  m_last_switch_state = true;
  m_never_dim = false;

  bool sstate = switch_state();
  m_last_switch_state = sstate;

  m_warning_raised = false;
  m_dim_off = false;

  m_cpm_cps_switch = false;
  m_current_units = 2;
  m_cpm_cps_threshold = 1100.0;
  m_cps_cpm_threshold = 1000.0;

  // Get warning cpm from flash
  m_warncpm = 0;
  const char *swarncpm = flashstorage_keyval_get("WARNCPM");
  if(swarncpm != 0) {
    int32_t c;
    sscanf(swarncpm, "%"PRIu32"", &c);
    m_warncpm = c;
  }

  // Get never dim from flash
  m_warncpm = -1;
  const char *sneverdim = flashstorage_keyval_get("NEVERDIM");
  if(sneverdim != 0) {
    if(strcmp(sneverdim,"true") == 0) m_never_dim=true;
                                 else m_never_dim=false;
  } else m_never_dim=false;

  if(m_never_dim) tick_item("Never Dim" ,true);

  // Get logging interval from flash
  const char *sloginter = flashstorage_keyval_get("LOGINTERVAL");
  if(sloginter != 0) {
    int32_t c;
    sscanf(sloginter, "%"PRIu32"", &c);
    m_log_interval_seconds = c;
  } else {
    m_log_interval_seconds = 30*60;
  }
  rtc_set_alarm(RTC,rtc_get_time(RTC)+m_log_interval_seconds);
}

void Controller::set_gui(GUI &g) {
  m_gui = &g;
}

void Controller::update_calibration() {
  int c1 = m_gui->get_item_state_uint8("CAL1");
  int c2 = m_gui->get_item_state_uint8("CAL2");
  int c3 = m_gui->get_item_state_uint8("CAL3");
  int c4 = m_gui->get_item_state_uint8("CAL4");
  float calibration_scaling = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);

  char text_sieverts[50];
  float_to_char(m_calibration_base*calibration_scaling,text_sieverts,5);
  text_sieverts[5] = ' ';
  text_sieverts[6] = '\x80';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;
  m_gui->receive_update("FIXEDSV",text_sieverts);
}

void Controller::event_save_calibration() {
  int c1 = m_gui->get_item_state_uint8("CAL1");
  int c2 = m_gui->get_item_state_uint8("CAL2");
  int c3 = m_gui->get_item_state_uint8("CAL3");
  int c4 = m_gui->get_item_state_uint8("CAL4");
  float calibration_scaling = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);
  float base_sieverts = system_geiger->get_microsieverts_nocal();

  char text_sieverts[50];
  float_to_char(base_sieverts*calibration_scaling,text_sieverts,5);
  text_sieverts[5] = ' ';
  text_sieverts[6] = '\x80';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;

  m_gui->receive_update("Sieverts",text_sieverts);
  system_geiger->set_calibration(calibration_scaling);
  m_dim_off=false;
  m_gui->jump_to_screen(0);
}

void Controller::initialise_calibration() {
  m_dim_off=true;
  display_set_brightness(15);
  m_calibration_base = system_geiger->get_microsieverts_nocal();
  char text_sieverts[50];
  float_to_char(m_calibration_base*system_geiger->get_calibration(),text_sieverts,5);
  text_sieverts[5] = ' ';
  text_sieverts[6] = '\x80';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;
  m_gui->receive_update("FIXEDSV",text_sieverts);

  uint8_t c1=system_geiger->get_calibration();
  uint8_t c2=((uint32_t)(system_geiger->get_calibration()*10))%10;
  uint8_t c3=((uint32_t)(system_geiger->get_calibration()*100))%10;
  uint8_t c4=((uint32_t)(system_geiger->get_calibration()*1000))%10;
  m_gui->receive_update("CAL1",&c1);
  m_gui->receive_update("CAL2",&c2);
  m_gui->receive_update("CAL3",&c3);
  m_gui->receive_update("CAL4",&c4);
}

void Controller::save_warncpm() {
  m_warning_raised = false;
  int w1 = m_gui->get_item_state_uint8("WARNCPM1");
  int w2 = m_gui->get_item_state_uint8("WARNCPM2");
  int w3 = m_gui->get_item_state_uint8("WARNCPM3");
  int w4 = m_gui->get_item_state_uint8("WARNCPM4");
  int w5 = m_gui->get_item_state_uint8("WARNCPM5");

  int32_t warn_cpm = 0;
  warn_cpm += w1*10000;
  warn_cpm += w2*1000;
  warn_cpm += w3*100;
  warn_cpm += w4*10;
  warn_cpm += w5*1;

  m_warncpm = warn_cpm;

  char swarncpm[50];
  sprintf(swarncpm,"%"PRIi32"",warn_cpm);
  flashstorage_keyval_set("WARNCPM",swarncpm);

  m_gui->jump_to_screen(0);
}

void Controller::save_loginterval() {
  int l1 = m_gui->get_item_state_uint8("LOGINTER1");
  int l2 = m_gui->get_item_state_uint8("LOGINTER2");
  int l3 = m_gui->get_item_state_uint8("LOGINTER3");
  int32_t log_interval_mins = (l1*100) + (l2*10) + l3;
  m_log_interval_seconds = log_interval_mins*60;

  char sloginterval[50];
  sprintf(sloginterval,"%"PRIu32"",m_log_interval_seconds);
  flashstorage_keyval_set("LOGINTERVAL",sloginterval);
  uint32_t current_time = realtime_get_unixtime();
  rtc_set_alarm(RTC,current_time+m_log_interval_seconds);
  m_gui->jump_to_screen(0);

}

void Controller::save_time() {
  int h1 = m_gui->get_item_state_uint8("TIMEHOUR1");
  int h2 = m_gui->get_item_state_uint8("TIMEHOUR2");
  int m1 = m_gui->get_item_state_uint8("TIMEMIN1");
  int m2 = m_gui->get_item_state_uint8("TIMEMIN2");
  int s1 = m_gui->get_item_state_uint8("TIMESEC1");
  int s2 = m_gui->get_item_state_uint8("TIMESEC2");

  int new_hours = h2 + (h1*10);
  int new_min   = m2 + (m1*10);
  int new_sec   = s2 + (s1*10);

  uint8_t hours,min,sec,day,month;
  uint16_t year;
  realtime_getdate(hours,min,sec,day,month,year);
  hours = new_hours;
  min   = new_min;
  sec   = new_sec;
  realtime_setdate(hours,min,sec,day,month,year);
  rtc_set_alarm(RTC,rtc_get_time(RTC)+m_log_interval_seconds);

  flashstorage_log_userchange();
  m_gui->jump_to_screen(0);
}

void Controller::save_date() {
  int d1 = m_gui->get_item_state_uint8("DATEDAY1");
  int d2 = m_gui->get_item_state_uint8("DATEDAY2");
  int m1 = m_gui->get_item_state_uint8("DATEMON1");
  int m2 = m_gui->get_item_state_uint8("DATEMON2");
  int y1 = m_gui->get_item_state_uint8("DATEYEAR1");
  int y2 = m_gui->get_item_state_uint8("DATEYEAR2");

  int new_day  = d2 + (d1*10);
  int new_mon  = m2 + (m1*10);
  int new_year = y2 + (y1*10);

  uint8_t hours,min,sec,day,month;
  uint16_t year;
  realtime_getdate(hours,min,sec,day,month,year);
  day   = new_day;
  month = new_mon-1;
  year  = (2000+new_year)-1900;
  realtime_setdate(hours,min,sec,day,month,year);
  rtc_set_alarm(RTC,rtc_get_time(RTC)+m_log_interval_seconds);

  flashstorage_log_userchange();
  m_gui->jump_to_screen(0);
}


bool is_leap(int month,int day,int year) {
  if((year % 400) == 0) return true;

  if((year % 100) == 0) return false;

  if((year % 4)   == 0) return true;

  return false;
}

/**
 * Put the device in sleep mode (will wakeup with
 * standby switch toggle, or RTC alarm for logging)
 */
void Controller::event_sleep(const char *event,const char *value) {
  #ifndef NEVERSLEEP
  if(m_sleeping == false) {
    display_powerdown();
    m_sleeping=true;
    m_gui->set_key_trigger();
    m_gui->set_sleeping(true);
    display_powerdown();
    power_standby();
  }
  #endif
}

void Controller::event_totaltimer(const char *event,const char *value) {
  system_geiger->reset_total_count();
  m_total_timer_start = realtime_get_unixtime();

  const char *blank = "              ";
  m_gui->receive_update("TTCOUNT",blank);
  m_gui->receive_update("TTTIME" ,blank);
  m_gui->redraw();
}

void Controller::event_save_pulsewidth(const char *event,const char *value) {
  int p1 = m_gui->get_item_state_uint8("PULSEWIDTH1");
  char sp[50];
  sprintf(sp,"%d",p1);
  flashstorage_keyval_set("PULSEWIDTH",sp);
  system_geiger->set_pulsewidth(p1);
  system_geiger->pulse_timer_init();
  m_gui->jump_to_screen(0);
}

void Controller::event_save_becq(const char *event,const char *value) {
  int b1 = m_gui->get_item_state_uint8("BECQ1");
  int b2 = m_gui->get_item_state_uint8("BECQ2");
  int b3 = m_gui->get_item_state_uint8("BECQ3");
  int b4 = m_gui->get_item_state_uint8("BECQ4");

  float beff = b1*1000 + b2*100 + b3*10 + b4;
  system_geiger->set_becquerel_eff(beff);
  m_gui->jump_to_screen(0);
}

void Controller::event_save_utcoff(const char *event,const char *value) {
    int h1 = m_gui->get_item_state_uint8("OFFHOUR1");
    int h2 = m_gui->get_item_state_uint8("OFFHOUR2");
    int m1 = m_gui->get_item_state_uint8("OFFMIN1");
    int m2 = m_gui->get_item_state_uint8("OFFMIN2");

    int utcoffset = (((h1*10)+h2)*60) + (m1*10) + m2;
    if(m_gui->get_item_state_uint8("SIGN:-,+,") == 0) {
      utcoffset = 0-utcoffset;
    }
    realtime_setutcoffset_mins(utcoffset);

    char sutcoffset[50];
    sprintf(sutcoffset,"%d",utcoffset);
    flashstorage_keyval_set("UTCOFFSETMINS",sutcoffset);

    m_gui->jump_to_screen(0);
}

void Controller::event_neverdim(const char *event,const char *value) {
    if(m_never_dim == false) {
      flashstorage_keyval_set("NEVERDIM","true");
      tick_item("Never Dim" ,true);
      m_never_dim=true;
    } else {
      flashstorage_keyval_set("NEVERDIM","false");
      tick_item("Never Dim" ,false);
      m_never_dim=false;
    }
}

void Controller::event_japanese(const char *event,const char *value) {
    m_gui->set_language(LANGUAGE_JAPANESE);
    flashstorage_keyval_set("LANGUAGE","Japanese");
    tick_item("English" ,false);
    tick_item("Japanese",true);
}

void Controller::event_english(const char *event,const char *value) {
    m_gui->set_language(LANGUAGE_ENGLISH);
    flashstorage_keyval_set("LANGUAGE","English");
    tick_item("English" ,true);
    tick_item("Japanese",false);
}

void Controller::event_cpm_cps_auto(const char *event,const char *value) {
    if(m_cpm_cps_switch == false) {
      m_cpm_cps_switch = true;
      flashstorage_keyval_set("CPMCPSAUTO","true");
      tick_item("CPM/CPS Auto",true);
    } else {
      m_cpm_cps_switch = false;
      flashstorage_keyval_set("CPMCPSAUTO","false");
      tick_item("CPM/CPS Auto",false);
    }
}

void Controller::event_geiger_beep(const char *event,const char *value) {
  system_geiger->toggle_beep();
  if(system_geiger->is_beeping()) { flashstorage_keyval_set("GEIGERBEEP","true");  tick_item("Geiger Beep",true);  }
                       else { flashstorage_keyval_set("GEIGERBEEP","false"); tick_item("Geiger Beep",false); }
}

void Controller::event_usv(const char *event,const char *value) {
  flashstorage_keyval_set("SVREM","SV");
  tick_item("\x80Sv" ,true);
  tick_item("\x80R",false);
}

void Controller::event_rem(const char *event,const char *value) {
  flashstorage_keyval_set("SVREM","REM");
  tick_item("\x80Sv" ,false);
  tick_item("\x80R",true);
}

/**
 * Clears the event log. TODO: ask for confirmation
 */
void Controller::event_clear_log(const char *event,const char *value) {
    flashstorage_log_clear();
    m_gui->show_dialog("Log Cleared",0,0,0,0,48,254,254,254);
}

void Controller::event_save_brightness(const char *event,const char *value) {
    uint8 b = m_gui->get_item_state_uint8("BRIGHTNESS");

    int br;
    if(b<= 5) br = (b*2) +1;
    if(b>  5) br = b+6;
    display_set_brightness(br);

    char sbright[50];
    sprintf(sbright,"%u",br);
    flashstorage_keyval_set("BRIGHTNESS",sbright);

    m_dim_off=false;
    m_gui->jump_to_screen(0);
}

void Controller::event_utcscreen(const char *event,const char *value) {
    int offset = realtime_getutcoffset_mins();
    if(offset < 0) offset = 0-offset;

    int hours = offset/60;
    int min   = offset-(hours*60);

    uint8_t h1,h2,m1,m2;
    h1 = hours/10;
    h2 = hours%10;
    m1 = min/10;
    m2 = min%10;

    m_gui->receive_update("OFFHOUR1",&h1);
    m_gui->receive_update("OFFHOUR2",&h2);
    m_gui->receive_update("OFFMIN1" ,&m1);
    m_gui->receive_update("OFFMIN2" ,&m2);

    uint8 zero=0;
    uint8 one =1;

    offset = realtime_getutcoffset_mins();
    if(offset <= 0) m_gui->receive_update("SIGN:-,+,",&zero);
               else m_gui->receive_update("SIGN:-,+,",&one);
    m_gui->redraw();
}

void Controller::event_timescreen(const char *event,const char *value) {
    uint8_t hours;
    uint8_t min;
    uint8_t sec;
    uint8_t day;
    uint8_t month;
    uint16_t year;

    realtime_getdate(hours,min,sec,day,month,year);

    uint8 h1,h2,m1,m2,s1,s2;
    h1 = hours/10;
    h2 = hours%10;
    m1 = min/10;
    m2 = min%10;
    s1 = sec/10;
    s2 = sec%10;

    m_gui->receive_update("TIMEHOUR1",&h1);
    m_gui->receive_update("TIMEHOUR2",&h2);
    m_gui->receive_update("TIMEMIN1" ,&m1);
    m_gui->receive_update("TIMEMIN2" ,&m2);
    m_gui->receive_update("TIMESEC1" ,&s1);
    m_gui->receive_update("TIMESEC2" ,&s2);
    m_gui->redraw();
}

void Controller::event_becqscreen(const char *event,const char *value) {
    float becq_val = 0;
    const char *val = flashstorage_keyval_get("BECQEFF");
    if(val != NULL) {
      sscanf(val,"%f",&becq_val);
    }

    uint8_t b1 = ((int)becq_val%10000) /1000;
    uint8_t b2 = ((int)becq_val%1000)  /100;
    uint8_t b3 = ((int)becq_val%100)   /10;
    uint8_t b4 = ((int)becq_val%10);

    m_gui->receive_update("BECQ1",&b1);
    m_gui->receive_update("BECQ2",&b2);
    m_gui->receive_update("BECQ3",&b3);
    m_gui->receive_update("BECQ4",&b4);
    m_gui->redraw();
}

void Controller::event_loginterval(const char *event,const char *value) {
    int32_t log_interval = 0;
    const char *val = flashstorage_keyval_get("LOGINTERVAL");
    if (val != NULL) {
      sscanf(val,"%"PRIi32"",&log_interval);
    } else {
      log_interval = 30*60;
    }

    log_interval = log_interval/60; // Turn it into minutes

    uint8_t l1 = (log_interval%1000)/100;
    uint8_t l2 = (log_interval%100) /10;
    uint8_t l3 = (log_interval%10)  /1;

    m_gui->receive_update("LOGINTER1",&l1);
    m_gui->receive_update("LOGINTER2",&l2);
    m_gui->receive_update("LOGINTER3",&l3);
    m_gui->redraw();
}

void Controller::event_warnscreen(const char *event,const char *value) {
    int32_t warn_level = 0;
    const char *val = flashstorage_keyval_get("WARNCPM");
    if(val != NULL) {
      sscanf(val,"%"PRIi32"",&warn_level);
    }

    uint8_t w1 = (warn_level%100000)/10000;
    uint8_t w2 = (warn_level%10000) /1000;
    uint8_t w3 = (warn_level%1000)  /100;
    uint8_t w4 = (warn_level%100)   /10;
    uint8_t w5 = (warn_level%10)    /1;

    m_gui->receive_update("WARNCPM1",&w1);
    m_gui->receive_update("WARNCPM2",&w2);
    m_gui->receive_update("WARNCPM3",&w3);
    m_gui->receive_update("WARNCPM4",&w4);
    m_gui->receive_update("WARNCPM5",&w5);
    m_gui->redraw();
}

void Controller::event_datescreen(const char *event,const char *value) {
    uint8_t hours;
    uint8_t min;
    uint8_t sec;
    uint8_t day;
    uint8_t month;
    uint16_t year;

    realtime_getdate(hours,min,sec,day,month,year);

    month+=1;

    year  = (year+1900)-2000;

    uint8 m1,m2,d1,d2,y1,y2;
    d1 = day/10;
    d2 = day%10;
    m1 = month/10;
    m2 = month%10;
    y1 = year/10;
    y2 = year%10;

    m_gui->receive_update("DATEMON1",&m1);
    m_gui->receive_update("DATEMON2",&m2);
    m_gui->receive_update("DATEDAY1",&d1);
    m_gui->receive_update("DATEDAY2",&d2);
    m_gui->receive_update("DATEYEAR1",&y1);
    m_gui->receive_update("DATEYEAR2",&y2);
    m_gui->redraw();
}

void Controller::event_brightnessscn(const char *event,const char *value) {
    const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
    unsigned int c=15;
    if(sbright != 0) {
      sscanf(sbright, "%u", &c);
      display_set_brightness(c);
    }

    uint8 b;
    if(c <= 11) b = (c-1)/2;
    if(c >  11) b = c-6;

    m_gui->receive_update("BRIGHTNESS",&b);
    m_gui->redraw();
}

void Controller::event_leftbrightness(const char *event,const char *value) {
    const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
    if(sbright != 0) {
      unsigned int c;
      sscanf(sbright, "%u", &c);
      display_set_brightness(c);
    }
    m_dim_off=false;
}

void Controller::event_varnum_brightness(const char *event,const char *value) {
      int b = m_gui->get_item_state_uint8("BRIGHTNESS");
      m_dim_off=true;

      int br;
      if(b<= 5) br = (b*2) +1;
      if(b>  5) br = b+6;
      display_set_brightness(br);
}

void Controller::event_varnum_date(const char *event,const char *value) {
      int d1 = m_gui->get_item_state_uint8("DATEDAY1");
      int d2 = m_gui->get_item_state_uint8("DATEDAY2");
      int m1 = m_gui->get_item_state_uint8("DATEMON1");
      int m2 = m_gui->get_item_state_uint8("DATEMON2");
      int y1 = m_gui->get_item_state_uint8("DATEYEAR1");
      int y2 = m_gui->get_item_state_uint8("DATEYEAR2");

      if((m1 == 0) && (m2 == 0)) m2 = 1;
      if((d1 == 0) && (d2 == 0)) d2 = 1;

      if((m1 >= 1) && (m2 > 2)) { m1 = 1; m2 = 2; }

      uint8 month = m1*10 + m2;
      uint8 day   = d1*10 + d2;
      int year  = 2000+((y1*10) + y2);
      if((month == 1 ) && (day > 31)) { d1 = 3; d2 = 1; } // Jan
      if((month == 2 ) && (day > 29)) { d1 = 2; d2 = 9; } // Feb
      if((month == 3 ) && (day > 31)) { d1 = 3; d2 = 1; } // March
      if((month == 4 ) && (day > 30)) { d1 = 3; d2 = 0; } // April
      if((month == 5 ) && (day > 31)) { d1 = 3; d2 = 1; } // May
      if((month == 6 ) && (day > 30)) { d1 = 3; d2 = 0; } // June
      if((month == 7 ) && (day > 31)) { d1 = 3; d2 = 1; } // July
      if((month == 8 ) && (day > 31)) { d1 = 3; d2 = 1; } // Aug
      if((month == 9 ) && (day > 30)) { d1 = 3; d2 = 0; } // Sept
      if((month == 10) && (day > 31)) { d1 = 3; d2 = 1; } // Oct
      if((month == 11) && (day > 30)) { d1 = 3; d2 = 0; } // Nov
      if((month == 12) && (day > 31)) { d1 = 3; d2 = 1; } // Dec

      if(is_leap(month,day,year) && (month == 2) && (day > 28) ) { d1 = 2; d2 = 8; } // Feb

      m_gui->receive_update("DATEMON1",&m1);
      m_gui->receive_update("DATEMON2",&m2);
      m_gui->receive_update("DATEDAY1",&d1);
      m_gui->receive_update("DATEDAY2",&d2);
}

void Controller::event_varnum_time(const char *event,const char *value) {
      uint8 h1 = m_gui->get_item_state_uint8("TIMEHOUR1");
      uint8 h2 = m_gui->get_item_state_uint8("TIMEHOUR2");
      uint8 m1 = m_gui->get_item_state_uint8("TIMEMIN1");
      uint8 m2 = m_gui->get_item_state_uint8("TIMEMIN2");
      uint8 s1 = m_gui->get_item_state_uint8("TIMESEC1");
      uint8 s2 = m_gui->get_item_state_uint8("TIMESEC2");

      uint8 h = (h1*10)+h2;
      uint8 m = (m1*10)+m2;
      uint8 s = (s1*10)+s2;
      if(h > 23) {h1 = 2; h2=3;}
      if(m > 59) {m1 = 5; m2=9;}
      if(s > 59) {s1 = 5; s2=9;}

      m_gui->receive_update("TIMEHOUR1",&h1);
      m_gui->receive_update("TIMEHOUR2",&h2);
      m_gui->receive_update("TIMEMIN1",&m1);
      m_gui->receive_update("TIMEMIN2",&m2);
      m_gui->receive_update("TIMESEC1",&s1);
      m_gui->receive_update("TIMESEC2",&s2);
}

void Controller::event_audioxfer(const char *event,const char *value) {
    display_clear(0);
    modem_full_range = false;
    display_draw_text(0,16," Audio Transfer ",65535);
    display_draw_text(4,32,"  in progress  " ,65535);
    modem_logxfer();
    m_gui->jump_to_screen(0);
}

void Controller::event_qrtweet(const char *event,const char *value) {
    char str[1024];

    if(system_geiger->is_cpm_valid()) {
                 //12345678901234567890123456789012345    1   2 34567890
      sprintf(str,"http://twitter.com/home?status=CPM:%u%%23scast",(int)system_geiger->get_cpm_deadtime_compensated());
    } else {
                 //12345678901234567890123456789012345    1   2 34567890
      sprintf(str,"http://twitter.com/home?status=CPM:%u%%23bad",(int)system_geiger->get_cpm_deadtime_compensated());
    }
    qr_draw(str);
}

/**
 * Called by the GUI when a key event is received, triggers an action in the
 * controller.
 *
 */
void Controller::receive_gui_event(const char *event,const char *value) {
  if(m_sleeping) return;

  //TODO: Fix this total mess, refactor into switch, break conditions out into methods.
  if(strcmp(event,"Sleep"          ) == 0) event_sleep(event,value);           else
  if(strcmp(event,"KEYPRESS"       ) == 0) m_powerup=true;                     else
  if(strcmp(event,"TOTALTIMER"     ) == 0) event_totaltimer(event,value);      else
  if(strcmp(event,"Save:PulseWidth") == 0) event_save_pulsewidth(event,value); else
  if(strcmp(event,"Save:Calib"     ) == 0) event_save_calibration();           else
  if(strcmp(event,"Save:Becq"      ) == 0) event_save_becq(event,value);       else
  if(strcmp(event,"Save:UTCOff"    ) == 0) event_save_utcoff(event,value);     else
  if(strcmp(event,"Save:Time"      ) == 0) save_time();                        else
  if(strcmp(event,"Save:Date"      ) == 0) save_date();                        else
  if(strcmp(event,"Save:WarnCPM"   ) == 0) save_warncpm();                     else
  if(strcmp(event,"Japanese"       ) == 0) event_japanese(event,value);        else
  if(strcmp(event,"Never Dim"      ) == 0) event_neverdim(event,value);        else
  if(strcmp(event,"English"        ) == 0) event_english(event,value);         else
  if(strcmp(event,"CPM/CPS Auto"   ) == 0) event_cpm_cps_auto(event,value);    else
  if(strcmp(event,"Geiger Beep"    ) == 0) event_geiger_beep(event,value);     else
  if(strcmp(event,"\x80Sv"         ) == 0) event_usv(event,value);             else
  if(strcmp(event,"\x80R"          ) == 0) event_rem(event,value);             else
  if(strcmp(event,"Clear Log"      ) == 0) event_clear_log(event,value);       else
  if(strcmp(event,"Save:Brightness") == 0) event_save_brightness(event,value); else
  if(strcmp(event,"Save:LogInter"  ) == 0) save_loginterval();                 else
  if(strcmp(event,"CALIBRATE"      ) == 0) initialise_calibration();           else
  if(strcmp(event,"UTCSCREEN"      ) == 0) event_utcscreen(event,value);       else
  if(strcmp(event,"TIMESCREEN"     ) == 0) event_timescreen(event,value);      else
  if(strcmp(event,"BECQSCREEN"     ) == 0) event_becqscreen(event,value);      else
  if(strcmp(event,"LOGINTERVAL"    ) == 0) event_loginterval(event,value);     else
  if(strcmp(event,"WARNSCREEN"     ) == 0) event_warnscreen(event,value);      else
  if(strcmp(event,"DATESCREEN"     ) == 0) event_datescreen(event,value);      else
  if(strcmp(event,"BrightnessSCN"  ) == 0) event_brightnessscn(event,value);   else
  if(strcmp(event,"LeftBrightness" ) == 0) event_leftbrightness(event,value);  else
  if(strcmp(event,"QR Transfer"    ) == 0) qr_logxfer();                       else
  if(strcmp(event,"Audio Xfer"     ) == 0) event_audioxfer(event,value);       else
  if(strcmp(event,"QR Tweet"       ) == 0) event_qrtweet(event,value);         else
  if(strcmp(event,"varnumchange"   ) == 0) {
    if(strcmp ("BRIGHTNESS",value) == 0) event_varnum_brightness(event,value); else
    if(strcmpl("CAL"       ,value,3)   ) update_calibration();                 else
    if(strcmpl("DATE"      ,value,4)   ) event_varnum_date(event,value);       else
    if(strcmpl("TIME"      ,value,4)   ) event_varnum_time(event,value);
  }
}

void Controller::check_warning_level() {

  if((m_warncpm > 0) && (system_geiger->get_cpm_deadtime_compensated() >= m_warncpm) && (m_warning_raised == false) && system_geiger->is_cpm_valid()) {
    if(m_sleeping) display_powerup();
    char text_cpm[20];
    sprintf(text_cpm,"%8.3f",system_geiger->get_cpm_deadtime_compensated());
    m_gui->show_dialog("WARNING LEVEL","EXCEEDED",text_cpm,"CPM",true,42,254,255,255);
    m_warning_raised = true;

    #ifndef NEVERSLEEP
    if(m_sleeping) display_powerdown();
    #endif
  }
}

/**
 * Append a new entry to the log if necessary. Checks the
 * state of m_alarm_log to determine if we should append a log
 * entry.
 */
void Controller::do_logging() {
  if(rtc_alarmed()) {
    m_alarm_log = true;
    m_last_alarm_time = rtc_get_time(RTC);
    #ifndef DISABLE_ACCEL
    accel_read_state(&m_accel_x_stored,&m_accel_y_stored,&m_accel_z_stored);
    #endif
    //m_magsensor_stored = gpio_read_bit(PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit);


    // set new alarm for log_interval_seconds from now.
    rtc_clear_alarmed();
  }

  if(m_alarm_log == true) {
    if(system_geiger->is_cpm30_valid()) {

      log_data_t data;
      #ifndef DISABLE_ACCEL
      accel_read_state(&data.accel_x_end,&data.accel_y_end,&data.accel_z_end);
      #endif
      //data.magsensor_end = gpio_read_bit(PIN_MAP[29].gpio_device,PIN_MAP[29].gpio_bit);

      data.time  = rtc_get_time(RTC);
      data.cpm   = system_geiger->get_cpm30();

      data.accel_x_start = m_accel_x_stored;
      data.accel_y_start = m_accel_y_stored;
      data.accel_z_start = m_accel_z_stored;
      //data.magsensor_start = m_magsensor_stored;
      data.log_type      = UINT_MAX;

      flashstorage_log_pushback((uint8_t *) &data,sizeof(log_data_t));

      bool full = flashstorage_log_isfull();
      if((full == true) && (!m_sleeping)) {
        m_gui->show_dialog("Flash Log","is full",0,0,0,43,44,255,255);
      }

      m_alarm_log = false;

      rtc_set_alarm(RTC,m_last_alarm_time+m_log_interval_seconds);
      rtc_enable_alarm(RTC);
      if(m_sleeping) {
        power_standby();
      }
    }
  }
}

/**
 * Checks position of "Sleep" (standby) switch at the back of the device and
 * take action if switch position changed.
 */
void Controller::check_sleep_switch() {

  #ifndef NEVERSLEEP
  bool sstate = switch_state();
  if(sstate != m_last_switch_state) {
    m_last_switch_state = sstate;

    if(sstate == false) {
      if(m_alarm_log && (!m_sleeping)) { m_sleeping=true; display_powerdown(); } else {
        m_sleeping=true;
        power_standby();
      }
    }

    if(sstate == true) {
      m_powerup = true;
    }
  }
  #endif

  if(m_powerup == true) {
    buzzer_nonblocking_buzz(0.5);
    display_powerup();
    m_gui->set_sleeping(false);
    m_gui->redraw();
    m_sleeping=false;
    m_powerup =false;

    const char *devicetag = flashstorage_keyval_get("DEVICETAG");
    char revtext[10];
    sprintf(revtext,"VERSION: %s ",OS100VERSION);
    display_splashscreen(devicetag,revtext);
    delay_us(3000000);
    display_clear(0);
  }

  if(m_sleeping) {
    // go back to sleep.
    if((!rtc_alarmed()) && (!m_alarm_log)) {
      power_standby();
    }
    return;
  }
}

/**
 * Dim the screen if necessary. Called by the Controller. Will dim in
 * several steps (one dim level at each call)
 */
void Controller::do_dimming() {

  if(m_never_dim) return;

  // only dim if not in brightness changing mode
  if(!m_dim_off) {
    // Check for no key presses then dim screen
    uint32_t release_time = cap_last_press_any();
    uint32_t   press_time = cap_last_release_any();
    uint32_t current_time = realtime_get_unixtime();

    uint8_t current_brightness = display_get_brightness();
    if(((current_time - press_time) > 10) && ((current_time - release_time) > 10)) {
      if(current_brightness > 1) display_set_brightness(current_brightness-1);
    } else {
      const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
      unsigned int user_brightness=15;
      if(sbright != 0) {
        sscanf(sbright, "%u", &user_brightness);
      }
      if(current_brightness < user_brightness) {
        display_set_brightness(current_brightness+1);
      }
    }
  }
}

/**
 * Send current CPM value to the GUI for display.
 */
void Controller::send_cpm_values() {
  //TODO: I should change this so it only sends the messages the GUI currently needs.
  char text_cpmdint[50];
  char text_cpmd[50];

  float cpm_deadtime_compensated = system_geiger->get_cpm_deadtime_compensated();
  int_to_char(cpm_deadtime_compensated+0.5,text_cpmdint,7);
  if(cpm_deadtime_compensated > MAX_CPM) {
    sprintf(text_cpmdint,"TOO HIGH"); // kanji image is 45
  }

  if(!m_cpm_cps_switch) {       // no auto switch, just display CPM
    char text_cpmd_tmp[30];
    sprintf(text_cpmd_tmp,"%8.3f",system_geiger->get_cpm_deadtime_compensated());
    sprintf(text_cpmd    ,"%8.8s",text_cpmd_tmp);
    m_gui->receive_update("CPMSLABEL","CPM");
  } else {

    float cpm = system_geiger->get_cpm_deadtime_compensated();

    if(cpm > m_cpm_cps_threshold) {
      m_current_units = UNITS_CPS;
    }
    else if(cpm < m_cps_cpm_threshold) {
     m_current_units = UNITS_CPM;
    }
    // else { we are in the gray zone, do not switch }

    if(m_current_units == UNITS_CPM) {
      char text_cpmd_tmp[30];
      sprintf(text_cpmd_tmp,"%8.0f",system_geiger->get_cpm_deadtime_compensated());
      sprintf(text_cpmd    ,"%8.8s",text_cpmd_tmp);
      m_gui->receive_update("CPMSLABEL","CPM");
    } else {
      char text_cpmd_tmp[30];
      sprintf(text_cpmd_tmp,"%8.0f",system_geiger->get_cpm_deadtime_compensated()/60);
      sprintf(text_cpmd    ,"%8.8s",text_cpmd_tmp);
      m_gui->receive_update("CPMSLABEL","CPS");
    }
  }

  if(system_geiger->get_cpm_deadtime_compensated() > MAX_CPM) {
    sprintf(text_cpmd,"TOO HIGH");
  }
  m_gui->receive_update("CPMDEADINT",text_cpmdint);
  m_gui->receive_update("CPMDEAD",text_cpmd);
}

void Controller::send_graph_data() {
  float *graph_data;
  graph_data = system_geiger->get_cpm_last_windows();
  m_gui->receive_update("RECENTDATA",graph_data);
}

/**
 * Total counts
 */
void Controller::send_total_timer() {
  char text_totaltimer_count[50];
  char text_totaltimer_time [50];
  uint32_t ctime = realtime_get_unixtime();
  uint32_t totaltimer_time = ctime - m_total_timer_start;

  char temp[50];
  sprintf(text_totaltimer_time ,"%"PRIu32"s",totaltimer_time);
  sprintf(temp,"  %6.3f  " ,((float)system_geiger->get_total_count()/((float)totaltimer_time))*60);
  int len = strlen(temp);
  int pad = (16-len)/2;
  for(int n=0;n<16;n++) {
    if((n > pad) && (n < (pad+len))) {text_totaltimer_count[n] = temp[n-pad];}
                                else {text_totaltimer_count[n] = ' ';}
    text_totaltimer_count[n+1] = 0;
  }

  m_gui->receive_update("DELAYA",NULL);
  m_gui->receive_update("DELAYB",NULL);
  m_gui->receive_update("TTCOUNT",text_totaltimer_count);
  m_gui->receive_update("TTTIME" ,text_totaltimer_time);
}

void Controller::send_svrem() {
  const char *svrem = flashstorage_keyval_get("SVREM");

  if((svrem != 0) && (strcmp(svrem,"REM") == 0)) {
    char text_rem[50];
    char text_rem_tmp[50];
    text_rem[0]=0;
    sprintf(text_rem_tmp,"%8.3f",system_geiger->get_microrems());
    sprintf(text_rem    ,"%8.8s",text_rem_tmp);
    if((system_geiger->get_cpm_deadtime_compensated() > MAX_CPM) || (system_geiger->get_microrems() > 99999999)) {
      sprintf(text_rem,"TOO HIGH");
    }


    m_gui->receive_update("SVREM", text_rem);
    m_gui->receive_update("SVREMLABEL","  \x80R/h");
  } else {
    char text_sieverts[50];
    char text_sieverts_tmp[50];
    text_sieverts[0]=0;
    sprintf(text_sieverts_tmp,"%8.3f",system_geiger->get_microsieverts());
    sprintf(text_sieverts,"%8.8s",text_sieverts_tmp);
    if((system_geiger->get_cpm_deadtime_compensated() > MAX_CPM) || (system_geiger->get_microsieverts() > 99999999)) {
      sprintf(text_sieverts,"TOO HIGH");
    }


    m_gui->receive_update("SVREM", text_sieverts);
    m_gui->receive_update("SVREMLABEL"," \x80Sv/h");
  }
}

void Controller::send_becq() {
  char text_becq_tmp[50];
  char text_becq[50];
  float becq = system_geiger->get_becquerel();
  if(becq >= 0) {
    //float_to_char(system_geiger->get_becquerel(),text_becq,7);
    sprintf(text_becq_tmp,"%8.3f",system_geiger->get_becquerel());
    sprintf(text_becq,"%8.8s",text_becq_tmp);

    if(system_geiger->get_becquerel() > 99999999) {
      sprintf(text_becq,"TOO HIGH");
    }

    m_gui->receive_update("BECQ",text_becq);
  } else {
    m_gui->receive_update("BECQINFO","Becquerel unset"); // kanji image is: 46
  }
}

/**
 * Send current log area status: time remaining at current log
 * interval, and percentage full.
 */
void Controller::send_logstatus() {
  uint32_t total = flashstorage_log_maxrecords();
  uint32_t current = flashstorage_log_currentrecords();

  uint32_t percent = current*100/total;
  if (percent > 100) percent = 100; // We should never have this, but in case
                                    // the flashstorage functions return wrong values,
                                    // we'll be safe.
  char text[20];
  char text2[20];
  sprintf(text,"%"PRIu32"%% full", percent);
  m_gui->receive_update("LOGPERCENT", text);

  // Now compute how much time we have left in the log area at
  // current log interval:
  if (flashstorage_logpaused() ) {
    sprintf(text, "Logging paused  ");
    sprintf(text2, "         ");
  } else if (m_log_interval_seconds) {
    uint32_t time_left_h = (total-current) * m_log_interval_seconds / 3600;
    if (time_left_h > 99999) time_left_h = 99999; // Just to keep string under 16 characters
    // Make the user's life easier: above 48 hours, display in days/hours
    if (time_left_h < 49) {
      sprintf(text, "%"PRIu32" hours", time_left_h);
    } else {
      uint32_t days  = time_left_h / 24;
      uint32_t hours = time_left_h % 24;
      sprintf(text,"%"PRIu32" days %"PRIu32" hrs", days, hours);
    }
    sprintf(text2, "remaining");
  } else {
    sprintf(text,"Logging disabled");
    sprintf(text2, "         ");
  }
  m_gui->receive_update("LOGREMAIN", text);
  m_gui->receive_update("LOGREMAIN2", text2);

  // Last, display at the bottom of the screen the log status in
  // terms of current/total
  sprintf(text,"%"PRIu32"/%"PRIu32" recs",current,total);
  m_gui->receive_update("LOGREMAIN3", text);

}

/**
 * The main update method of the controller, called from
 * main.cpp. This sends all the current variables to the GUI, so that
 * they can be updated on the screen if necessary.
 */
void Controller::update() {
  check_warning_level();
  m_keytrigger=false;

  do_logging();

  check_sleep_switch();

  if(m_sleeping) return;

  do_dimming();

  send_cpm_values();
  send_graph_data();
  send_total_timer();
  send_svrem();
  send_becq();
  send_logstatus();
}
