#include "flashstorage.h"
#include "utils.h"
#include "safecast_wirish_types.h"

extern uint8_t _binary_flash_data_start;
extern uint8_t _binary_flash_data_size;

void flashstorage_initialise() {
}

char *flashstorage_getstring(char *key) {
  return (char *) &_binary_flash_data_start;
}

void flashstorage_clear() {
}
