#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include <stdint.h>
void flashstorage_initialise();


const char *flashstorage_keyval_get(const char *key);
void flashstorage_keyval_set(const char *key,const char *value);

void     flashstorage_log_clear();
void     flashstorage_log_pushback(uint8_t *data,uint32_t size);
uint32_t flashstorage_log_size();
uint8_t *flashstorage_log_get();
bool     flashstorage_log_isfull();

void flashstorage_log_userchange();

bool flashstorage_islocked();
void flashstorage_clear();

// low level access functions
bool flashstorage_unlock();
bool flashstorage_lock();
bool flashstorage_erasepage(uint8_t *pageaddr);
bool flashstorage_writepage(uint8_t *new_data,uint8_t *pageaddr);

#endif
