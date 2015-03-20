#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void float_to_char(float    number,char *text,int32_t width);
uint32_t str_to_uint(const char *text);
bool strcmpl(const char *i,const char *j,uint32_t len);

#endif
