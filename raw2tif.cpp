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

#ifdef _MSC_VER // this one change enabled building on an M1 Mac
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
    printf( "          A minimal Adobe .xmp file is created for the output image with Rating=1\n" );
    printf( "          On Windows, tz.exe is invoked to attempt to enable ZIP compression on the TIFF file\n" );
    exit( 0 );
} //Usage

const char * pcRating1XMP =
    R"(<x:xmpmeta xmlns:x="adobe:ns:meta/" x:xmptk="Adobe XMP Core 7.0-c000 1.000000, 0000/00/00-00:00:00        ">)" "\n"
    R"( <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">)"                                           "\n"
    R"(  <rdf:Description rdf:about="")"                                                                              "\n"
    R"(    xmlns:xmp="http://ns.adobe.com/xap/1.0/")"                                                                 "\n"
    R"(   xmp:Rating="1")"                                                                                            "\n"
    R"(  </rdf:Description>)"                                                                                         "\n"
    R"( </rdf:RDF>)"                                                                                                  "\n"
    R"(</x:xmpmeta>)"                                                                                                 "\n"
    ;

int main( int ac, char * av[] )
{
    if ( 2 != ac )
      Usage();
  
    putenv( (char *) "TZ=UTC" ); // dcraw compatibility, affects TIFF datestamp field

    unique_ptr<LibRaw> RawProcessor( new LibRaw );
    RawProcessor->imgdata.params.output_tiff = 1;
    RawProcessor->imgdata.params.output_bps = 16;
  
    static char infn[ 1024 ];
    strcpy( infn, av[1] );
    printf( "Processing file %s\n", infn );
  
    int ret = RawProcessor->open_file( infn );
    if ( LIBRAW_SUCCESS != ret )
    {
        printf( "error %#x == %d; can't open input file %s: %s\n", ret, ret, infn, libraw_strerror( ret ) );
        Usage();
    }
  
    ret = RawProcessor->unpack();
    if ( LIBRAW_SUCCESS != ret )
    {
        printf( "error %#x == %d; can't unpack %s: %s\n", ret, ret, infn, libraw_strerror( ret ) );
        Usage();
    }
  
    ret = RawProcessor->dcraw_process();
  
    if ( LIBRAW_SUCCESS != ret )
    {
        printf( "error %#x == %d, can't do pocessing on %s: %s\n", ret, ret, infn, libraw_strerror( ret ) );
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
  
    static char outfn[ 1024 ];
    strcpy( outfn, infn );
    char * dot = strrchr( outfn, '.' );
    if ( 0 == dot )
    {
        printf( "input filename doesn't have a file extension, which is required\n" );
        Usage();
    }

    strcpy( dot, "-lr.tiff" );
    printf( "Writing file %s\n", outfn );
  
    ret = RawProcessor->dcraw_ppm_tiff_writer( outfn );
    if ( LIBRAW_SUCCESS != ret )
    {
        printf( "error %#x == %d; can't write %s: %s\n", ret, ret, outfn, libraw_strerror( ret ) );
        Usage();
    }

    RawProcessor->recycle_datastream();
    RawProcessor->recycle();
    RawProcessor.reset( 0 );

    // Create a .xmp file with a rating of 1 so it's easier to find the image in Lightroom

    static char xmpfn[ 1024 ];
    strcpy( xmpfn, outfn );
    dot = strrchr( xmpfn, '.' );
    strcpy( dot, ".xmp" );
    printf( "Writing xmp sidecar file %s\n", xmpfn );

    FILE * fp = fopen( xmpfn, "w" );
    if ( fp )
    {
        fprintf( fp, "%s", pcRating1XMP );
        fclose( fp );
    }
    else
        printf( "can't open the xmp file for writing\n" );

#ifdef _MSC_VER // if on Windows, try to compress the tiff file

    printf( "Attempting to compress the tiff file\n" );

    STARTUPINFO si;
    ZeroMemory( &si, sizeof si );
    si.cb = sizeof si;

    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof pi );

    static char acProcess[ 1024 ];
    sprintf( acProcess, "tz \"%s\"", outfn );

    do
    {
        HANDLE hTmp = CreateFileA( outfn, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );

        if ( INVALID_HANDLE_VALUE == hTmp )
        {
            // Wait for Defender / Indexer / etc. to let go of the file. This is silly, but happens quite often.

            Sleep( 100 );
            printf( "." );
        }
        else
        {
            CloseHandle( hTmp );
            break;
        }
    } while( true );

    BOOL ok = CreateProcessA( NULL, acProcess, 0, 0, FALSE, 0, 0, 0, &si, &pi );
    if ( ok )
    {
        WaitForSingleObject( pi.hProcess, INFINITE );
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
        //printf( "Successfully invoked tz to compress the tiff file\n" );
    }

    printf( "Raw2Tif is complete\n" );

#endif

    return 0;
} //main
