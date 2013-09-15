#include "delay.h"
#include <stdint.h>

#include "wirish_boards.h"
#include "safecast_config.h"
#include "buzzer.h"

#define BUZZER_PWM   24 // PB9
#define BUZZ_RATE    250  // in microseconds; set to 4kHz = 250us
#define MAX_RELOAD ((1 << 16) - 1)

uint32_t buzz_count;
uint32_t buzz_time;

void buzzer_handler(void) {
  gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);

  buzz_count++;
  if(buzz_count == buzz_time) {
    timer_pause(TIMER2);
  }
}

void buzzer_nonblocking_buzz(float time) {

  buzz_time = 4100*time*2;

  // Configure timer2 to fire every N microseconds
  timer_pause(TIMER2);
  timer_set_prescaler(TIMER2,1);
  timer_set_reload(TIMER2,(125*CYCLES_PER_MICROSECOND)/2);

  // setup interrupt on channel 2
  timer_set_mode(TIMER2,TIMER_CH2,TIMER_OUTPUT_COMPARE);
  timer_set_compare(TIMER2,TIMER_CH2,MAX_RELOAD-1);
  timer_attach_interrupt(TIMER2,TIMER_CH2,buzzer_handler);

  // start timer2
  buzz_count=0;
  timer_generate_update(TIMER2); // refresh timer count, prescale, overflow
  timer_resume(TIMER2);
}

void buzzer_initialise() {
  // initially, un-bias the buzzer
  gpio_set_mode (PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit, GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);
}

void buzzer_blocking_buzz(float time) {
  // buzz
  uint32_t frequency = 4100;
  uint32_t t = 4100*time;//((time*1000000)/frequency)*2;
  for(uint32_t n=0;n<t;n++) {
    gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
    delay_us(frequency/2);
  }
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);
}
