#include "HardwareTimer.h"
#include "delay.h"
#include "wirish_time.h"

#define BUZZER_PWM   24 // PB9
#define BUZZ_RATE    250  // in microseconds; set to 4kHz = 250us

void handler_buzz(void) {
    togglePin(BUZZER_PWM);
}

class Buzzer {

public:

  Buzzer() {
  }

  void initialise() {
    // initially, un-bias the buzzer
    pinMode(BUZZER_PWM, OUTPUT);
    digitalWrite(BUZZER_PWM, 0);

    buzzTimer = new HardwareTimer(4);
    // pause timer during setup
    buzzTimer->pause();
    //setup period
    buzzTimer->setPeriod(BUZZ_RATE);

    // setup interrupt on channel 4
    buzzTimer->setChannel4Mode(TIMER_OUTPUT_COMPARE);
    buzzTimer->setCompare(TIMER_CH4, 1); // interrupt one count after each update
    buzzTimer->attachCompare4Interrupt(handler_buzz);

    // refresh timer count, prescale, overflow
    buzzTimer->refresh();

    
    // start the timer counting
    //    buzzTimer.resume();
  }


  void buzz() {
    buzzTimer->resume();
    delay(50);
    buzzTimer->pause();
  }

  HardwareTimer *buzzTimer;
};
