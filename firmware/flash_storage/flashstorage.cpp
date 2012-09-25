#include "flashstorage.h"
#include "flash.h"
#include "utils.h"
#include "safecast_wirish_types.h"
#include "display.h"

extern uint8_t _binary___binary_data_flash_data_start;
extern uint8_t _binary___binary_data_flash_data_size;

uint8_t  *flash_data_area_aligned;
uint32_t  flash_data_area_aligned_size;

#define flash_log_base  10240
#define keyval_size     50
#define keyval_size_all 100
#define pagesize        2048

bool flashstorage_writewait() {
  // wait to write
  for(uint32_t n=0;FLASH_BASE->SR & (1<<0);n++) {
    if(n >= 40000000) return false;
  } 

  return true;
}

bool flashstorage_unlock() {

  // unlock, using STM32 magic numbers
  FLASH_BASE->KEYR = 0x45670123;
  FLASH_BASE->KEYR = 0xCDEF89AB;

  // verify not locked
  if(!flashstorage_islocked()) return false;

  return true;
}

bool flashstorage_lock() {

  // lock
  FLASH_BASE->CR = 0x80;

  // verify locked
  if(flashstorage_islocked()) return true;

  return false;
}

bool flashstorage_islocked() {
  if (FLASH_BASE->CR & (1<<7)) return true;
  return false;
}

bool flashstorage_erasepage(uint8_t *pageaddr) {

  if(flashstorage_islocked()) return false;

  FLASH_BASE->CR |= 1<<1;
  FLASH_BASE->AR = (uint32_t) pageaddr;
 
  if(flashstorage_writewait() == false) return false;

  FLASH_BASE->CR |= 1<<1;
  FLASH_BASE->AR = (uint32_t) pageaddr;
  FLASH_BASE->CR |= 1<<6;

  // Wait for complete, or return false if timeout.
  if(flashstorage_writewait() == false) return false;

  // Verify erase
  for(uint32_t i=0;i<pagesize;i++) {
    if(flashstorage_writewait() == false) return false;
    if(pageaddr[i] != 0xFF) return false;
  }

  return true;
} 

bool flashstorage_writepage(uint8_t *new_data,uint8_t *pageaddr) {

  if(flashstorage_islocked()) return false;

  FLASH_BASE->CR  |= 1<<0;

  for (uint32_t n=0;n<(pagesize/2);n++) {

    if(flashstorage_writewait() == false) return false;

    // write MUST be 16bits at a time.
    ((uint16_t *) pageaddr)[n] = ((uint16_t *) new_data)[n];
  }

  return true;
}


void flashstorage_initialise() {
  uint32_t flash_addr = (uint32_t) &_binary___binary_data_flash_data_start;

  uint32_t unusable = 0;
  if((flash_addr%pagesize) != 0) {
    unusable = pagesize-(flash_addr%pagesize);
    flash_addr += unusable;
  }


  flash_data_area_aligned = (uint8_t *) flash_addr;
  flash_data_area_aligned_size = _binary___binary_data_flash_data_size - unusable;
}

char *flashstorage_keyval_get_address(const char *key) {

  for(uint32_t n=0;n<flash_log_base;n+=(keyval_size*2)) {

    if(flash_data_area_aligned[n] == 0) {
      return 0;
    }

    if(strcmp(key, (const char *) (flash_data_area_aligned+n)) == true) {
      uint8_t *addr = ((flash_data_area_aligned)+n);
      return (char *) addr;
    }
  }

  return 0;
}

char *flashstorage_keyval_get_unalloced() {
  for(uint32_t n=0;n<flash_log_base;n+=(keyval_size*2)) {

    if(flash_data_area_aligned[n] == 0) {
      return (char *) (flash_data_area_aligned)+n;
    }
  }
}

const char *flashstorage_keyval_get(const char *key) {

  char *v = flashstorage_keyval_get_address(key);

  if(v != 0) return v+keyval_size;

  return 0;
}


uint8_t *flashstorage_get_pageaddr(uint8_t *keyaddr) {

  uint32_t rem = ((uint32_t) keyaddr)%pagesize;
  return keyaddr-rem;
}

void flashstorage_readpage(uint8_t *page_address,uint8_t *pagedata) {
  for(uint32_t n=0;n<pagesize;n++) {
    pagedata[n] = page_address[n];
  }
}

