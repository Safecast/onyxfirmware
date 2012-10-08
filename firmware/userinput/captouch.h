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
bool cap_ispressed(int key);

uint32_t cap_last_press_any();
uint32_t cap_last_release_any();

uint32_t cap_last_press(int key);
uint32_t cap_last_release(int key);
void cap_clear_press();
void cap_set_disable_messages(bool b);

#endif /* __CAPTOUCH_H__ */
