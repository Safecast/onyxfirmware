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
#include "realtime.h"
#include "serialinterface.h"
#include "switch.h"

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

    Controller c(g);
    switch_init();


    // if we woke up on an alarm, we're going to be sending the system back.
    if(power_get_wakeup_source() == WAKEUP_RTC) {
      c.m_sleeping = true;

      // manual beep test
      /*
      for(int n=0;n<5;n++) {
	gpio_set_mode (GPIOB,9, GPIO_OUTPUT_PP);
	for(int n=0;n<100;n++) {
	  gpio_toggle_bit(GPIOB,9);
	  delay_us(1000);
	}
	gpio_write_bit(GPIOB,9,0);
	delay_us(1000000);
      }
      */
    }

    //TODO: if(rtc_alarmed() && (switch_mode() == SWITCH_SLEEP)) c.m_sleeping = true;

    GUI m_gui(c);
    c.set_gui(m_gui);
    UserInput  u(m_gui);
    u.initialise();
    flashstorage_initialise();
    serial_initialise();

    for(;;) {
      c.update();
      m_gui.render();
      power_wfi();
     // display_draw_tinytext(0,100,"!\"#HELLO WORLD",65535);
     // int wakeup = gpio_read_bit(PIN_MAP[18].gpio_device,PIN_MAP[18].gpio_bit);
     // display_draw_number(0,80,wakeup,5,0);
    }

    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      b.buzz();
    }
    return 0;
}
