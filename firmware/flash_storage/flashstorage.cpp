#include "flashstorage.h"
#include "flash.h"
#include "utils.h"
#include "safecast_wirish_types.h"
#include "display.h"
#include "log.h"
#include <string.h>
#include "serialinterface.h"
#include <stdio.h>
#include "Controller.h"
#include "Geiger.h"

/**
 * The two variables below are defined at link time, and are
 * pointers to the flash address to the"flash_data" structure that
 * comes from "binary_data/flash_data". flash_data is a 100k blob.
 */
extern uint32_t _binary___binary_data_flash_data_start;
extern uint32_t _binary___binary_data_flash_data_size;

/**
 * We have no assurance that the flash_data area starts at a page
 * boundary. The two variables below are initialized when we call
 * flash_initialize, and align the flash area to a page size - we can
 * have some unusable space between the start of the flash_data area and
 * the next page boundary.
 */
uint8_t  *flash_data_area_aligned;        // Starting address of the log area,
                                          // aligned to page boundary
uint32_t  flash_data_area_aligned_size;   // Size of the usable log area

extern Geiger *system_geiger;
extern Controller *system_controller;


bool log_pause = false;

/**
 * The flash storage area serves two purposes:
 *  - Store device settings
 *  - Store logs
 */
#define flash_log_base  10240 // Space reserved for storing settings
                              // the log area starts above this index

// Settings are stored as key/value, each with max 50 bytes
#define keyval_size     50   // Max size of a settings key name
#define keyval_size_all 100  // Max size of a settings key name + key value

// The size of a page of flash
// TODO: this should come from another place, not be hardcoded here, right ?
#define pagesize        2048


/*************************
 *   Utility functions for reading/writing in data_area
 **************************/

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

// Flash erase page with retry
bool flashstorage_erasepage_rt(uint8_t *pageaddr) {
  for(int n=0;n<20;n++) {

    bool ret = flashstorage_erasepage(pageaddr);
    if(ret == true) return true;
  }
  return false;
}

// Flash write page with retry
bool flashstorage_writepage_rt(uint8_t *new_data,uint8_t *pageaddr) {
  for(int n=0;n<20;n++) {

    bool ret = flashstorage_writepage(new_data,pageaddr);
    if(ret == true) return true;
  }
  return false;
}

/*************************
 *  END Utility functions for reading/writing in data_area
 **************************/



/**
 * Initialize the flash storage area for use by firmware, by setting
 * up the base address of the flash data area. That area has to start
 * at a page boundary.
 *
 * Has to be called by firmware at startup (see main.cpp)
 */
void flashstorage_initialise() {
  uint32_t flash_addr = (uint32_t) &_binary___binary_data_flash_data_start;

  // Check how much unusable space we have between start of flash_data
  // and the next page boundary:
  uint32_t unusable = 0;
  if((flash_addr%pagesize) != 0) {
    unusable = pagesize-(flash_addr%pagesize);
    flash_addr += unusable;
  }

  flash_data_area_aligned = (uint8_t *) flash_addr;
  flash_data_area_aligned_size = (uint32_t) (&_binary___binary_data_flash_data_size) - unusable;
}

/**
 * Internal function, returns the flash address of a settings
 * key.
 * returns 0 if the key is not found.
 */
char *flashstorage_keyval_get_address(const char *key) {

  for(uint32_t n=0;n<flash_log_base;n+=(keyval_size*2)) {

    if(flash_data_area_aligned[n] == 0) {
      // We are at the end of the settings keys and found nothing.
      return 0;
    }

    if(strcmp(key, (const char *) (flash_data_area_aligned+n)) == 0) {
      uint8_t *addr = ((flash_data_area_aligned)+n);
      return (char *) addr;
    }
  }

  return 0; // Did not find that settings key
}

/**
 * Find the first free address in flash area for storing
 * settings keys.
 *
 * Returns 0 if no space, otherwise returns the first free
 * address.
 */
char *flashstorage_keyval_get_unalloced() {
  for(uint32_t n=0;n<flash_log_base;n+=(keyval_size*2)) {

    if(flash_data_area_aligned[n] == 0) {
      return (char *) (flash_data_area_aligned)+n;
    }
  }
  return 0;
}

