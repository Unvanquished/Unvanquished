#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build JPEG
cd jpeg-9
make distclean
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp jerror.h jconfig.h jmorecfg.h jpeglib.h ../include
cp .libs/libjpeg.a ../win32
make distclean
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp .libs/libjpeg.a ../win64
