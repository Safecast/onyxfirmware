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
    gpio_set_mode(PIN_MAP[UART_CTS_GPIO].gpio_device,PIN_MAP[UART_CTS_GPIO].gpio_bit,GPIO_INPUT_PD);
    gpio_set_mode(PIN_MAP[UART_RTS_GPIO].gpio_device,PIN_MAP[UART_RTS_GPIO].gpio_bit,GPIO_INPUT_PD);
    gpio_set_mode(PIN_MAP[UART_TXD_GPIO].gpio_device,PIN_MAP[UART_TXD_GPIO].gpio_bit,GPIO_INPUT_PD);
    gpio_set_mode(PIN_MAP[UART_RXD_GPIO].gpio_device,PIN_MAP[UART_RXD_GPIO].gpio_bit,GPIO_INPUT_PD);

    gpio_set_mode(PIN_MAP[GEIGER_PULSE_GPIO].gpio_device,PIN_MAP[GEIGER_PULSE_GPIO].gpio_bit,GPIO_INPUT_PD);

    gpio_set_mode (PIN_MAP[LED_GPIO].gpio_device,PIN_MAP[LED_GPIO].gpio_bit,GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[LED_GPIO].gpio_device,PIN_MAP[LED_GPIO].gpio_bit,1);
}

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

    power_set_debug(0);
    power_init();
    setup_gpio();
    
    power_set_debug(1);

    power_set_state(PWRSTATE_USER);
    d.initialise();
    b.initialise();
    u.initialise();
    a.initialise();

    d.test();
    for(int n=0;n<10;n++) {
      delay_us(1000000);
      b.buzz();
    }

    d.powerdown();
    b.powerdown();
    u.powerdown();
    a.powerdown();

    for(;;);
    return 0;
}
