/*
	Simple example to open a maximum of 4 devices - write some data then read it back.
	Shows one method of using list devices also.
	Assumes the devices have a loopback connector on them and they also have a serial number

	To build use the following gcc statement 
	(assuming you have the d2xx library in the /usr/local/lib directory).
	gcc -o simple main.c -L. -lftd2xx -Wl,-rpath /usr/local/lib
*/

#include "stm32ld/stm32ld.h"
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <limits.h>

#include <ftd2xx.h>

#define BL_VERSION_MAJOR  2
#define BL_VERSION_MINOR  1
#define BL_MKVER( major, minor )    ( ( major ) * 256 + ( minor ) ) 
#define BL_MINVERSION               BL_MKVER( BL_VERSION_MAJOR, BL_VERSION_MINOR )

#define CHIP_ID           0x0414

#define BUF_SIZE 0x10

FT_HANDLE	ftHandle[MAX_DEVICES];
char 	        cBufLD[MAX_DEVICES][64];
int             foundDevices = 0;
extern int stm32_ser_id;
int ser_dbg = 0;

static  FILE *fp_page1;
static  FILE *fp_page4;
static  u32 fpsize;

int fw_abort();

#define CBUS_RESET_MASK   0x1
#define CBUS_BOOT_MASK    0x4

int getandcheckCBUS( FT_HANDLE ftHandle0 ) {
  FT_PROGRAM_DATA Data;
  int need_write = 0;
  FT_STATUS	ftStatus;

  if( ser_dbg )
    printf("ftHandle0 = %p\n", ftHandle0);

  Data.Signature1 = 0x00000000;
  Data.Signature2 = 0xffffffff;
  Data.Manufacturer = (char *)malloc(256);		// "FTDI"
  Data.ManufacturerId = (char *)malloc(256);	// "FT"
  Data.Description = (char *)malloc(256);			// "USB HS Serial Converter"
  Data.SerialNumber = (char *)malloc(256);		// "FT000001" if fixed, or NULL
  ftStatus = FT_EE_Read(ftHandle0, &Data);
  if(ftStatus != FT_OK) {
    printf("FT_EE_Read failed\n");
    FT_Close(ftHandle0);
    return 1;
  }

  if( ser_dbg ) {
    printf("Cbus0 = 0x%X\n", Data.Cbus0);				// Cbus Mux control
    printf("Cbus1 = 0x%X\n", Data.Cbus1);				// Cbus Mux control
    printf("Cbus2 = 0x%X\n", Data.Cbus2);				// Cbus Mux control
    printf("Cbus3 = 0x%X\n", Data.Cbus3);				// Cbus Mux control
    printf("Cbus4 = 0x%X\n", Data.Cbus4);				// Cbus Mux control
  }

  // check that cbus0 and 2 are write
  if( Data.Cbus0 != 0x0A ) {
    printf( "Cbus0 is %d, should be %d, updating!\n", Data.Cbus0, 0xA );
    Data.Cbus0 = 0x0A;
    need_write = 1;
  }
  
  if( Data.Cbus2 != 0x0A ) {
    printf( "Cbus2 is %d, should be %d, updating!\n", Data.Cbus2, 0xA );
    Data.Cbus2 = 0x0A;
    need_write = 1;
  }

  // check that CBUS3 is power enable
  if( Data.Cbus3 != 0x0A ) {  // was 01
    printf( "Cbus3 is %d, should be %d, updating!\n", Data.Cbus3, 0x1);
    //Data.Cbus2 = 0x0B;  // wierd, should be writing to cbus3, has no effect because current verion is alwasy default (0x01)

    Data.Cbus3 = 0x0A;

    need_write = 1;
  }

  // not necessary, but for the hell of it, cbus 1 is read
  if( Data.Cbus1 != 0x0A ) {
    printf( "Cbus1 is %d, should be %d, updating!\n", Data.Cbus1, 0xA );
    Data.Cbus1 = 0x0A;
    need_write = 1;
  }




if( need_write ) {
    printf( "Updating EEPROM to correct setting for safecast.\n" );
    ftStatus = FT_EE_Program(ftHandle0, &Data);
    if(ftStatus != FT_OK) {
      printf("FT_EE_Program failed (%d)\n", ftStatus);
      FT_Close(ftHandle0);
      return 1;
    }
    printf( "------> Now that the EEPROM is updated, unplug and replug the device.\n" );
  } else {
    printf( "EEPROM values are up to date, not modifying them\n" );
  }

   printf( "Updating CBUS3 for charging.\n" );  


  ftStatus = FT_SetBitMode(ftHandle0, 0x80, 0x20); // CBUS bitbang mode
  if(ftStatus != FT_OK) {
    printf("Failed to set CBUS\n");
  }else {
   
      printf("Set CBUS3 to LOW\n" );
  }




  return 0;

}

