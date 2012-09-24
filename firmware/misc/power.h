#ifndef __POWER_H__
#define __POWER_H__

#define PWRSTATE_DOWN  0   // everything off, no logging; entered when battery is low
#define PWRSTATE_LOG   1   // system is on, listening to geiger and recording; but no UI
#define PWRSTATE_USER  2   // system is on, UI is fully active
#define PWRSTATE_BOOT  3   // during boot
#define PWRSTATE_OFF   4   // power is simply off, or cold reset
#define PWRSTATE_ERROR 5   // an error conditions state
#include <stdint.h>

int power_initialise(void);
int power_deinit(void);
void power_wfi(void);
int power_switch_state(void);

int power_is_battery_low(void);
uint16_t power_battery_level(void);
int power_set_state(int state);
int power_get_state(void);
void power_standby(void);
#define WAKEUP_NONE 0
#define WAKEUP_RTC  1
#define WAKEUP_WKUP 2

int power_get_wakeup_source();


#endif /* __POWER_H__ */
