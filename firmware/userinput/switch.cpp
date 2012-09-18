// Sample main.cpp file. Blinks the built-in LED, sends a message out
// USART1.

#include "switch.h"
#include "exti.h"
#include "gpio.h"
#include "safecast_config.h"
#include "power.h"
#include "display.h"

int switch_state() {
  int wakeup = gpio_read_bit(PIN_MAP[18].gpio_device,PIN_MAP[18].gpio_bit);
  if(wakeup != 0) return 0; else return 1;
}

void switch_init(void) {
  gpio_set_mode (PIN_MAP[18].gpio_device,PIN_MAP[18].gpio_bit, GPIO_INPUT_PD);
}
