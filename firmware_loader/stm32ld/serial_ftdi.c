// Serial inteface implementation for POSIX-compliant systems

#include "serial.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "stm32ld.h"
#include <ftd2xx.h>

static u32 ser_timeout = SER_INF_TIMEOUT;
extern FT_HANDLE	ftHandle[MAX_DEVICES];
extern int ser_dbg;

// Open the serial port
ser_handler ser_open( const char* sername )
{
  int fd;

  if( ( fd = open( sername, O_RDWR | O_NOCTTY | O_NDELAY ) ) == -1 )
    perror( "ser_open: unable to open port" );
  else
    fcntl( fd, F_SETFL, 0 );
  return ( ser_handler )fd;
}

// Close the serial port
void ser_close( ser_handler id )
{
  close( ( int )id );
}

// Helper function: get baud ID from actual baud rate
#define BAUDCASE(x)  case x: return B##x
static u32 ser_baud_to_id( u32 baud )
{
  switch( baud )
  {
    BAUDCASE( 1200 );
    BAUDCASE( 1800 );
    BAUDCASE( 2400 );
    BAUDCASE( 4800 );
    BAUDCASE( 9600 );
    BAUDCASE( 19200 );
    BAUDCASE( 38400 );
    BAUDCASE( 57600 );
    BAUDCASE( 115200 );
    BAUDCASE( 230400 );
  }
  return 0;
}

// Helper function: get number of bits ID from actual number of bits
#define NBCASE(x) case x: return CS##x
static int ser_number_of_bits_to_id( int nb )
{
  switch( nb )
  {
    NBCASE( 5 );
    NBCASE( 6 );
    NBCASE( 7 );
    NBCASE( 8 );
  }
  return 0;
}

#if 0
int ser_setup( ser_handler id, u32 baud, int databits, int parity, int stopbits )
{
  struct termios termdata;
  int hnd = ( int )id;

  usleep( 200000 );
  tcgetattr( hnd, &termdata );

  // Baud rate
  cfsetispeed( &termdata, ser_baud_to_id( baud ) );
  cfsetospeed( &termdata, ser_baud_to_id( baud ) );

  // Parity / stop bits
  termdata.c_cflag &= ~CSTOPB;
  if( parity == SER_PARITY_NONE ) // no parity
  {
    termdata.c_cflag &= ~PARENB;
  }
  else if( parity == SER_PARITY_EVEN ) // even parity
  {
    termdata.c_cflag |= PARENB;
    termdata.c_cflag &= ~PARODD;
  }
  else if( parity == SER_PARITY_ODD ) // odd parity
  {
    termdata.c_cflag |= PARENB;
    termdata.c_cflag |= PARODD;
  }

   // Data bits
  termdata.c_cflag |= ( CLOCAL | CREAD );
  termdata.c_cflag &= ~CSIZE;
  termdata.c_cflag |= ser_number_of_bits_to_id( databits );

  // Disable HW and SW flow control
  termdata.c_cflag &= ~CRTSCTS;
  termdata.c_iflag &= ~( IXON | IXOFF | IXANY );

  // Raw input
  termdata.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );

  // Raw output
  termdata.c_oflag &= ~OPOST;

  // Check and strip parity bit
  if( parity == SER_PARITY_NONE )
    termdata.c_iflag &= ~( INPCK | ISTRIP );
  else
    termdata.c_iflag |= ( INPCK | ISTRIP );

  // Setup timeouts
  termdata.c_cc[ VMIN ] = 1;
  termdata.c_cc[ VTIME ] = 0;

  // Set the attibutes now
  tcsetattr( hnd, TCSANOW, &termdata );

  // Flush everything
  tcflush( hnd, TCIOFLUSH );

  // And set blocking mode by default
  fcntl( id, F_SETFL, 0 );
}
#endif

// Read up to the specified number of bytes, return bytes actually read
u32 ser_read( int id, u8* dest, u32 maxsize )
{
  DWORD dwRxSize;
  FT_STATUS	ftStatus;
  DWORD dwBytesRead;

  if( ser_timeout == SER_INF_TIMEOUT ) {
    printf( "infinite timeout selected.\n" );
    FT_SetTimeouts(ftHandle[id], 10000000, 10000000); // a couple hours to timeout...
    dwRxSize = 0;
    ftStatus = FT_OK;
    while ((dwRxSize < maxsize) && (ftStatus == FT_OK)) {
      ftStatus = FT_GetQueueStatus(ftHandle[id], &dwRxSize);
    }

    if(ftStatus == FT_OK) {
      if((ftStatus = FT_Read(ftHandle[id], dest, maxsize, &dwBytesRead)) != FT_OK) {
	printf("Error FT_Read(%d)\n", ftStatus);
      }
      else {
	if( ser_dbg )
	  printf("FT_Read = %d\n", dwBytesRead);
      }
    }
    else {
      printf("Error FT_GetQueueStatus(%d)\n", ftStatus);
      return 0;
    }
    return (u32) dwBytesRead;
  }
  else
  {
    fd_set readfs;
    struct timeval tv;
    int retval;

    if( ser_timeout == 0 )
      ser_timeout = 1;
    //    printf( "setting timeout to %d\n", ser_timeout );
    FT_SetTimeouts(ftHandle[id], ser_timeout, ser_timeout);

    if((ftStatus = FT_Read(ftHandle[id], dest, maxsize, &dwBytesRead)) != FT_OK) {
      printf("Error FT_Read(%d)\n", ftStatus);
    }
    else {
      ftStatus = FT_GetQueueStatus(ftHandle[id], &dwRxSize);
      
      if( ser_dbg )
	printf("  - ser_read [%d | %02x] . %d\n", dwBytesRead, *dest & 0xFF, dwRxSize);
    }

    return (u32) dwBytesRead;
  }
}

// Read a single byte and return it (or -1 for error)
int ser_read_byte( int id )
{
  u8 data;
  int res = ser_read( id, &data, 1 );

  return res == 1 ? data : -1;
}

// Write up to the specified number of bytes, return bytes actually written
u32 ser_write( int id, const u8 *src, u32 size )
{
  u32 res;
  FT_STATUS	ftStatus;
  DWORD dwBytesWritten;
  
  FT_SetTimeouts(ftHandle[id], ser_timeout, ser_timeout);

  if((ftStatus = FT_Write(ftHandle[id], (char *) src, size, &dwBytesWritten)) != FT_OK) {
    printf("Error FT_Write(%d)\n", ftStatus);
  }
  if( ser_dbg )
    printf("  - ser_wrt  [%d | %02x] *\n", dwBytesWritten, *src & 0xFF);

  return (u32) dwBytesWritten;
}

// Write a byte to the serial port
u32 ser_write_byte( int id, u8 data )
{
  return ( u32 )ser_write( id, &data, (u32) 1 );
}

// Set communication timeout
void ser_set_timeout_ms( ser_handler id, u32 timeout )
{
  ser_timeout = timeout;
}

