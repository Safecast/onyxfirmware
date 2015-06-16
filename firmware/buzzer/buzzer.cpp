#include "delay.h"
#include <stdint.h>
#include "string.h"

#include "wirish_boards.h"
#include "safecast_config.h"
#include "buzzer.h"
#include "flashstorage.h"

#define BUZZ_RATE    250  // in microseconds; set to 4kHz = 250us
#define MAX_RELOAD ((1 << 16) - 1)

uint32_t buzz_count;
uint32_t buzz_time;

bool headphones_out = false;
bool piezo_out = true;

void buzzer_handler(void) {
	if (piezo_out)
		gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
	if (headphones_out)
		gpio_toggle_bit(PIN_MAP[HP_COMBINED].gpio_device,PIN_MAP[HP_COMBINED].gpio_bit);

	buzz_count++;
	if(buzz_count == buzz_time) {
		timer_pause(TIMER2);
		piezo_out = true;
	}
}

/**
 * Non-blocking piezo/headphone beep
 */
void buzzer_nonblocking_buzz(float time, bool piezo, bool headphones) {

	// No need to go further if both outputs are
	// false
  if (!(piezo || headphones))
	  return;
  piezo_out = piezo;
  headphones_out = headphones;
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

/**
 * Buzz for 'time' microseconds. Blocking.
 *  Generates a square wave on the buzzer
 */
void buzzer_blocking_buzz(uint32_t time) {
  // buzz
  uint32_t frequency = 440; // 440 Hz
  uint32_t t = time*frequency*2/1e6; // Number of pulses to generate
  for(uint32_t n=0;n<t;n++) {
    gpio_toggle_bit(PIN_MAP[BUZZER_PWM].gpio_device, PIN_MAP[BUZZER_PWM].gpio_bit);
    delay_us(1e6/(frequency*2));
  }
  gpio_write_bit(PIN_MAP[BUZZER_PWM].gpio_device,PIN_MAP[BUZZER_PWM].gpio_bit,0);
}

/**
 * Output a string in morse code only if we are in debug mode
 */
void buzzer_morse_debug(char const* c) {
	  const char *dbg= flashstorage_keyval_get("DEBUG");
	  if (dbg != NULL && dbg[0] == '1') {
		  buzzer_morse(c);
	  }
}

/**
 * Output a string in morse code
 */
void buzzer_morse(char const* c) {
	const char
	  *ascii = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@ ",
	  *code[] = {
	     ".-","-...","-.-.","-..",".",
	     "..-.","--.","....","..",".---","-.-",
	     ".-..","--","-.","---",".--.","--.-",
	     ".-.","...","-","..-","...-",".--","-..-",
	     "-.--","--..","-----",".----","..---",
	     "...--","....-",".....","-....",
	     "--...","---..","----.",".-.-.-",
	     "--..--","..--..",".----.","-.-.--",
	     "-..-.","-.--.","-.--.-",".-...","---...",
	     "-.-.-.","-...-",".-.-.","-....-","..--.-",
	     ".-..-.","...-..-",".--.-.", "*"
	  };
	  uint32_t unit = 80000; // 80ms

	// Loop over the string
	while (*c) {
		// Get the index of the character
		char* idx  = strchr(ascii,*c++);
		if (idx == 0)
			return;
		// Get the corresponding code:
		const char*  morse = code[idx-ascii];
		while (*morse) {
			if (*morse == '*') {
				// Word separation (7 units)
				delay_us(3*unit);
			} else if (*morse == '.') {
				buzzer_blocking_buzz(unit);
			} else {
				buzzer_blocking_buzz(unit*3);
			}
			*morse++;
			delay_us(unit);
		}
		delay_us(unit*3);
	}
}