void flashstorage_keyval_by_idx(int idx,char *key,char *val) {

  uint32_t n = (keyval_size*2)*idx;

  if(flash_data_area_aligned[n] == 0) {
    key[0]=0;
    val[0]=0;
    return;
  }

  strcpy(key,(char *) flash_data_area_aligned+n);
  strcpy(val,(char *) flash_data_area_aligned+n+keyval_size);
}

/**
 * Get the value of a settings key
 *
 * returns a char* pointing to the value, or 0
 * if no key/value found
 */
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

/**
 * Set a settings key value.
 */
void flashstorage_keyval_set(const char *key,const char *value) {

  uint8_t pagedata[pagesize];
  uint8_t new_keyval_data[keyval_size_all];

  strcpy((char *) new_keyval_data            ,(char *)key);
  strcpy((char *) new_keyval_data+keyval_size,(char *)value);

  // just in case!
  new_keyval_data[keyval_size-1] = 0;
  new_keyval_data[keyval_size_all-1] = 0;

  // read original page data
  char *kvaddr     = flashstorage_keyval_get_address(key);

  // If we did not find this key, get a free address to store it
  if(kvaddr == 0) kvaddr = flashstorage_keyval_get_unalloced();

  // If there are no free addresses, quit.
  if(kvaddr == 0) return;

  uint8_t *page_address    = flashstorage_get_pageaddr((uint8_t *) kvaddr);
  flashstorage_readpage(page_address,pagedata);
  uint32_t data_offset = ((uint32_t) kvaddr) - ((uint32_t) page_address);

  // update page data
  for(uint32_t n=0;n<keyval_size_all;n++) {
    pagedata[n+data_offset] = new_keyval_data[n];
  }

  // write new page data
  flashstorage_unlock();
  flashstorage_erasepage_rt(page_address);
  flashstorage_lock();
  flashstorage_unlock();
  flashstorage_writepage_rt(pagedata,page_address);
  flashstorage_lock();
}


/**
 * Clear the flash log area. In practice, this zeroes out the flash logging area,
 * but only the first page as this resets the size.
 */
void flashstorage_log_clear() {

  uint8_t pagedata[pagesize];

  for(uint32_t n=0;n<pagesize;n++) {
    pagedata[n] = 0;
  }

  uint8_t *page_address = flash_data_area_aligned+flash_log_base;

  for(;page_address < (flash_data_area_aligned+flash_data_area_aligned_size);) {
    // write new page data
    flashstorage_unlock();
    flashstorage_erasepage_rt(page_address);
    flashstorage_lock();
    flashstorage_unlock();
    flashstorage_writepage_rt(pagedata,page_address);
    flashstorage_lock();
    page_address += pagesize;
  }
}

/**
 * Somehow this is empty ??
 */
void flashstorage_log_size_set(uint32_t new_size) {
}

/**
 * Returns the size (in bytes) of the current log
 */
uint32_t flashstorage_log_size() {

  // scan the log, find the first unused log entry.
  uint8_t *logbase = flashstorage_log_get();

  uint32_t zerocount=0;
  for(uint32_t n=0;n<(flash_data_area_aligned_size-pagesize);n++) {

    if(*(logbase+n) == 0) zerocount++;
                     else zerocount=0;

    if(zerocount > sizeof(log_data_t)) {
      // We're past the last log entry.
      return n-sizeof(log_data_t);
    }
  }

  // there doesn't seem to a page with any free data, so return max size.
  return flash_data_area_aligned_size-flash_log_base;
}

/**
 * Returns true if the log area is full
 */
bool flashstorage_log_isfull() {
  if(flashstorage_log_size() == (flash_data_area_aligned_size-flash_log_base)) return true;
  return false;
}

/**
 * Get the maximum number of records that can be stored in the flash
 */
uint32_t flashstorage_log_maxrecords() {
  // Careful to remove the space reserved for settings (flash_log_base)
  uint32_t records = (flash_data_area_aligned_size-flash_log_base) / sizeof(log_data_t);
  return records;
}

/**
 * Get the current number of records stored in flash
 */
uint32_t flashstorage_log_currentrecords() {
  uint32_t records = flashstorage_log_size() / sizeof(log_data_t);
  return records;
}

/**
 * Stores a new log entry. Usually called from controller/controller.cpp
 */
