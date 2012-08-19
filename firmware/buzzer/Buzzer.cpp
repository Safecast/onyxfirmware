#include "delay.h"
#include <stdint.h>

#include "wirish_boards.h"
#include "power.h"
#include "safecast_config.h"
#include "Buzzer.h"

#define BUZZER_PWM   24 // PB9
#define BUZZ_RATE    250  // in microseconds; set to 4kHz = 250us
#define MAX_RELOAD ((1 << 16) - 1)

void handler_buzz(void) {
    gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
}

Buzzer::Buzzer() {
}

void Buzzer::initialise() {
  // initially, un-bias the buzzer
  gpio_set_mode (PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit, GPIO_OUTPUT_PP);
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);

  // pause timer during setup

  set_frequency(BUZZ_RATE);
}

void Buzzer::set_frequency(uint16_t freq) {
  m_frequency = freq;
}

void Buzzer::buzz(uint16_t time) {
/*
  timer_pause(TIMER4);
  //TODO: fix this so it uses both prescaler and reload...
  timer_set_prescaler(TIMER4,(m_frequency*CYCLES_PER_MICROSECOND)/MAX_RELOAD);
  timer_set_reload(TIMER4,MAX_RELOAD);

  // setup interrupt on channel 4
  timer_set_mode(TIMER4,TIMER_CH4,TIMER_OUTPUT_COMPARE);
  timer_set_compare(TIMER4,TIMER_CH4,1);
  timer_attach_interrupt(TIMER4,TIMER_CH4,handler_buzz);

  timer_generate_update(TIMER4); // refresh timer count, prescale, overflow

  // buzz
  timer_resume(TIMER4);
  delay_us(time);
  timer_disable(TIMER4);
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);
*/
}

void Buzzer::powerup() {}
void Buzzer::powerdown() {}
