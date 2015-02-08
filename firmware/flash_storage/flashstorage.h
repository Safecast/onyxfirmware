#ifndef FLASHSTORAGE_H
#define FLASHSTORAGE_H

#include <stdint.h>
void flashstorage_initialise();


// Get-set settings
const char *flashstorage_keyval_get(const char *key);
void flashstorage_keyval_set(const char *key,const char *value);
void flashstorage_keyval_by_idx(int idx,char *key,char *val);
void flashstorage_keyval_update();

// Manipulation of log space
void     flashstorage_log_clear();
int      flashstorage_log_pushback(uint8_t *data,uint32_t size);
uint32_t flashstorage_log_size();
uint8_t *flashstorage_log_baseaddress();
bool     flashstorage_log_isfull();
void     flashstorage_log_pause();
void     flashstorage_log_resume();
bool     flashstorage_logpaused();
void     flashstorage_log_userchange();
void	 flashstorage_dump_hex();

uint32_t flashstorage_log_maxrecords();
uint32_t flashstorage_log_currentrecords();

bool flashstorage_islocked();
void flashstorage_clear();

// low level access functions
bool flashstorage_unlock();
bool flashstorage_lock();
bool flashstorage_erasepage(uint8_t *pageaddr);
bool flashstorage_writepage(uint8_t *new_data,uint8_t *pageaddr);


#endif
