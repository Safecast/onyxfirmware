#include "flashstorage.h"
#include "flash.h"
#include "utils.h"
#include "safecast_wirish_types.h"
#include "display.h"
#include "log.h"
#include <string.h>
#include "serialinterface.h"
#include "buzzer.h"
#include "usart.h"
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
uint8_t *flash_data_area_aligned;        // Starting address of the data area,
// aligned to page boundary

uint32_t flash_data_area_aligned_size;   // Size of the usable data area

extern Geiger *system_geiger;
extern Controller *system_controller;

bool log_pause = false;

/**
 * The flash storage area serves two purposes:
 *  - Store device settings
 *  - Store logs
 */
#define settings_area_size  2048 	// Space reserved for storing settings. This is
									// 1 page in the flash area, since pages are 2048 bytes long.
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

/**
 * Wait for the Flash to be ready (loops until Flash Busy is clear)
 */
bool flashstorage_writewait() {
	// wait to write
	for (uint32_t n = 0; FLASH_BASE->SR & (1 << 0); n++) {
		if (n >= 40000000) {
			return false;
		}
	}
	return true;
}

bool flashstorage_unlock() {

	// unlock, using STM32 magic numbers
	FLASH_BASE->KEYR = 0x45670123;
	FLASH_BASE->KEYR = 0xCDEF89AB;

	// verify not locked
	if (!flashstorage_islocked())
		return false;

	return true;
}

bool flashstorage_lock() {

	// lock
	FLASH_BASE->CR = 0x80;

	// verify locked
	if (flashstorage_islocked())
		return true;

	return false;
}

bool flashstorage_islocked() {
	if (FLASH_BASE->CR & (1 << 7))
		return true;
	return false;
}

/**
 * Erase a flash page as per recommendations in PM0075 (ST Documentation)
 */
bool flashstorage_erasepage(uint8_t *pageaddr) {

	if (flashstorage_islocked())
		return false;

	// FLASH_BASE->CR |= 1<<1;
	// FLASH_BASE->AR = (uint32_t) pageaddr;

	// Check the Busy flag, we don't want to do anything if the flash
	// is busy.
	if (flashstorage_writewait() == false)
		return false;

	FLASH_BASE->CR |= 1 << 1;   			// Set PER (Page Erase) bit
	FLASH_BASE->AR = (uint32_t) pageaddr;   // Select page address
	FLASH_BASE->CR |= 1 << 6;	// Set STRT bit - triggers the erase operation

	// Wait for complete, or return false if timeout.
	if (flashstorage_writewait() == false)
		return false;

	// Verify erase
	for (uint32_t i = 0; i < pagesize; i++) {
		if (flashstorage_writewait() == false)
			return false;
		if (pageaddr[i] != 0xFF)
			return false;
	}
	return true;
}

bool flashstorage_writepage(uint8_t *new_data, uint8_t *pageaddr) {

	if (flashstorage_islocked())
		return false;
	if (flashstorage_writewait() == false)
		return false;

	FLASH_BASE->CR |= 1 << 0;		// set PG bit to one (Flash Programming)

	for (uint32_t n = 0; n < (pagesize / 2); n++) {
		// write MUST be 16bits at a time.
		((uint16_t *) pageaddr)[n] = ((uint16_t *) new_data)[n];
		if (flashstorage_writewait() == false)
			return false;
		// Verify that programming worked by reading the value back
		if (((uint16_t *) pageaddr)[n] != ((uint16_t *) new_data)[n]) {
			return false;
		}
	}

	return true;
}

/**
 * Erase a flash page, retrying up to 20 times
 *
 *  TODO: remove those stupid retries!
 */
bool flashstorage_erasepage_rt(uint8_t *pageaddr) {

	// Check that we are at a page boundary:
	if (((uint32_t) pageaddr) % pagesize != 0)
		return false;

	for (int n = 0; n < 20; n++) {
		bool ret = flashstorage_erasepage(pageaddr);
		if (ret == true)
			return true;
		// DEBUG: there no good reason why we should
		// have to retry
		buzzer_morse_debug("F ERASE E");
	}
	return false;
}

// Flash write page with retry
bool flashstorage_writepage_rt(uint8_t *new_data, uint8_t *pageaddr) {
	for (int n = 0; n < 20; n++) {

		bool ret = flashstorage_writepage(new_data, pageaddr);
		if (ret == true)
			return true;
		// DEBUG: there no good reason why we should
		// have to retry
		buzzer_morse_debug("F WRITE E");
	}
	return false;
}

