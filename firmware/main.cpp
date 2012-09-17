/**************************************************
*                                                 *
*            Safecast Geiger Counter              *
*                                                 *
**************************************************/

#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"

//#define DISABLE_ACCEL
#include "Buzzer.h"
#include "UserInput.h"
#include "Geiger.h"
#include "Led.h"
#include "GUI.h"
#include "Controller.h"
#include <stdint.h>
#include "flashstorage.h"
#include "rtc.h"
#include "accel.h"


// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain() {
  gpio_init_all();
  // manual power up beep
  gpio_set_mode (GPIOB,9, GPIO_OUTPUT_PP);
  for(int n=0;n<100;n++) {
    gpio_toggle_bit(GPIOB,9);
    delay_us(1000);
  }
  gpio_write_bit(GPIOB,9,0);

  init();
  delay_us(100000);
}

int main(void) {

    Buzzer        b;
    Geiger        g;
    Led           l;

    power_set_debug(0);
    power_init();
    
    power_set_debug(1);

    power_set_state(PWRSTATE_USER);
    display_initialise();
    b.initialise();
    //a.initialise();
    l.initialise();
    g.initialise();

    //d.test();
    delay_us(1000000);

    l.set_on();

    realtime_init();
    #ifndef DISABLE_ACCEL
    accel_init();
    #endif
    power_standby();

    Controller c(g);
    GUI m_gui(c);
    c.set_gui(m_gui);
    UserInput  u(m_gui);
    u.initialise();
    //bool res1 = rtc_set_alarm(RTC,30);
    //bool res2 = rtc_enable_alarm(RTC);
    flashstorage_initialise();
    serial_initialise();
    //flashstorage_keyval_set("nicethings","i like cakes ");
    //flashstorage_keyval_set("nicerthings","working code");
    //const char* flashstr2 = flashstorage_keyval_get("nicerthings");
    //display_draw_text(0,80,flashstr2,0);
    //uint8_t *log = flashstorage_log_get();
    //display_draw_text(0,100,(char *)log,0);
    for(;;) {
      c.update();
      m_gui.render();
     // if(res1          == 0) display_draw_text(0,70,"alarmfail1",0);
      //if(res2          == 0) display_draw_text(0,70,"alarmfail2",0);
      //if(rtc_alarmed() == 1) display_draw_text(0,70,"alarmflag",0);
      //display_draw_number(0,70,rtc_get_time(RTC),6,0);
      power_wfi();
    }

    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      b.buzz();
    }
    return 0;
}
