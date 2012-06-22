/**************************************************
*                                                 *
*            Safecast Geiger Counter              *
*                                                 *
**************************************************/

#include "Display.h"
#include "Buzzer.h"
#include "power.h"
#include "captouch.h"

#define LED_GPIO 25       // PD2

#define UART_CTS_GPIO     46 // PA12
#define UART_RTS_GPIO     47 // PA11
#define UART_TXD_GPIO     8 // PA10
#define UART_RXD_GPIO     7 // PA9

#define MEASURE_FET_GPIO  45 // PC12
#define GEIGER_PULSE_GPIO 42 // PB3
#define GEIGER_ON_GPIO    4  // PB5


void setup_gpio() {
    // setup the inputs
    pinMode(UART_CTS_GPIO, INPUT);
    pinMode(UART_RTS_GPIO, INPUT);
    pinMode(UART_TXD_GPIO, INPUT);
    pinMode(UART_RXD_GPIO, INPUT);

    pinMode(GEIGER_PULSE_GPIO, INPUT);

    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, 1);
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void
premain()
{
    init();
    delay(100);
}


int main(void) {

    Display d;
    Buzzer b;

    power_set_debug(0);
    cap_init();
    power_init();
    setup_gpio();
    
    b.initialise();
    power_set_debug(1);
    b.buzz();
    b.buzz();
    b.buzz();
    b.buzz();

    power_set_state(PWRSTATE_USER);
    d.initialise();
    d.test();


    return 0;
}