int openSerialPorts(int baud) {
  char * 	pcBufRead = NULL;
  char * 	pcBufLD[MAX_DEVICES + 1];
  DWORD	dwRxSize = 0;
  DWORD 	dwBytesWritten, dwBytesRead;
  FT_STATUS	ftStatus;
  int	iNumDevs = 0;
  int	i, j;
  int	iDevicesOpen;	
  unsigned char ucMode = 0x00;

  printf( "warning: opening up to %d ports and assuming all are Safecast devices.\n", MAX_DEVICES );
  printf( "todo: make this more selective and safer.\n" );

  for(i = 0; i < MAX_DEVICES; i++) {
    pcBufLD[i] = cBufLD[i];
  }
  pcBufLD[MAX_DEVICES] = NULL;
  
  ftStatus = FT_ListDevices(pcBufLD, &iNumDevs, FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER);
  
  if(ftStatus != FT_OK) {
    printf("Error: FT_ListDevices(%d)\n", ftStatus);
    return -1;
  }
  
  for(i = 0; ( (i <MAX_DEVICES) && (i < iNumDevs) ); i++) {
    printf("Device %d Serial Number - %s\n", i, cBufLD[i]);
  }
  
  for(i = 0; ( (i <MAX_DEVICES) && (i < iNumDevs) ) ; i++) {
    /* Setup */
    if((ftStatus = FT_OpenEx(cBufLD[i], FT_OPEN_BY_SERIAL_NUMBER, &ftHandle[i])) != FT_OK){
      /* 
	 This can fail if the ftdi_sio driver is loaded
	 use lsmod to check this and rmmod ftdi_sio to remove
	 also rmmod usbserial
      */
      printf("Error FT_OpenEx(%d), device\n", ftStatus);
      return -1;
    }
    
    printf("Opened device %s\n", cBufLD[i]);
    
//    if(getandcheckCBUS(ftHandle[i]) ) {
 //     printf( "getandcheckCBUS failed, exiting.\n" );
 //     return -1;
 //   }
    
    iDevicesOpen++;
    if((ftStatus = FT_SetBaudRate(ftHandle[i], baud)) != FT_OK) {
      printf("Error FT_SetBaudRate(%d), cBufLD[i] = %s\n", ftStatus, cBufLD[i]);
      break;
    }

    if((ftStatus = FT_SetDataCharacteristics(ftHandle[i], FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_EVEN)) != FT_OK) {
      printf("Error FT_SetDataCharacteristics(%d) = %s\n", ftStatus, cBufLD[i]);
      break;
    }
    
    FT_SetTimeouts(ftHandle[i], 500, 500);	// 500 ms read/write timeout

  }
  
  iDevicesOpen = i;
  foundDevices = i; // record this in a global

  if(pcBufRead)
    free(pcBufRead);

  return 0; // we always use the 0th device for now
}

