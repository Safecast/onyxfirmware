// STM32 bootloader client

#include <stdio.h>
#include "serial.h"
#include "type.h"
#include "stm32ld.h"

//static ser_handler stm32_ser_id = ( ser_handler )-1;
int stm32_ser_id = 0;
extern void safecast_resetboard(int mode);

#define STM32_RETRY_COUNT             10

// ****************************************************************************
// Helper functions and macros

// Check initialization
#define STM32_CHECK_INIT\
  if( stm32_ser_id == ( ser_handler )-1 )\
    return STM32_NOT_INITIALIZED_ERROR

// Check received byte
#define STM32_EXPECT( expected )\
  if( stm32h_read_byte() != expected )\
    return STM32_COMM_ERROR;

#define STM32_READ_AND_CHECK( x )\
  if( ( x = stm32h_read_byte() ) == -1 )\
    return STM32_COMM_ERROR;

// Helper: send a command to the STM32 chip
static int stm32h_send_command( u8 cmd )
{
  ser_write_byte( stm32_ser_id, cmd );
  ser_write_byte( stm32_ser_id, ~cmd );
}

// Helper: read a byte from STM32 with timeout
static int stm32h_read_byte()
{
  return ser_read_byte( stm32_ser_id );
}

// Helper: append a checksum to a packet and send it
static int stm32h_send_packet_with_checksum( u8 *packet, u32 len )
{
  u8 chksum = 0;
  u32 i;

  for( i = 0; i < len; i ++ )
    chksum ^= packet[ i ];
  ser_write( stm32_ser_id, packet, len );
  ser_write_byte( stm32_ser_id, chksum );
  return STM32_OK;
}

// Helper: send an address to STM32
static int stm32h_send_address( u32 address )
{
  u8 addr_buf[ 4 ];

  addr_buf[ 0 ] = address >> 24;
  addr_buf[ 1 ] = ( address >> 16 ) & 0xFF;
  addr_buf[ 2 ] = ( address >> 8 ) & 0xFF;
  addr_buf[ 3 ] = address & 0xFF;
  return stm32h_send_packet_with_checksum( addr_buf, 4 );
}

// Helper: intiate BL communication
static int stm32h_connect_to_bl()
{
  int res;

  // Flush all incoming data
  //  printf( "flushing data\n" );
  ser_set_timeout_ms( stm32_ser_id, SER_NO_TIMEOUT );
  while( stm32h_read_byte() != -1 );
  ser_set_timeout_ms( stm32_ser_id, STM32_COMM_TIMEOUT );

  //  printf( "starting comms\n" );
  // Initiate communication
  ser_write_byte( stm32_ser_id, STM32_CMD_INIT );
  res = stm32h_read_byte();
  return res == STM32_COMM_ACK || res == STM32_COMM_NACK ? STM32_OK : STM32_INIT_ERROR;
}

// ****************************************************************************
// Implementation of the protocol

// no port name or baud needed as ports are auto-enumerated by openports and
// the baud is hard coded to 115200
int stm32_init( const char *portname, u32 baud )
{
  stm32_ser_id = openSerialPorts(baud);
  
  safecast_resetboard(1);

  if( stm32_ser_id < 0 ) {
    printf( "Failed to find a valid device, exiting.\n" );
    return -1;
  }

  // Connect to bootloader
  return stm32h_connect_to_bl();
}

// Get bootloader version
// Expected response: ACK version OPTION1 OPTION2 ACK
int stm32_get_version( u8 *major, u8 *minor )
{
  u8 i, version;
  int temp, total;
  int tries = STM32_RETRY_COUNT;  

  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_COMMAND );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_READ_AND_CHECK( total );
  for( i = 0; i < total + 1; i ++ )
  {
    STM32_READ_AND_CHECK( temp );
    if( i == 0 )
      version = ( u8 )temp;
  }
  *major = version >> 4;
  *minor = version & 0x0F;
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}

// Get chip ID
int stm32_get_chip_id( u16 *version )
{
  u8 temp;
  int vh, vl;

  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_GET_ID );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( 1 );
  STM32_READ_AND_CHECK( vh );
  STM32_READ_AND_CHECK( vl );
  STM32_EXPECT( STM32_COMM_ACK );
  *version = ( ( u16 )vh << 8 ) | ( u16 )vl;
  return STM32_OK;
}

// Write unprotect
int stm32_write_unprotect()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_WRITE_UNPROTECT );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( STM32_COMM_ACK );
  // At this point the system got a reset, so we need to re-enter BL mode
  return stm32h_connect_to_bl();
}

// Erase flash
int stm32_erase_flash()
{
  u8 temp;

  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_ERASE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK );
  ser_write_byte( stm32_ser_id, 0xFF );
  ser_write_byte( stm32_ser_id, 0x00 );
  STM32_EXPECT( STM32_COMM_ACK );
  return STM32_OK;
}


