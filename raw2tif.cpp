#define RAW2TIF_STATIC_LINK

#ifdef RAW2TIF_STATIC_LINK

    #define LIBRAW_WIN32_DLLDEFS
    #define LIBRAW_NODLL

    #pragma comment( lib, "libraw_static.lib" )

#else

    #pragma comment( lib, "libraw.lib" )

#endif

// Need this so it builds

#define LIBRAW_NO_WINSOCK2

#ifdef _MSC_VER // this one change enabled buildin on an M1 Mac
#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory>

#include "libraw/libraw.h"

#ifndef LIBRAW_WIN32_CALLS
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

using namespace std;

void Usage()
{
    printf( "Raw to Tiff conversion, using open-source LibRaw\n" );
    printf( "Usage: raw2tif inputfile\n" );
    printf( "  e.g.:   raw2tif p1010365.rw2       This produces the file p1010365-lr.tiff\n" );
    printf( "  notes:  16-bit per channel uncompressed TIFF files are produced\n" );
    printf( "          Defaults for all RAW conversion options are used\n" );
    printf( "          The output filename is the input name without an extension and \"-lr.tiff\" appended\n" );
    exit( 0 );
} //Usage

int main( int ac, char * av[] )
{
    unique_ptr<LibRaw> RawProcessor( new LibRaw );
  
    if (ac != 2)
      Usage();
  
    putenv( (char *) "TZ=UTC" ); // dcraw compatibility, affects TIFF datestamp field
  
    RawProcessor->imgdata.params.output_tiff = 1;
    RawProcessor->imgdata.params.output_bps = 16;
  
    char infn[ 1024 ];
    strcpy( infn, av[1] );
  
    printf( "Processing file %s\n", infn );
  
    int ret;
  
    if ( ( ret = RawProcessor->open_file( infn ) ) != LIBRAW_SUCCESS )
    {
        printf( "can't open input file %s\n", infn );
        Usage();
    }
  
    if ( ( ret = RawProcessor->unpack() ) != LIBRAW_SUCCESS )
    {
        printf( "Cannot unpack %s: %s\n", infn, libraw_strerror( ret ) );
        Usage();
    }
  
    ret = RawProcessor->dcraw_process();
  
    if (LIBRAW_SUCCESS != ret)
    {
        printf( "Cannot do postpocessing on %s: %s\n", infn, libraw_strerror( ret ) );
        Usage();
    }

#if false

    // for fun get the image in-memory

    int width, height, colors, bps;
    RawProcessor->get_mem_image_format( &width, &height, &colors, &bps );
    printf("in-memory format: width %d, height %d, colors %d, bps %d\n", width, height, colors, bps );

    int stride = width * colors * bps / 8;
    unique_ptr<byte> data( new byte[ height * stride + 1 ] );
    memset( data.get(), 0x37, height * stride + 1 );
    ret = RawProcessor->copy_mem_image( data.get(), stride, true );
    printf( "ret from copy_mem_image: %d\n", ret );

    byte * pastend = data.get() + ( height * stride );
    printf( "last real %#x, first fake %#x\n", *( pastend - 1 ), *pastend );

#endif
  
    char outfn[ 1024 ];
    strcpy( outfn, infn );
    char * dot = strrchr( outfn, '.' );
    strcpy( dot, "-lr.tiff" );
  
    printf( "Writing file %s\n", outfn );
  
    if ( LIBRAW_SUCCESS != ( ret = RawProcessor->dcraw_ppm_tiff_writer( outfn ) ) )
    {
        printf( "Cannot write %s: %s\n", outfn, libraw_strerror( ret ) );
        Usage();
    }
  
    RawProcessor->recycle();
  
    return 0;
} //main
