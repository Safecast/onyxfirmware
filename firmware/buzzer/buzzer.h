#ifndef BUZZER_H
#define BUZZER_H

void buzzer_initialise();

void buzzer_blocking_buzz   (float time);
void buzzer_nonblocking_buzz(float time);

#endif
