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
  // buzz
  uint32_t t = (time*1000000)/m_frequency;
  for(uint32_t n=0;n<t;n++) {
    gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
    delay_us(m_frequency);
  }
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);
}

void Buzzer::powerup() {}
void Buzzer::powerdown() {}
