// Loader driver

#include "stm32ld.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static FILE *fp;
static u32 fpsize;

#define BL_VERSION_MAJOR  2
#define BL_VERSION_MINOR  1
#define BL_MKVER( major, minor )    ( ( major ) * 256 + ( minor ) ) 
#define BL_MINVERSION               BL_MKVER( BL_VERSION_MAJOR, BL_VERSION_MINOR )

#define CHIP_ID           0x0414
#define CHIP_ID_ALT           0x0413

// ****************************************************************************
// Helper functions and macros

// Get data function
static u32 writeh_read_data( u8 *dst, u32 len )
{
  size_t readbytes = 0;

  if( !feof( fp ) )
    readbytes = fread( dst, 1, len, fp );
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

// ****************************************************************************
// Entry point

int main( int argc, const char **argv )
{
  u8 not_flashing=0;
  u8 send_go_command=0;
  u8 minor, major;
  u16 version;
  u32 data_flash;
  long baud;
 
  // Argument validation
  if( argc < 4 )
  {
    fprintf( stderr, "Usage: stm32ld <port> <baud> \n" );
    exit( 1 );
  }
  errno = 0;
  baud = strtol( argv[ 2 ], NULL, 10 );
  if( ( errno == ERANGE && ( baud == LONG_MAX || baud == LONG_MIN ) ) || ( errno != 0 && baud == 0 ) || ( baud < 0 ) ) 
  {
    fprintf( stderr, "Invalid baud '%s'\n", argv[ 2 ] );
    exit( 1 );
  }
  
  // Connect to bootloader
  if( stm32_init( argv[ 1 ], baud ) != STM32_OK )
  {
    fprintf( stderr, "Unable to connect to bootloader\n" );
    exit( 1 );
  }
  
  // Get version
  if( stm32_get_version( &major, &minor ) != STM32_OK )
  {
    fprintf( stderr, "Unable to get bootloader version\n" );
    exit( 1 );
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
    if( stm32_get_chip_id( &version ) != STM32_OK )
    {
      fprintf( stderr, "Unable to get chip ID\n" );
      exit( 1 );
    }
    else
    {
      printf( "Chip ID: %04X\n", version );
      if( version != CHIP_ID && version != CHIP_ID_ALT )
      {
        fprintf( stderr, "Unsupported chip ID" );
        exit( 1 );
      }
    }
  
  if( not_flashing == 0 )
  {
    // Write unprotect
    if( stm32_write_unprotect() != STM32_OK )
    {
      fprintf( stderr, "Unable to execute write unprotect\n" );
      exit( 1 );
    }
    else
      printf( "Cleared write protection.\n" );

    // Erase flash
    if( major == 3 )
    {
      printf( "Starting Extended Erase of FLASH memory. This will take some time ... Please be patient ...\n" );
      if( stm32_extended_erase_flash() != STM32_OK )
      {
        fprintf( stderr, "Unable to extended erase chip\n" );
        exit( 1 );
      }
      else
        printf( "Extended Erased FLASH memory.\n" );
    }
    else
    {
      if( stm32_erase_flash() != STM32_OK )
      {
        fprintf( stderr, "Unable to erase chip\n" );
        exit( 1 );
      }
      else
        printf( "Erased FLASH memory.\n" );
    }

    // Program flash
    setbuf( stdout, NULL );
    printf( "Programming flash ... ");
    if( poke(0x80000000,0x12345678) != STM32_OK )
    {
      fprintf( stderr, "Unable to program FLASH memory.\n" );
      exit( 1 );
    }
    else{
      printf( "\nDone.\n" );
      data_flash=peek(0x80000000);
      printf("%lu\n", data_flash);
    }

    fclose( fp );
  }
  else
    printf( "Skipping flashing ... \n" );

  return 0;
}
           
