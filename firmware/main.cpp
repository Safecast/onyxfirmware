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

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain()
{
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

    power_set_debug(0);
    power_init();
    
    power_set_debug(1);

    power_set_state(PWRSTATE_USER);
    d.initialise();
    b.initialise();
    //u.initialise();
    a.initialise();
    l.initialise();
    g.initialise();

    d.test();
    for(int n=0;n<10;n++) {
 //     delay_us(1000000);
      l.set_on();
      b.buzz();
      l.set_off();
    }

    char *aa="Hello again";
    for(int n=0;n<128;n++) {
      delay_us(1000000);
      b.buzz();
      d.draw_text(n,16,aa,0);
      aa[0] = aa[0]+1;
    }

    d.powerdown();
    //b.powerdown();
    //u.powerdown();
    //a.powerdown();
    delay_us(1000000);
    l.set_off();

    for(;;) {
      power_standby();
      b.buzz();
    }
    // should never get here
    for(int n=0;n<60;n++) {
      delay_us(100000);
      b.buzz();
    }
    return 0;
}
