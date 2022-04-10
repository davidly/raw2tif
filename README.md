# raw2tif
command-line app that converts images from RAW to 16-bit TIFF files using LibRaw. Build for MacOS and Windows.

Build on Windows using a Visual Studio vcvars64.bat command window like this:

    cl /nologo raw2tif.cpp /O2i /EHac /Zi /Gy /D_AMD64_ /link /OPT:REF
    
Built and tested on MacOS as well.

Requires LibRaw to be cloned and built. https://github.com/LibRaw/LibRaw

    Raw to Tiff conversion, using open-source LibRaw
    Usage: raw2tif inputfile
      e.g.:   raw2tif p1010365.rw2       This produces the file p1010365-lr.tiff
      notes:  16-bit per channel uncompressed TIFF files are produced
              Defaults for all RAW conversion options are used
              The output filename is the input name without an extension and "-lr.tiff" appended
