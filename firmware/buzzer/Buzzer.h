#ifndef BUZZER_H
#define BUZZER_H

class Buzzer {

public:

  Buzzer();

  void initialise();

  void set_frequency(uint16_t freq);
  void buzz(uint16_t time=50000);
  void powerup(); 
  void powerdown();

  uint16_t m_frequency;
};

#endif
