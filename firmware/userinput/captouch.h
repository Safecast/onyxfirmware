#ifndef __CAPTOUCH_H__
#define __CAPTOUCH_H__

void cap_debug(void);
void cap_init(void);
void cap_deinit(void);
int cap_setkeydown(void (*new_keydown)(int key));
int cap_setkeyup(void (*new_keyup)(int key));
int cap_lastkey();
void cap_clearlastkey();
char *diag_data();
#endif /* __CAPTOUCH_H__ */