/**
 * Reads a flash page. page_address needs to be at the page boundary,
 * we check this.
 */
void flashstorage_readpage(uint8_t *page_address, uint8_t *pagedata) {
	// Return if we are not a page boundary
	if (((uint32_t) page_address) % pagesize != 0)
		return;

	for (uint32_t n = 0; n < pagesize; n++) {
		pagedata[n] = page_address[n];
	}
}

/**
 * Returns the base page address for a given address in flash
 */
uint8_t *flashstorage_get_pageaddr(uint8_t *keyaddr) {
	uint32_t rem = ((uint32_t) keyaddr) % pagesize;
	return keyaddr - rem;
}

/**
 * Debug utility: dump the flash storage area
 */
void hexdump(unsigned char *buffer, unsigned long index, unsigned long width) {
	unsigned long i;
	char str[10];
	for (i = 0; i < index; i++) {
		sprintf(str, "%02x ", buffer[i]);
		serial_write_string(str);
	}
	for (unsigned long spacer = index; spacer < width; spacer++)
		serial_write_string("	");
	serial_write_string(": ");
	for (i = 0; i < index; i++) {
		if (buffer[i] < 32)
			serial_write_string(".");
		else {
			sprintf(str, "%c", buffer[i]);
			serial_write_string(str);
		}
	}
	serial_write_string("\r\n");
}

/**
 * Dump settings or log area in hex format
 */
void flashstorage_dump_hex() {
	uint32_t max_size = flash_data_area_aligned_size;
	uint8_t *min_addr = flash_data_area_aligned;


	for (uint32_t n = 0; n < (max_size - 16); n += 16) {
				if (n == settings_area_size) {
			serial_write_string(
					"\r\n-----------\r\nLogging Area\r\n-----------\r\n");
		}
		uint8_t ff = 0;
		uint8_t zr = 0;
		for (uint8_t i =0; i < 16; i++) {
			if (min_addr[n] == 0xff)
				ff++;
			if (min_addr[n] == 0x00)
				zr++;
		}
		if (ff == 16) {
			serial_write_string("f");
		} else if (zr == 16) {
			serial_write_string("0");
		} else {
			hexdump((unsigned char *) (min_addr + n), 16, 16);
		}
	}

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
	if ((flash_addr % pagesize) != 0) {
		unusable = pagesize - (flash_addr % pagesize);
		flash_addr += unusable;
	}

	flash_data_area_aligned = (uint8_t *) flash_addr;
	flash_data_area_aligned_size =
			(uint32_t) (&_binary___binary_data_flash_data_size) - unusable;
}

/**
 * Internal function, returns the flash address of a settings
 * key.
 * returns 0 if the key is not found.
 */
