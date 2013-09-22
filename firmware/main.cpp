/**************************************************
*                                                 *
*            Safecast Geiger Counter              *
*                                                 *
**************************************************/

#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"

#include "UserInput.h"
#include "Geiger.h"
#include "GUI.h"
#include "Controller.h"
#include <stdint.h>
#include "flashstorage.h"
#include "rtc.h"
#include "accel.h"
#include "realtime.h"
#include "serialinterface.h"
#include "switch.h"
#include "buzzer.h"
#include <stdio.h>
#include <string.h>
#include "rtc.h"

extern uint8_t _binary___binary_data_private_key_data_start;
extern uint8_t _binary___binary_data_private_key_data_size;

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain() {
  gpio_init_all();
  init();
  delay_us(100000);
}

/**
 * This is where the application starts.
 */
int main(void) {

    Geiger g;
    power_initialise();

    if(power_battery_level() < 1) {
      power_standby();
    }

    serial_initialise();
    flashstorage_initialise();
    buzzer_initialise();
    realtime_initialise();
    g.initialise();

    uint8_t *private_key = ((uint8_t *) &_binary___binary_data_private_key_data_start);
    if(private_key[0] != 0) delay_us(1000);

    delay_us(10000);  // can be removed?

    #ifndef DISABLE_ACCEL
    accel_init();
    #endif

    switch_initialise();
    Controller c;

    // if we woke up on an alarm, we're going to be sending the system back.
    #ifndef NEVERSLEEP
    if(power_get_wakeup_source() == WAKEUP_RTC) {
      rtc_set_alarmed(); // if we woke up from the RTC, force the the alarm trigger.
      c.m_sleeping = true;
    } else {
      c.m_sleeping = false;
      buzzer_nonblocking_buzz(0.1);
      display_initialise();
      const char *devicetag = flashstorage_keyval_get("DEVICETAG");
      char revtext[10];
      sprintf(revtext,"VERSION: %s ",OS100VERSION);
      display_splashscreen(devicetag,revtext);
      delay_us(3000000);
      display_clear(0);
    }
    #endif
    #ifdef NEVERSLEEP
      display_initialise();
    #endif


    GUI m_gui(c);
    if(!c.m_sleeping) {
      bool full = flashstorage_log_isfull();
      if(full == true) {
        m_gui.show_dialog("Flash Log","is full",0,0,0);
      }
    }

    c.set_gui(m_gui);
    UserInput  u(m_gui);
    u.initialise();

    int utcoffsetmins_n = 0;
    const char *utcoffsetmins = flashstorage_keyval_get("UTCOFFSETMINS");
    if(utcoffsetmins != 0) {
      int c;
      sscanf(utcoffsetmins, "%d", &c);
      utcoffsetmins_n = c;

      realtime_setutcoffset_mins(utcoffsetmins_n);
    }


    // Need to refactor out stored settings
    flashstorage_keyval_update();

    const char *language = flashstorage_keyval_get("LANGUAGE");
    if(language != 0) {
      if(strcmp(language,"English" ) == 0) { m_gui.set_language(LANGUAGE_ENGLISH);  tick_item("English"  ,true); } else
      if(strcmp(language,"Japanese") == 0) { m_gui.set_language(LANGUAGE_JAPANESE); tick_item("Japanese" ,true); }
    } else {
      m_gui.set_language(LANGUAGE_ENGLISH);
      tick_item("English",true);
    }


    m_gui.jump_to_screen(1);
    m_gui.push_stack(0,1);

    /**
     * Start of main event loop here, we will not leave this
     * loop until battery runs our or user changes the standby switch
     * position.
     *
     * c is our controller
     * g is th Geiger object
     * m_gui is the GUI
     *
     */
    for(;;) {

      if(power_battery_level() < 1) {
        power_standby();
      }

      c.update();

      if(!c.m_sleeping) {
        m_gui.render();
        serial_eventloop();

        // This was causing the serial interface to fail, so have removed it.
        // It might be a good idea to move the following code to Controller.
        // Hack to check that captouch is ok, and reset it if not.
        //bool c = cap_check();
        //if(c == false) {
        //  display_draw_text(0,90,"CAPFAIL",0);
        //  cap_init();
        //}

        // Screen lock code
        uint32_t release1_time = cap_last_press(KEY_BACK);
        uint32_t   press1_time = cap_last_release(KEY_BACK);
        uint32_t release2_time = cap_last_press(KEY_SELECT);
        uint32_t   press2_time = cap_last_release(KEY_SELECT);
        uint32_t current_time  = realtime_get_unixtime();

        int cap1 = cap_ispressed(KEY_BACK);
        int cap2 = cap_ispressed(KEY_SELECT);
        if((release1_time != 0) &&
           (release2_time != 0) &&
           ((current_time-press1_time) > 3) &&
           ((current_time-press2_time) > 3) &&
           cap1 && cap2) {
           system_gui->toggle_screen_lock();
           cap_clear_press();
        }

        power_wfi();
      }
    }

    /**
     *   End of main event loop
     */

    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      buzzer_blocking_buzz(1000);
    }
    return 0;
}