// mode = 1 goes to bootloader, mode = 0 goes to normal
void safecast_resetboard(int mode) {
  unsigned char mask = 0;
  unsigned char ucMode;
  FT_STATUS	ftStatus;

  if( mode == 1 ) {
    mask = CBUS_BOOT_MASK;  // 0x4
  } else {
    mask = 0;
  }
  printf( "Resetting MCU and forcing " );
  if( mode == 1 ) {
    printf( "System Mode." );
  } else {
    printf( "normal mode." );
  }
  fflush(stdout);

  ucMode = 0xF0 | mask; // set to system memory mode
  ftStatus = FT_SetBitMode(ftHandle[stm32_ser_id], ucMode, 0x20); // CBUS bitbang mode
  if(ftStatus != FT_OK) {
    printf("Failed to set Bit Mode\n");
  } else {
    if( ser_dbg )
      printf("Set bitbang mode to %02x\n", ucMode );
  }
  usleep(500000);
  printf( "." );
  fflush(stdout);
  
  ucMode = 0xF0 | mask | CBUS_RESET_MASK; // release reset
  ftStatus = FT_SetBitMode(ftHandle[stm32_ser_id], ucMode, 0x20); // CBUS bitbang mode
  if(ftStatus != FT_OK) {
    printf("Failed to set Bit Mode\n");
  } else {
    if( ser_dbg )
      printf("Set bitbang mode to %02x\n", ucMode );
  }
  usleep(500000);
  if( mode ) 
    printf( ".should now be in system mode.\n" );
  else




    printf( ".should now be running as normal.\n" );
  
}

int closeSerialPorts() {
  int i;
  FT_STATUS	ftStatus;

  /* Cleanup */
  for(i = 0; i < foundDevices; i++) {
    if((ftStatus = FT_SetDataCharacteristics(ftHandle[i], FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE)) != FT_OK) {
      printf("Error FT_SetDataCharacteristics(%d) = %s\n", ftStatus, cBufLD[i]);
      break;
    }
    FT_Close(ftHandle[i]);
    printf("Closed device %s\n", cBufLD[i]);
  }
  return 0;
}

void printhelp(char *name, char *note) {
  printf( "Usage: %s \n", name );
}

int fw_abort() {
  closeSerialPorts();
  return 0;
}

// Get data function
static u32 writeh_read_data_page1( u8 *dst, u32 len )
{
  size_t readbytes = 0;

  if( !feof( fp_page1 ) )
    readbytes = fread( dst, 1, len, fp_page1 );
  return ( u32 )readbytes;
}

// Get data function
static u32 writeh_read_data_page4( u8 *dst, u32 len )
{
  size_t readbytes = 0;

  if( !feof( fp_page4 ) )
    readbytes = fread( dst, 1, len, fp_page4 );
  return ( u32 )readbytes;
}

// Progress function
static void writeh_progress( u32 wrote )
{
  unsigned pwrite = ( wrote * 100 ) / fpsize;
  static int expected_next = 10;

  if( pwrite >= expected_next )
  {
    printf( "%d%% ", expected_next );
    expected_next += 10;
  }
}

