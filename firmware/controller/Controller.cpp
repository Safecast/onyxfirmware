#include "Controller.h"
#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"
#include "realtime.h"
#include "flashstorage.h"
#include "rtc.h"
#include "serialinterface.h"
#include "power.h"
#include <stdio.h>
#include "accel.h"
#include "log.h"
#include "switch.h"
#include "qr_xfer.h"
#include "buzzer.h"
//#define DISABLE_ACCEL

Controller *system_controller;

Controller::Controller(Geiger &g) : m_geiger(g) {
  m_sleeping=false;
  m_powerup=false;
  m_log_period_seconds = 120;
  rtc_clear_alarmed();
  rtc_set_alarm(RTC,rtc_get_time(RTC)+m_log_period_seconds);
  rtc_enable_alarm(RTC);
  m_alarm_log = false;
  system_controller = this;
  m_last_switch_state = true;
  m_warning_raised = false;
  m_changing_brightness=false;

  // Get warning cpm from flash
  m_warncpm = -1;
  const char *swarncpm = flashstorage_keyval_get("WARNCPM");
  if(swarncpm != 0) {
    uint32_t c;
    sscanf(swarncpm, "%d", &c);
    m_warncpm = c;
  }

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
  text_sieverts[6] = 'u';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;
  m_gui->receive_update("FIXEDSV",text_sieverts);
}
 
void Controller::save_calibration() {
  int c1 = m_gui->get_item_state_uint8("CAL1");
  int c2 = m_gui->get_item_state_uint8("CAL2");
  int c3 = m_gui->get_item_state_uint8("CAL3");
  int c4 = m_gui->get_item_state_uint8("CAL4");
  float calibration_scaling = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);
  float base_sieverts = m_geiger.get_microsieverts();    

  char text_sieverts[50];
  float_to_char(base_sieverts*calibration_scaling,text_sieverts,5);
  text_sieverts[5] = ' ';
  text_sieverts[6] = 'u';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;

  m_gui->receive_update("Sieverts",text_sieverts);
  m_geiger.set_calibration(calibration_scaling);
  m_gui->jump_to_screen(0);
}

void Controller::initialise_calibration() {
  m_calibration_base = m_geiger.get_microsieverts();
  char text_sieverts[50];
  float_to_char(m_calibration_base,text_sieverts,5);
  text_sieverts[5] = ' ';
  text_sieverts[6] = 'u';
  text_sieverts[7] = 'S';
  text_sieverts[8] = 'v';
  text_sieverts[9] = 0;
  m_gui->receive_update("FIXEDSV",text_sieverts);
  uint8_t val=1;
  m_gui->receive_update("CAL1",&val);
}

void Controller::save_warncpm() {
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
  sprintf(swarncpm,"%d",warn_cpm);
  flashstorage_keyval_set("WARNCPM",swarncpm);

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

  flashstorage_log_userchange();
  m_gui->jump_to_screen(0);
}


bool is_leap(int month,int day,int year) {
  if((year % 400) == 0) return true;
 
  if((year % 100) == 0) return false;

  if((year % 4)   == 0) return true;

  return false;
}

