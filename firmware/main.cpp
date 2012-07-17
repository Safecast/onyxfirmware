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
#include "Display.h"
#include "UserInput.h"
#include "Geiger.h"
#include "Led.h"
#include "GUI.h"

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

    Display       d;
    Buzzer        b;
    UserInput     u;
    Accelerometer a;
    Geiger        g;
    Led           l;
    RealTime      r;

    power_set_debug(0);
    power_init();
    
    power_set_debug(1);

    power_set_state(PWRSTATE_USER);
    d.initialise();
    b.initialise();
    u.initialise();
    //a.initialise();
    l.initialise();
    r.initialise();
    g.initialise();


    l.set_on();
    //delay_us(10000000);


    //d.powerdown();
    //b.powerdown();
    //u.powerdown();
    //a.powerdown();

    //int array[100];
    //for(size_t n=0;n<100;n++) array[n] =5;
    uint16_t h[6];
    for(size_t n=0;n<6;n++) h[n]=0;
    h[0]=65535;
    int highlight=0;

    GUI m_gui(d);
    for(;;) {
      char count[100];
      l.set_off();
     // sprintf(count,"count: %u",g.get_count());
     // d.draw_text(0,96,count,0);
      //d.draw_text(0,96,diag_data(),0); 
      //sprintf(count,"capkey: %d",u.last_key());
      //d.draw_text(0,112,count,0);
     
      
      if(u.last_key() == 3) m_gui.receive_up();
      if(u.last_key() == 4) m_gui.receive_down();
      if(u.last_key() == 2) m_gui.receive_select();

      u.clear_last_key();
      m_gui.set_cpm(g.get_cpm());
      m_gui.render();
      m_gui.set_graph(g.get_cpm_last_30mins(),30);

      power_standby();
      l.set_on();
    }

    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      b.buzz();
    }
    return 0;
}
