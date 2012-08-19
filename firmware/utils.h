#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void float_to_char(float number,char *text,uint32_t width);
void int_to_char(uint32_t number,char *text,uint32_t width);
int32_t strlen(const char *i);
bool strcmp(const char *i,const char *j);

#endif