int main(int argc, char **argv)
{
  int aflag = 0;
  char *argval = NULL;
  char infile_name[256];
  int index;
  int c;
  int baud = 115200;
  u8 minor, major;
  u16 version;
  int badness = 0;
  int readflag = 0;
  int readoffset = 0;

  opterr = 0;
  infile_name[0] = '\0';
  ser_dbg = 0;

  while ((c = getopt(argc, argv, "b:f:dr:")) != -1 ) {
    switch (c) {
      
    case 'a': 
      aflag = 1;
      break;
      
    case 'b':
      argval = optarg;
      baud = strtol(argval, NULL, 0);
      if( baud < 1200 || baud > 115200 ) {
	printf( "Baud should be between 1200 and 115200; got: %d\n", baud );
	return 0;
      }
      break;
      
    case 'f':
      strncpy(infile_name, optarg, 256);
      break;
      
    case 'd':
      ser_dbg = 1;
      break;
      
    case 'r':
      argval = optarg;
      readoffset = strtol(argval, NULL, 0);
      readflag = 1;
      break;
      
    case '?':
      printhelp(argv[0], NULL);
      break;

    default:
      printhelp(argv[0], NULL);
      fw_abort();
    }
  }

  if( !readflag ) {
    fp_page1 = fopen( infile_name, "rb" );
    fp_page4 = fopen( infile_name, "rb" );

    if((fp_page1 == NULL) || (fp_page4 == NULL)) {
      fprintf( stderr, "Unable to open %s\n", infile_name );
      exit( 1 );
    }  else    {
      fseek( fp_page4, 0, SEEK_END );
      fpsize = ftell( fp_page4 );
      fseek( fp_page4, 0, SEEK_SET );
    }
    fseek( fp_page4, 6144, SEEK_SET);
  }

  // Connect to bootloader
 
  printf( "Connect to bootloader.\n" );
  if( stm32_init( NULL, baud ) != STM32_OK )
  {
    fprintf( stderr, "Unable to connect to bootloader\n" );
    exit( 1 );
  }
  printf( "\n" );
  
  // Get version
  printf( "Get version.\n" );
  if( stm32_get_version( &major, &minor ) != STM32_OK )
  {
    fprintf( stderr, "Unable to get bootloader version\n" );
    //    exit( 1 );
    badness = 1;
  }
  else
  {
    printf( "Found bootloader version: %d.%d\n", major, minor );
    if( BL_MKVER( major, minor ) < BL_MINVERSION )
    {
      fprintf( stderr, "Unsupported bootloader version" );
      exit( 1 );
    }
  }
  
  // Get chip ID
  printf( "Get chip ID.\n" );
  if( stm32_get_chip_id( &version ) != STM32_OK )
  {
    fprintf( stderr, "Unable to get chip ID\n" );
    badness = 1;
    //    exit( 1 );
  }
  else
  {
    printf( "Chip ID: %04X\n", version );
    if( version != CHIP_ID )
    {
      fprintf( stderr, "Unsupported chip ID" );
      exit( 1 );
    }
  }
  if( badness )
    exit( 1 );
  
  if( !readflag ) {
    // Write unprotect
    printf( "Write unprotect.\n" );
    if( stm32_write_unprotect() != STM32_OK )
      {
	fprintf( stderr, "Unable to execute write unprotect\n" );
	exit( 1 );
      }
    else
      printf( "Cleared write protection.\n" );

    // Read unprotect
    printf( "Read unprotect.\n" );
    if( stm32_read_unprotect() != STM32_OK )
      {
	fprintf( stderr, "Unable to execute read unprotect\n" );
	exit( 1 );
      }
    else
      printf( "Cleared read protection.\n" );

    // Erase flash
    printf( "Erase flash.\n" );
    int res1 = stm32_erase_flash_page(0,1);     // first page
    int res2 = stm32_erase_flash_page(3,0xFD);  // all the rest
    if(res1 != STM32_OK) {
	    fprintf( stderr, "Unable to erase chip - pre prvkey\n" );
	    exit(1);
    }
    if(res2 != STM32_OK) {
	    fprintf( stderr, "Unable to erase chip - post pk\n" );
	    exit(1);
    }

    printf( "Erased FLASH memory.\n" );

    // Program flash
    setbuf( stdout, NULL );
    printf( "Programming flash ... ");
    if( stm32_write_flash_page(0x08000000,1,writeh_read_data_page1, writeh_progress ) != STM32_OK ) {
      fprintf( stderr, "Unable to program - initial page.\n" );
	    exit(1);
    } else {
      printf( "\nProgram pre-pk Done.\n" );
    }

    if( stm32_write_flash_page(0x08001800,0xFC,writeh_read_data_page4, writeh_progress ) != STM32_OK ) {
      fprintf( stderr, "Unable to program - post pk flash.\n" );
	    exit(1);
    } else {
      printf( "\nProgram post-pk Done.\n" );
    }


  } else {
    printf( "Readback flash memory at offset %x\n", readoffset );
    if(stm32_read_flash( readoffset, 10240 ) != STM32_OK ) {
      fprintf( stderr, "Unable to read FLASH memory.\n" );
      exit( 1 );
    } else {
      printf( "\nDone.\n" );
    }
  }

  // reset back into normal mode
  printf( "\n" );
  safecast_resetboard(0);

  if( !readflag ) {
    fclose( fp_page1 );
    fclose( fp_page4 );
  }

  closeSerialPorts();

  return 0;
}

