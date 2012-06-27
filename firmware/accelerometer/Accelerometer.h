#include "accel.h"

class Accelerometer {

public:

  Accelerometer() {
  }

  void initialise() {
    accel_init();
  }

  void powerup() {}
  void powerdown() {}

};
