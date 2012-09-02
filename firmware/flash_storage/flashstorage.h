#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include <stdint.h>
void flashstorage_initialise();


const char *flashstorage_keyval_get(const char *key);
void flashstorage_keyval_set(const char *key,const char *value);

void     flashstorage_log_clear();
void     flashstorage_log_pushback(char *data,uint32_t size);
uint32_t flashstorage_log_size();
uint8_t *flashstorage_log_getbase();

bool flashstorage_islocked();
void flashstorage_clear();

#endif
