/**
 * Manage the Sleep/Wakeup switch at the back of the device
 */
#include "switch.h"
#include "exti.h"
#include "gpio.h"
#include "safecast_config.h"
#include "power.h"
#include "display.h"

/**
 * Get state of sleep switch
 * @return 1 if in Power up position
 * @return 0 if in Sleep position
 */
bool switch_state() {
	int wakeup = gpio_read_bit(PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,
			PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit);
	if (wakeup != 0)
		return SWITCH_SLEEP;
	else
		return SWITCH_POWERUP;
}

/**
 * Setup the IO line as input for the Sleep/Standby switch at the back of the device. Has to be
 * called once by the firmware, and before any further call to switch_state()
 */
void switch_initialise(void) {
	gpio_set_mode(PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_device,
			PIN_MAP[MANUAL_WAKEUP_GPIO].gpio_bit, GPIO_INPUT_PD);
}
