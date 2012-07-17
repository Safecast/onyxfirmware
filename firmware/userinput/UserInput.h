#include "captouch.h"

class UserInput {

public:

  UserInput() {
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
};