int flashstorage_log_pushback(uint8_t *data,uint32_t size) {

  // if datalogging is paused (usually for data transfer) just return.
  if(log_pause == true) return 1;

  uint32_t flash_data_size = flashstorage_log_size();

  // If we're falling off the end of the logging area just return, -2048 to cope with any potential edge case, TODO: more testing.
  if((flash_data_size+size) > (flash_data_area_aligned_size-2048)) return 2;

  // 1. Identify current page.
  uint8_t *address      = flash_data_area_aligned+flash_log_base+flash_data_size;
  uint32_t excess       = ((uint32_t) address)%pagesize;
  uint8_t *page_address = address-excess;

  uint8_t  *data_position = data;
  uint32_t write_size     = size;

  // Sanity checks
  if(page_address < flash_data_area_aligned) return 3;
  if(page_address > (flash_data_area_aligned+flash_data_area_aligned_size)) return 4;

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
    flashstorage_erasepage_rt(page_address);
    flashstorage_lock();
    flashstorage_unlock();
    flashstorage_writepage_rt(pagedata,page_address);
    flashstorage_lock();
    page_address += pagesize;
  }

  // Sanity checks
  if(page_address < flash_data_area_aligned) return 5;
  if(page_address > (flash_data_area_aligned+flash_data_area_aligned_size)) return 6;

  // 3. Write full pages until all data is written
  for(;write_size != 0;) {
    uint8_t pagedata[pagesize];
    uint32_t pagepos=0;
    for(pagepos=0;pagepos<pagesize;pagepos++) {
      if(write_size != 0) {
        pagedata[pagepos] = *data_position;
        data_position++;
        write_size--;
      } else {
        pagedata[pagepos]=0;
      }
    }

    flashstorage_unlock();
    flashstorage_erasepage_rt(page_address);
    flashstorage_lock();
    flashstorage_unlock();
    flashstorage_writepage_rt(pagedata,page_address);
    flashstorage_lock();
    page_address += pagesize;

    if(write_size == 0) break;

    // Sanity checks
    if(page_address < flash_data_area_aligned) return 7;
    if(page_address > (flash_data_area_aligned+flash_data_size)) return 8;
  }

  // 4. Update log size
  flashstorage_log_size_set(flash_data_size+size);

  return 0;
}

void flashstorage_log_userchange() {
  uint8_t data[32];
  for(int n=0;n<32;n++) data[n] = 255;
  flashstorage_log_pushback(data,sizeof(log_data_t));
}

/**
 * Pauses logging
 */
void flashstorage_log_pause() {
  log_pause = true;
}

/**
 * Resumes logging
 */
void flashstorage_log_resume() {
  log_pause = false;
}

/**
 * Check if log is paused
 */
bool flashstorage_logpaused() {
  return log_pause;
}

uint8_t *flashstorage_log_get() {
  return flash_data_area_aligned+flash_log_base;
}


/**
 * Loads all stored settings into the running firmware.
 *
 */
void flashstorage_keyval_update() {
  const char *spulsewidth = flashstorage_keyval_get("PULSEWIDTH");
  if(spulsewidth != 0) {
    unsigned int c;
    sscanf(spulsewidth, "%u", &c);
    system_geiger->set_pulsewidth(c);
    system_geiger->pulse_timer_init();
  }
  else {
    system_geiger->set_pulsewidth(6);
  }

  const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
  if(sbright != 0) {
    unsigned int c;
    sscanf(sbright, "%u", &c);
    display_set_brightness(c);
  }

  const char *sbeep = flashstorage_keyval_get("GEIGERBEEP");
  if(sbeep != 0) {
    if(!system_controller->m_sleeping) {
      if(strcmp(sbeep,"true") == 0) {
        system_geiger->set_beep(true);
        tick_item("Geiger Beep",true);
      }
      else system_geiger->set_beep(false);
    }
  }

  const char *scpmcps = flashstorage_keyval_get("CPMCPSAUTO");
  if(scpmcps != 0) {
    if(strcmp(scpmcps,"true") == 0) {
      system_controller->m_cpm_cps_switch = true;
      tick_item("CPM/CPS Auto",true);
    }
  }

  const char *svrem = flashstorage_keyval_get("SVREM");
  if(strcmp(svrem,"REM") == 0) {
    tick_item("Roentgen",true);
  }
  else {
    tick_item("Sievert",true);
  }
}