void Controller::receive_gui_event(char *event,char *value) {

  //TODO: Fix this total mess, refactor into switch, break conditions out into methods.
  if(strcmp(event,"Sleep")) {
    if(m_sleeping == false) {
      display_powerdown();
      m_sleeping=true;
      m_gui->set_key_trigger();
      m_gui->set_sleeping(true);
      power_standby();
    }
  } else
  if(strcmp(event,"KEYPRESS")) {
    m_powerup=true;
  } else
  if(strcmp(event,"TOTALTIMER")) {
    m_geiger.reset_total_count();
    m_total_timer_start = realtime_get_unixtime();

    char *blank = "              ";
    m_gui->receive_update("TTCOUNT",blank);
    m_gui->receive_update("TTTIME" ,blank);
    m_gui->redraw();
  } else
  if(strcmp(event,"Save")) {
    save_calibration();
  } else
  if(strcmp(event,"SaveTime")) {
    save_time();
  } else
  if(strcmp(event,"SaveDate")) {
    save_date();
  } else 
  if(strcmp(event,"SaveWarnCPM")) {
    m_warning_raised = false; 
    save_warncpm();
  } else
  if(strcmp(event,"Japanese")) {
    m_gui->set_language(LANGUAGE_JAPANESE);
    flashstorage_keyval_set("LANGUAGE","Japanese");
  } else
  if(strcmp(event,"English")) {
    m_gui->set_language(LANGUAGE_ENGLISH);
    flashstorage_keyval_set("LANGUAGE","English");
  } else
  if(strcmp(event,"Geiger Beep")) {
     m_geiger.toggle_beep();
     if(m_geiger.is_beeping()) flashstorage_keyval_set("GEIGERBEEP","true");
                          else flashstorage_keyval_set("GEIGERBEEP","false");
  } else 
  if(strcmp(event,"Clear Log")) {
    buzzer_nonblocking_buzz(3);
    flashstorage_log_clear();
  } else 
  if(strcmp(event,"SaveBrightness")) {
    uint8 b = m_gui->get_item_state_uint8("BRIGHTNESS");
    char sbright[50];
    sprintf(sbright,"%u",b+6);
    flashstorage_keyval_set("BRIGHTNESS",sbright);
    m_gui->jump_to_screen(0);
    m_changing_brightness = false;
  } else
  if(strcmp(event,"CALIBRATE")) {
    initialise_calibration();
  } else
  if(strcmp(event,"DATESCREEN")) {
    uint8 m1,m2,d1,d2,y1,y2;
    d1 = m_gui->get_item_state_uint8("DATEDAY1");
    d2 = m_gui->get_item_state_uint8("DATEDAY2");
    m1 = m_gui->get_item_state_uint8("DATEMON1");
    m2 = m_gui->get_item_state_uint8("DATEMON2");
    y1 = m_gui->get_item_state_uint8("DATEYEAR1");
    y2 = m_gui->get_item_state_uint8("DATEYEAR2");

    if((d1 == 0) && (d2 == 0)) {
      m1=0;m2=1;d1=0;d2=1;
      m_gui->receive_update("DATEMON1",&m1);
      m_gui->receive_update("DATEMON2",&m2);
      m_gui->receive_update("DATEDAY1",&d1);
      m_gui->receive_update("DATEDAY2",&d2);
      m_gui->redraw();
    }
  } else 
  if(strcmp(event,"varnumchange")) {
    m_changing_brightness = true;
    if(strcmp("BRIGHTNESS",value)) {
      int b = m_gui->get_item_state_uint8("BRIGHTNESS");
      display_set_brightness(b+6);
    } else

    if(strcmpl("CAL",value,3)) {
      update_calibration();
    } else
    if(strcmpl("DATE",value,4)) {
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
    } else
    if(strcmpl("TIME",value,4)) {
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
  } else
  if(strcmp(event,"Serial Transfer")) {
    display_draw_text(0,48,"Sending Log",0);
    serial_sendlog();
    display_draw_text(0,48,"Xfer Complete",0);
  } else 
  if(strcmp(event,"QR Transfer")) {
    display_draw_text(0,100,"QR Xfer",0);
    qr_logxfer();
  }
}

void Controller::update() {

  if((m_warncpm > 0) && (m_geiger.get_cpm() >= m_warncpm) && (m_warning_raised == false) && m_geiger.is_cpm_valid()) {
    m_warning_raised = true;
    if(m_sleeping) display_powerup();

    display_clear(0);
    int keys = cap_lastkey();
    for(;;) {
      display_draw_text  (0,32 ," WARNING  LEVEL "   ,FOREGROUND_COLOR);
      display_draw_text  (0,48 ,"    EXCEEDED    "   ,FOREGROUND_COLOR);
      display_draw_number_center(0,64 ,m_geiger.get_cpm(),16,FOREGROUND_COLOR);
      display_draw_text  (4,80 ,"      CPM      "    ,FOREGROUND_COLOR);
      display_draw_text  (4,112," PRESS ANY KEY "    ,FOREGROUND_COLOR);

      int newkeys = cap_lastkey();
      if(keys != newkeys) break;
      buzzer_nonblocking_buzz(1);
    }
    display_clear(0);
    m_gui->redraw();
    
    if(m_sleeping) display_powerdown();
  }
  m_keytrigger=false;

  if(rtc_alarmed()) {
    m_alarm_log = true;
    m_last_alarm_time = rtc_get_time(RTC);
    #ifndef DISABLE_ACCEL
    int8 res = accel_read_state(&m_accel_x_stored,&m_accel_y_stored,&m_accel_z_stored);
    #endif

    // set new alarm for log_period_seconds from now.
    rtc_clear_alarmed();
  }

  if(m_alarm_log == true) {
    if(m_geiger.is_cpm_valid()) {

      log_data_t data;
      #ifndef DISABLE_ACCEL
      int8 res = accel_read_state(&data.accel_x_end,&data.accel_y_end,&data.accel_z_end);
      #endif

      data.time  = rtc_get_time(RTC);
      data.cpm   = m_geiger.get_cpm30();
      data.accel_x_start = m_accel_x_stored;
      data.accel_y_start = m_accel_y_stored;
      data.accel_z_start = m_accel_z_stored;
      data.log_type      = 255;

      flashstorage_log_pushback((uint8_t *) &data,sizeof(log_data_t));
      m_alarm_log = false;

      rtc_set_alarm(RTC,m_last_alarm_time+m_log_period_seconds);
      rtc_enable_alarm(RTC);
    }
  }

  bool sstate = switch_state();
  if(sstate != m_last_switch_state) {
    m_last_switch_state = sstate;

    if(sstate == false) {
      if(m_alarm_log) { m_sleeping=true; display_powerdown(); } else {
        power_standby();
      }
    }
    
    if(sstate == true) {
      // how did this happen?!?
      m_powerup = true;
    }
  }

  if(m_powerup == true) {
    display_powerup();
    m_gui->set_sleeping(false);
    m_gui->redraw();
    m_sleeping=false;
    m_powerup =false;
  }


  if(m_sleeping) {
    // go back to sleep.
    if((!rtc_alarmed()) && (!m_alarm_log)) {
      power_standby();
    }
    return;
  }

  // Check for no key presses then dim screen
  uint32_t release_time = cap_last_press();
  uint32_t   press_time = cap_last_release();
  uint32_t current_time = realtime_get_unixtime();

  if(!m_changing_brightness) {
		uint8_t current_brightness = display_get_brightness();
		if(((current_time - press_time) > 10) && (current_time - release_time > 10)) {
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
 

  //TODO: I should change this so it only sends the messages the GUI currently needs.
  char text_cpmdint[50];
  char text_cpmd[50];
  char text_sieverts[50];

  text_cpmdint[0] =0;
  text_sieverts[0]=0;
  int_to_char(m_geiger.get_cpm_deadtime_compensated()+0.5,text_cpmdint,7);
  float_to_char(m_geiger.get_microsieverts(),text_sieverts,5);
  float_to_char(m_geiger.get_cpm_deadtime_compensated(),text_cpmd,7);
  
//  text_cpm[0]      = 'C';
//  text_cpm[1]      = 'P';
//  text_cpm[2]      = 'M';
//  text_cpm[3]      = ' ';
//  text_cpm[4]      = ':';

  float *graph_data;
  graph_data = m_geiger.get_cpm_last_windows();

  uint8_t hours,min,sec,day,month;
  uint16_t year;
  realtime_getdate(hours,min,sec,day,month,year);

  char text_time[50];
  int_to_char(hours,text_time,3);
  text_time[3]=':';
  int_to_char(min  ,text_time+4,3);
  text_time[6]=':';
  int_to_char(sec  ,text_time+7,3);
  text_time[10] = 0;

  char text_date[50];
  int_to_char(day,text_date,3);
  text_date[3]='/';
  int_to_char(month+1,text_date+4,3);
  text_date[6]='/';
  int_to_char(year+1900,text_date+7,4);
  text_date[11] = 0;

  char text_totaltimer_count[50];
  char text_totaltimer_time [50];
  uint32_t ctime = realtime_get_unixtime();
  uint32_t totaltimer_time = ctime - m_total_timer_start;

  sprintf(text_totaltimer_time ,"%us",totaltimer_time);
  sprintf(text_totaltimer_count,"%6.3f" ,((float)m_geiger.get_total_count()/((float)totaltimer_time))*60);

  //if(m_geiger.is_cpm_valid()) m_gui->receive_update("CPMVALID","true");
  //                       else m_gui->receive_update("CPMVALID","false");
  m_gui->receive_update("CPMDEADINT",text_cpmdint);
  m_gui->receive_update("CPMDEAD",text_cpmd);
  m_gui->receive_update("SIEVERTS",text_sieverts);
  m_gui->receive_update("RECENTDATA",graph_data);
  m_gui->receive_update("DELAYA",NULL);
  m_gui->receive_update("DELAYB",NULL);
  m_gui->receive_update("TIME",text_time);
  m_gui->receive_update("DATE",text_date);
  m_gui->receive_update("TTCOUNT",text_totaltimer_count);
  m_gui->receive_update("TTTIME" ,text_totaltimer_time);
}