// erase pages, starting at page_number for page_count pages
int stm32_erase_flash_page(u32 page_number,int page_count) {

  u8 checksum=0;
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_ERASE_FLASH );
  STM32_EXPECT( STM32_COMM_ACK ); // Check that I actually need this.
 // ser_write_byte( stm32_ser_id, page_count >> 8   );
  ser_write_byte( stm32_ser_id, page_count-1);// & 0xFF );
  checksum ^= (page_count-1);

  // Write list of pages to erase
  int n=0;
  for(n=0;n<page_count;n++) {
    ser_write_byte( stm32_ser_id, (n+page_number) );
    checksum ^= (n+page_number);
  //  ser_write_byte( stm32_ser_id, (n+page_number) >> 8   );
  //  ser_write_byte( stm32_ser_id, (n+page_number) & 0xFF );
  }
  ser_write_byte( stm32_ser_id, checksum ); // Other code indicates this should be a checksum
  STM32_EXPECT( STM32_COMM_ACK ); // Check that I actually need this.

  return STM32_OK;
}

// Write from a given address (stops when read_data_func is out of data)
int stm32_write_flash_page(u32 address_in,int page_count,p_read_data read_data_func, p_progress progress_func )
{
  u32 wrote = 0;
  u8 data[ STM32_WRITE_BUFSIZE + 1 ];
  int datalen;
  u32 address = address_in;
  //u32 datalen, address = STM32_FLASH_START_ADDRESS;

  int pagedata_size = page_count*2048;
  int wrote_data = 0;
 
  STM32_CHECK_INIT; 
  while( 1 )
  {
    // Read data to program
    if( ( datalen = read_data_func( data + 1, STM32_WRITE_BUFSIZE ) ) == 0 )
      break;

    if((wrote_data+datalen) > pagedata_size) {
      datalen = pagedata_size - wrote_data;
      if(datalen <= 0) break;
      if(wrote_data >= pagedata_size) break;
    }

    data[ 0 ] = ( u8 )( datalen - 1 );

    // Send write request
    stm32h_send_command( STM32_CMD_WRITE_FLASH );
    STM32_EXPECT( STM32_COMM_ACK );
    // Send address
    stm32h_send_address( address );
    STM32_EXPECT( STM32_COMM_ACK );

    // Send data
    stm32h_send_packet_with_checksum( data, datalen + 1 );
    STM32_EXPECT( STM32_COMM_ACK );

    // Call progress function (if provided)
    wrote += datalen;
    if( progress_func )
      progress_func( wrote );

    // Advance to next data
    address += datalen;
    wrote_data += datalen;
  }
  return STM32_OK;
}


// Program flash
// Requires pointers to two functions: get data and progress report
int stm32_write_flash( p_read_data read_data_func, p_progress progress_func )
{
  u32 wrote = 0;
  u8 data[ STM32_WRITE_BUFSIZE + 1 ];
  u32 datalen, address = STM32_FLASH_START_ADDRESS;

  STM32_CHECK_INIT; 
  while( 1 )
  {
    // Read data to program
    if( ( datalen = read_data_func( data + 1, STM32_WRITE_BUFSIZE ) ) == 0 )
      break;
    data[ 0 ] = ( u8 )( datalen - 1 );

    // Send write request
    stm32h_send_command( STM32_CMD_WRITE_FLASH );
    STM32_EXPECT( STM32_COMM_ACK );
    
    // Send address
    stm32h_send_address( address );
    STM32_EXPECT( STM32_COMM_ACK );

    // Send data
    stm32h_send_packet_with_checksum( data, datalen + 1 );
    STM32_EXPECT( STM32_COMM_ACK );

    // Call progress function (if provided)
    wrote += datalen;
    if( progress_func )
      progress_func( wrote );

    // Advance to next data
    address += datalen;
  }
  return STM32_OK;
}


int stm32_read_flash( u32 offset, u32 datalen )
{
  u32 wrote = 0;
  u8 data[ STM32_WRITE_BUFSIZE + 1 ];
  u32 address = STM32_FLASH_START_ADDRESS + offset;
  int i;

  STM32_CHECK_INIT; 
  while( 1 ) {
    if( address > offset + datalen + STM32_FLASH_START_ADDRESS ) {
      break;
    }

    // Send write request
    stm32h_send_command( STM32_CMD_READ_FLASH );
    STM32_EXPECT( STM32_COMM_ACK );
    
    // Send address
    stm32h_send_address( address );
    STM32_EXPECT( STM32_COMM_ACK );

    // 128 bytes of data to return
    ser_write_byte( stm32_ser_id, 0x7f ); // 127 
    ser_write_byte( stm32_ser_id, 0x80 ); // compliment checksum
    STM32_EXPECT( STM32_COMM_ACK );

    for( i = 0; i < 128; i++ ) {
      if( (i % 16) == 0 ) 
	printf( "\n%04x: ", (unsigned int) address + i );
      
      printf( "%02x ", stm32h_read_byte() );
    }
    // Advance to next data
    address += 128;
  }
  printf( "\n" );

  return STM32_OK;
}

// Read unprotect
int stm32_read_unprotect()
{
  STM32_CHECK_INIT;
  stm32h_send_command( STM32_CMD_READ_UNPROTECT );
  STM32_EXPECT( STM32_COMM_ACK );
  STM32_EXPECT( STM32_COMM_ACK );
  // At this point the system got a reset, so we need to re-enter BL mode
  return stm32h_connect_to_bl();
}
