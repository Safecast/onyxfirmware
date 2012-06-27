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

};
