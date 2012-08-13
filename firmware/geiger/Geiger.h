#ifndef GEIGER_H
#define GEIGER_H

#include <stdint.h>

class Geiger {

public:

  Geiger();
  void initialise();
  uint32_t get_count();
  float get_cpm();
  float get_seiverts();

  float *get_cpm_last_30mins();
  void powerup  ();
  void powerdown();
  float cpm_last_30min[30];
};

#endif
