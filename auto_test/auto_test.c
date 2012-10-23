/**
 * host-side code for FLASH boot loader
 *
 * designed to target a linux with serial ports pre-configured
 **/

// http://www.easysw.com/~mike/serial/serial.html
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <time.h>

// #define dbg_printf  printf
#define dbg_printf dummyp

int dummyp(const char *format, ...) {
  return 0;
}

#define SERIALPORT "/dev/ttyUSB0"

// test numbers (just a unique number per test, could also use enum type here)
#define GUIDTEST  0
#define PKTEST    1
#define VALIDTEST 2
#define PUBTEST   3
#define LOGTEST   4
#define RTCTEST   5

// file descriptors
int serial;
FILE *infile;

int better_sleep (double sleep_time)
{
 struct timespec tv;
 /* Construct the timespec from the number of whole seconds... */
 tv.tv_sec = (time_t) sleep_time;
 /* ... and the remainder in nanoseconds. */
 tv.tv_nsec = (long) ((sleep_time - tv.tv_sec) * 1e+9);

 while (1)
 {
   /* Sleep for the time specified in tv. If interrupted by a
      signal, place the remaining time left to sleep back into tv. */
  int rval = nanosleep (&tv, &tv);
  if (rval == 0)
    /* Completed the entire sleep time; all done. */
   return 0;
  else if (errno == EINTR)
    /* Interrupted by a signal. Try again. */
   continue;
  else 
    /* Some other error; bail out. */
   return rval;
 }
 return 0;
}

int putString( unsigned char *str ) {
  int i = 0;
  int bytesSent = 0;

  while( str[i] != '\0' ) {
    bytesSent += write( serial, &(str[i]), sizeof(unsigned char) );
    //    putchar( str[i] );
    i++;
  }
  return(bytesSent);
}
int CPputs( char *str ) {
  return(putString((unsigned char *)str));
}

int CPputc( char c ) {
  //  putchar(c);
  fflush(stdout);
  return( write( serial, &c, sizeof(unsigned char) ) );
}
// pass me a string to a buffer with unsigned int *len characters
// unsigned int *len gets the actual number of characters received
int getString( unsigned char *str, unsigned int *len ) {
  int i = 0;
  unsigned char c;
  int done = 0;

  while( (i < *len) && !done ) {
    read( serial, &c, sizeof(unsigned char) );
    str[i] = c;
    //    putchar(c);
    if( c == '>' ) {
      done = 1;
      //      putchar('.');
    }
    i++;
  }
  *len = i;

  return i;
}

void setlinuxterm() {
  struct termios options;
  bzero(&options, sizeof(options));

  options.c_cflag |= (CLOCAL | CREAD | CS8 | B115200);

  // dirt simple. don't interpret. we really want all those non-ascii characters!
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;

  // this is key. otherwise we end up having the character input block all the time. yuck!
  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 1;

  // commit options
  tcflush(serial, TCIFLUSH);
  tcsetattr(serial, TCSANOW, &options);  
}

void configurePort() {
  struct termios options, oldoptions;
 
  // Get the current options for the port...
  tcgetattr(serial, &oldoptions);
  // use a big hammer. Because we don't want previous config options to mess us up!
  bzero(&options, sizeof(options));

  options.c_cflag |= (CLOCAL | CREAD | CS8 | B115200);

  // dirt simple. don't interpret. we really want all those non-ascii characters!
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;

  // this is key. otherwise we end up having the character input block all the time. yuck!
  options.c_cc[VTIME] = 0;
  options.c_cc[VMIN] = 1;

  // commit options
  tcflush(serial, TCIFLUSH);
  tcsetattr(serial, TCSANOW, &options);  
  
}


int main(int argc, char **argv) {
  unsigned char *ibuf;
  unsigned char iBuf[16384];
  unsigned int len = 16384;
  int testNum;
  unsigned char timeBuf[256];

  if( argc == 1 ) {
    printf( "\n\nMicrocontroller testing: defaulting to guid test. \n" );
    printf( "Valid test args: guid pktest valid pub log rtc\n" );
  }
  if( argc > 1 ) {
    if( strcmp(argv[1], "guid") == 0 )
      testNum = GUIDTEST;
    else if( strcmp(argv[1], "pktest") == 0 )
      testNum = PKTEST;
    else if( strcmp(argv[1], "valid") == 0 )
      testNum = VALIDTEST;
    else if( strcmp(argv[1], "pub") == 0 )
      testNum = PUBTEST;
    else if( strcmp(argv[1], "log") == 0 )
      testNum = LOGTEST;
    else if( strcmp(argv[1], "rtc") == 0 )
      testNum = RTCTEST;
    else {
      printf( "test specifier not valid, aborting.\n" );
      exit(0);
    } 
    printf( "\nPerforming %s test.\n", argv[1] );

  } else {
    testNum = GUIDTEST;
    printf( "\nPerforming guid test\n", argv[1] );
  }

  printf( "opening port\n" );
  fflush(stdout);
  //  serial = open( SERIALPORT, O_RDWR | O_NOCTTY | O_NDELAY );
  serial = open( SERIALPORT, O_RDWR /*| O_NOCTTY | O_NDELAY*/ );
  //  printf( "port open\n" );
  fflush(stdout);
  if( serial == -1 ) {
    printf( "Cannot open %s for read/write access.\n", SERIALPORT );
    return 1;
  } else {
    fcntl(serial, F_SETFL, 0);
  }
  //  printf( "configuring port\n" );
  fflush(stdout);
  configurePort();  // configure the serial port
  printf( "port configured\n" );
  fflush(stdout);

  fflush(stdout);

  switch( testNum) {
  case GUIDTEST:
    putString( "GUID\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );
    break;

  case PKTEST:
    putString( "TESTSIGN\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );
    break;

  case VALIDTEST:
    putString( "KEYVALID\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );
    break;

  case PUBTEST:
    putString( "PUBKEY\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );
    break;

  case LOGTEST:
    putString( "LOGXFER\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );

    fflush(stdout);
    // reset len if it's being used again
    len = 16384;
    putString( "LOGSIG\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );

    printf( "%s", iBuf );
    break;
    
  case RTCTEST:
    putString( "SETRTC\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );
    printf( "%s", iBuf );
    
    sprintf( timeBuf, "%d", (unsigned int) time(NULL));
    putString( timeBuf );
    putString( "\n" );
    getString( iBuf, &len );
    iBuf[len] = '\0';
    printf( "received %d characters:\n", len );
    printf( "%s", iBuf );
    break;

  default:
    printf( "test not yet implemented, nothing happened.\n" );
  }

  return 0;
}
