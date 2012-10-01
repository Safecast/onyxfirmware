#ifndef __CAPTOUCH_H__
#define __CAPTOUCH_H__

#define KEY_PRESSED  0
#define KEY_RELEASED 1
#include <stdint.h>

class GUI;
extern GUI *system_gui;

void cap_debug(void);
void cap_init(void);
void cap_deinit(void);
int cap_lastkey();
void cap_clearlastkey();
char *diag_data(int e);
bool cap_check();

uint32_t cap_last_press();
uint32_t cap_last_release();
void cap_clear_press();

#endif /* __CAPTOUCH_H__ */
