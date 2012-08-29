/**************************************************
*                                                 *
*            Safecast Geiger Counter              *
*                                                 *
**************************************************/

#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"

#include "Buzzer.h"
#include "Accelerometer.h"
#include "UserInput.h"
#include "Geiger.h"
#include "Led.h"
#include "GUI.h"
#include "Controller.h"
#include <stdint.h>

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain()
{
  gpio_init_all();
  // manual power up beep
  gpio_set_mode (GPIOB,9, GPIO_OUTPUT_PP);
  for(int n=0;n<100;n++) {
    gpio_toggle_bit(GPIOB,9);
    delay_us(1000);
  }
  gpio_write_bit(GPIOB,9,0);

/*
  for(int i=0;i<11;i++) {
    for(int n=0;n<100;n++) {
      gpio_toggle_bit(GPIOB,9);
      delay_us(1000);
    }
    delay_us(100000);
  }
*/
//  power_standby();
  init();
  delay_us(100000);
}

int main(void) {

    Buzzer        b;
    Accelerometer a;
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

    Controller c(g);
    GUI m_gui(c);
    c.set_gui(m_gui);
    UserInput  u(m_gui);
    u.initialise();
    realtime_init();
    realtime_set_unixtime(0);
    for(;;) {
      c.update();
      m_gui.render();
      power_wfi();
    }

    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      b.buzz();
    }
    return 0;
}
