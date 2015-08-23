#ifndef LOG_H
#define LOG_H

#include <stdint.h>

#define LOG_TYPE_CPM30  0  // 30 seconds log interval when unit alarmed
#define LOG_TYPE_CPM    1  // Standard CPM with unit on
#define LOG_TYPE_MINMAX 3  // TODO: Is this needed ?

/**
 * Standard log data for logging
 */
struct log_data_t {
  uint32_t time;				// Unix timestamp
  uint32_t duration;			// Duration of the window in seconds
  uint32_t cpm;					// Int value (can be up to 100k hence 32bit uint)
  uint32_t cpm_min;				// Min value over duration
  uint32_t cpm_max;				// Max value over duration
  uint32_t counts;				// Number of counts during the window
  uint8_t log_type;      // Bit 7 (MSB) is Magsensor state at log start
  	  	  	  	  	  	 // Bit 6 is magsensor state at log end
  	  	  	  	  	  	 // Bit 0/1 are log type
}; // 25 bytes

#endif