void flashstorage_keyval_set(const char *key,const char *value) {

  uint8_t pagedata[pagesize];
  uint8_t new_keyval_data[keyval_size_all];

  strcpy((char *) new_keyval_data            ,(char *)key);
  strcpy((char *) new_keyval_data+keyval_size,(char *)value);

  // just in case!
  new_keyval_data[49] = 0;
  new_keyval_data[99] = 0;

  // read original page data
  char *kvaddr     = flashstorage_keyval_get_address(key);

  if(kvaddr == 0) kvaddr = flashstorage_keyval_get_unalloced();

  uint8_t *page_address    = flashstorage_get_pageaddr((uint8_t *) kvaddr);
  flashstorage_readpage(page_address,pagedata);
  uint32_t data_offset = ((uint32_t) kvaddr) - ((uint32_t) page_address);

  // update page data
  for(uint32_t n=0;n<keyval_size_all;n++) {
    pagedata[n+data_offset] = new_keyval_data[n];
  }

  // write new page data
  flashstorage_unlock();
  bool eraseret = flashstorage_erasepage(page_address);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage(pagedata,page_address);
  flashstorage_lock();
}

// zero out flash logging area (only the first page as this resets the size)
void flashstorage_log_clear() {

  uint8_t pagedata[pagesize];

  for(uint32_t n=0;n<pagesize;n++) {
    pagedata[n] = 0;
  }

  uint8_t *page_address = flash_data_area_aligned+flash_log_base;
  
  // write new page data
  flashstorage_unlock();
  bool eraseret = flashstorage_erasepage(page_address);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage(pagedata,page_address);
  flashstorage_lock();
}

void flashstorage_log_size_set(uint32_t new_size) {

  uint8_t pagedata[pagesize];
  uint8_t *page_address = flash_data_area_aligned+flash_log_base;
  flashstorage_readpage(page_address,pagedata);

  ((uint32_t *) pagedata)[0] = new_size;

  flashstorage_unlock();
  flashstorage_erasepage(page_address);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage(pagedata,page_address);
  flashstorage_lock();
}

void flashstorage_log_pushback(uint8_t *data,uint32_t size) {

  uint32_t flash_data_size = flashstorage_log_size();

  // If we're falling off the end of the logging area just return, -2048 to cope with any potential edge case, TODO: more testing.
  if((flash_data_size+size) > (flash_data_area_aligned_size-2048)) return;

  // 1. Identify current page.
  uint8_t *address      = flash_data_area_aligned+flash_log_base+4+flash_data_size;
  uint32_t excess       = ((uint32_t) address)%pagesize;
  uint8_t *page_address = address-excess;

  uint8_t  *data_position = data;
  uint32_t write_size     = size;


  // 2. Write data segment covering current page.
  if(excess != 0) {
    uint8_t pagedata[pagesize];
    flashstorage_readpage(page_address,pagedata);

    for(uint32_t n=excess;n<pagesize;n++) {
      pagedata[n] = *data_position;
      data_position++;
      write_size--;
      if(write_size==0) break;
    }

    flashstorage_unlock();
    flashstorage_erasepage(page_address);
    flashstorage_lock();
    flashstorage_unlock();
    flashstorage_writepage(pagedata,page_address);
    flashstorage_lock();
    page_address += pagesize;
  }

  // 3. Write full pages until all data is written
  for(;write_size != 0;) {
    uint8_t pagedata[pagesize];
    for(uint32_t n=0;n<pagesize;n++) {
      pagedata[n] = *data_position;
      data_position++;
      write_size--;
      if(write_size == 0) break;
    }
    
    flashstorage_unlock();
    flashstorage_erasepage(page_address);
    flashstorage_lock();
    flashstorage_unlock();
    flashstorage_writepage(pagedata,page_address);
    flashstorage_lock();
    page_address += pagesize;
  }

  // 4. Update log size
  flashstorage_log_size_set(flash_data_size+size);
}


uint32_t flashstorage_log_size() {
  return ((uint32_t *)(flash_data_area_aligned+flash_log_base))[0];
}

uint8_t *flashstorage_log_get() {
  return flash_data_area_aligned+(flash_log_base+4);
}
