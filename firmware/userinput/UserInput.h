#ifndef USERINPUT
#define USERINPUT

#include "captouch.h"

class GUI;

class UserInput {

public:

  UserInput(GUI &r) : receiver(r) {
    system_gui = &r;
  }

  void initialise() {
    cap_init();
  }

  void powerup() {}

  void powerdown() {
    cap_deinit();
  }

  int last_key() {
    return cap_lastkey();
  }
  
  void clear_last_key() {
    cap_clearlastkey();
  }

  GUI &receiver;
};

#endif
