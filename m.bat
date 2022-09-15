@echo off
del raw2tif.exe
del raw2tif.pdb

REM the libraw folder has the bare minimum .h and .lib files needed to target libraw.dll

cl /nologo raw2tif.cpp /O2i /EHac /Zi /Gy /D_AMD64_ /link /OPT:REF