char *flashstorage_keyval_get_address(const char *key) {

	for (uint32_t n = 0; n < settings_area_size; n += (keyval_size * 2)) {
		if (flash_data_area_aligned[n] == 0xff) {
			// We are at the end of the settings keys and found nothing.
			return 0;
		}
		if (strcmp(key, (const char *) (flash_data_area_aligned + n)) == 0) {
			uint8_t *addr = ((flash_data_area_aligned) + n);
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
	for (uint32_t n = 0; n < settings_area_size; n += (keyval_size * 2)) {
		if (flash_data_area_aligned[n] == 0xff) {
			return (char *) (flash_data_area_aligned) + n;
		}
	}
	return 0;
}

/**
 * Get the content of a setting by its index.
 */
void flashstorage_keyval_by_idx(int idx, char *key, char *val) {

	uint32_t n = (keyval_size * 2) * idx;

	if (flash_data_area_aligned[n] == 0xff) {
		key[0] = 0;
		val[0] = 0;
		return;
	}

	strcpy(key, (char *) flash_data_area_aligned + n);
	strcpy(val, (char *) flash_data_area_aligned + n + keyval_size);

	// Safeguard:
	key[keyval_size - 1] = 0;
	val[keyval_size - 1] = 0;

}

/**
 * Get the value of a settings key
 *
 * returns a char* pointing to the value, or 0
 * if no key/value found
 */
const char *flashstorage_keyval_get(const char *key) {

	char *v = flashstorage_keyval_get_address(key);

	if (v != 0)
		return v + keyval_size;

	return 0;
}

/**
 * Stores a settings key value in Flash. If the key exists,
 * overwrite it, if it does not, then create a new one if there is
 * still space in the settings area.
 */
void flashstorage_keyval_set(const char *key, const char *value) {

	uint8_t pagedata[pagesize];
	uint8_t new_keyval_data[keyval_size_all];

	// Fill the new_keyval_data with 0xff so that we don't
	// unnecessarily wear out the flash with random values
	for (int i = 0; i < keyval_size_all; i++)
		new_keyval_data[i] = 0xff;

	strcpy((char *) new_keyval_data, (char *) key);
	strcpy((char *) new_keyval_data + keyval_size, (char *) value);

	// Just in case...
	new_keyval_data[keyval_size - 1] = 0;
	new_keyval_data[keyval_size_all - 1] = 0;

	// Read original page data
	char *kvaddr = flashstorage_keyval_get_address(key);

	// If we did not find this key, get a free address to store it
	if (kvaddr == 0)
		kvaddr = flashstorage_keyval_get_unalloced();

	// If there are no free addresses, quit.
	if (kvaddr == 0)
		return;

	uint8_t *page_address = flashstorage_get_pageaddr((uint8_t *) kvaddr);
	flashstorage_readpage(page_address, pagedata);
	uint32_t data_offset = ((uint32_t) kvaddr) - ((uint32_t) page_address);

	// Update page data with key/value:
	for (uint32_t n = 0; n < keyval_size_all; n++) {
		pagedata[n + data_offset] = new_keyval_data[n];
	}

	// There is a lingering issue with the firmware, where very heavy
	// traffic on the serial port can lead to crashes. For this reason,
	// we disable USART1 when we delete then write to flash, to avoid
	// a potential crash after page deletion but before page write, which
	// would corrupt the settings area completely.
	usart_disable(USART1);
	// write new page data
	flashstorage_unlock();
	flashstorage_erasepage_rt(page_address);
	flashstorage_lock();
	flashstorage_unlock();
	flashstorage_writepage_rt(pagedata, page_address);
	flashstorage_lock();
	usart_enable(USART1);
}

//////////////////
//   Flash log management
//////////////////

/**
 * Return the base address of the log area
 */
uint8_t *flashstorage_log_baseaddress() {
	return flash_data_area_aligned + settings_area_size;
}

/**
 * Clear the flash log area. We need to clear all pages because of the way
 * we look for free space across pages (see flashstorage_log_size below)
 */
void flashstorage_log_clear() {

	uint8_t *page_address = flashstorage_log_baseaddress(); // Get base address of log area
	usart_disable(USART1);
	for (;
			page_address
					< (flash_data_area_aligned + flash_data_area_aligned_size);
			) {
		// Erase the page
		flashstorage_unlock();
		flashstorage_erasepage_rt(page_address);
		flashstorage_lock();
		page_address += pagesize;
	}
	usart_enable(USART1);
}

/**
 * Returns the size (in bytes) of the current log.
 * The size is also the offset where we can start writing a new
 * log entry.
 */
uint32_t flashstorage_log_size() {

	// scan the log, find the first unused log entry.
	uint8_t *logbase = flashstorage_log_baseaddress();
	uint32_t recsize = sizeof(log_data_t);

	for (uint32_t n = 0; n < (flash_data_area_aligned_size - settings_area_size); n += recsize) {
		// Check if the next record is only 0xff (empty)
		// Note: previous implementations checked successive byte values, but not
		// aligned to log_data_t size, leading to random log corruption. This is now
		// fixed.
		uint8_t empty = true;
		for (uint8_t j = 0; j < recsize; j++) {
			if (logbase[n+j] != 0xff) {
				empty = false;
				break;
			}
		}
		if (empty)
			return n; // we found an empty slot, return the size of the log
	}
	// there doesn't seem to a page with any free data, so return max size.
	return flash_data_area_aligned_size - settings_area_size;
}

/**
 * Returns true if the log area is full
 */
bool flashstorage_log_isfull() {
	if (flashstorage_log_size()
			== (flash_data_area_aligned_size - settings_area_size))
		return true;
	return false;
}

/**
 * Get the maximum number of records that can be stored in the flash
 */
uint32_t flashstorage_log_maxrecords() {
	// Careful to remove the space reserved for settings (flash_log_base)
	uint32_t records = (flash_data_area_aligned_size - settings_area_size)
			/ sizeof(log_data_t);
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
int flashstorage_log_pushback(uint8_t *data, uint32_t size) {

	// if data logging is paused (usually for data transfer) just return.
	if (log_pause == true)
		return 1;

	uint32_t current_log_size = flashstorage_log_size();

	// If we're falling off the end of the logging area just return
	if ((current_log_size + size)
			> (flash_data_area_aligned_size - settings_area_size))
		return 2;

	// 1. Identify current page address.
	uint8_t *address = flashstorage_log_baseaddress() + current_log_size;
	uint32_t excess = ((uint32_t) address) % pagesize;
	uint8_t *page_address = address - excess; // Align on page boundary

	uint8_t *data_position = data;
	uint32_t write_size = size;

	// Sanity checks (we should never fail those tests!)
	if (page_address < flash_data_area_aligned)
		return 3;
	if (page_address > (flash_data_area_aligned + flash_data_area_aligned_size))
		return 4;

	// 2. Write data segment covering current page.
	if (excess != 0) {
		uint8_t pagedata[pagesize];
		flashstorage_readpage(page_address, pagedata);

		for (uint32_t n = excess; n < pagesize; n++) {
			pagedata[n] = *data_position;
			data_position++;
			write_size--;
			if (write_size == 0)
				break;
		}

		flashstorage_unlock();
		flashstorage_writepage_rt(pagedata, page_address);
		flashstorage_lock();
		page_address += pagesize;
	}

	// Sanity checks
	if (page_address < flash_data_area_aligned)
		return 5;

	if (page_address > (flash_data_area_aligned + flash_data_area_aligned_size))
		return 6;

	// 3. Write full pages until all data is written
	for (; write_size != 0;) {
		uint8_t pagedata[pagesize];
		uint32_t pagepos = 0;
		for (pagepos = 0; pagepos < pagesize; pagepos++) {
			if (write_size != 0) {
				pagedata[pagepos] = *data_position;
				data_position++;
				write_size--;
			} else {
				pagedata[pagepos] = 0xff;
			}
		}

		flashstorage_unlock();
		flashstorage_writepage_rt(pagedata, page_address);
		flashstorage_lock();
		page_address += pagesize;

		if (write_size == 0)
			break;

		// Sanity checks
		if (page_address < flash_data_area_aligned)
			return 7;
		if (page_address > (flash_data_area_aligned + current_log_size))
			return 8;
	}

	return 0;
}

void flashstorage_log_userchange() {
	uint8_t data[32];
	for (int n = 0; n < 32; n++)
		data[n] = 255;
	flashstorage_log_pushback(data, sizeof(log_data_t));
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

/**
 * Loads all stored settings into the running firmware.
 *
 */
void flashstorage_keyval_update() {
	const char *spulsewidth = flashstorage_keyval_get("PULSEWIDTH");
	if (spulsewidth != 0) {
		unsigned int c;
		sscanf(spulsewidth, "%u", &c);
		system_geiger->set_pulsewidth(c);
		system_geiger->pulse_timer_init();
	} else {
		system_geiger->set_pulsewidth(6);
	}

	const char *sbright = flashstorage_keyval_get("BRIGHTNESS");
	if (sbright != 0) {
		unsigned int c;
		sscanf(sbright, "%u", &c);
		display_set_brightness(c);
	}

	const char *sbeep = flashstorage_keyval_get("GEIGERBEEP");
	if (sbeep != 0) {
		if (!system_controller->m_sleeping) {
			if (strcmp(sbeep, "true") == 0) {
				system_geiger->set_beep(true);
				tick_item("Geiger Beep", true);
			} else
				system_geiger->set_beep(false);
		}
	}

	const char *scpmcps = flashstorage_keyval_get("CPMCPSAUTO");
	if (scpmcps != 0) {
		if (strcmp(scpmcps, "true") == 0) {
			system_controller->m_cpm_cps_switch = true;
			tick_item("CPM/CPS Auto", true);
		}
	}

	const char *svrem = flashstorage_keyval_get("SVREM");
	if (strcmp(svrem, "REM") == 0) {
		tick_item("Roentgen", true);
	} else {
		tick_item("Sievert", true);
	}
}
