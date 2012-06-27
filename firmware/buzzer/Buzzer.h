#include "delay.h"

#define BUZZER_PWM   24 // PB9
#define BUZZ_RATE    250  // in microseconds; set to 4kHz = 250us

void handler_buzz(void) {
    gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
}

class Buzzer {

public:

  Buzzer() {
  }

  void initialise() {
    // initially, un-bias the buzzer
    gpio_set_mode (PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);

    // pause timer during setup
    timer_pause(TIMER4);

    set_frequency(BUZZ_RATE);

    // setup interrupt on channel 4
    timer_set_mode(TIMER4,TIMER_CH4,TIMER_OUTPUT_COMPARE);
    timer_set_compare(TIMER4,TIMER_CH4,1);
    timer_attach_interrupt(TIMER4,TIMER_CH4,handler_buzz);
    
    // refresh timer count, prescale, overflow
    timer_generate_update(TIMER4);
  }

  void set_frequency(uint16_t freq) {
    //setup period

    //TODO: fix this so it uses both prescaler and reload...
    timer_set_prescaler(TIMER4,freq*CYCLES_PER_MICROSECOND);
    timer_set_reload(TIMER4,1);
  }

  void buzz(uint16_t time=5000) {
    timer_resume(TIMER4);
    delay_us(time);
    timer_pause(TIMER4);
  }

  void powerup() {}
  void powerdown() {}

};
