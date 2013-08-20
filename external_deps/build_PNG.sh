#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build PNG, requires zlib
cd libpng-1.6.3
make distclean
export CPPFLAGS="-I$PWD/../include"
export LDFLAGS="-L$PWD/../win32"
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp png.h pngconf.h pnglibconf.h ../include
cp .libs/libpng16.a ../win32/libpng.a
make distclean
export LDFLAGS="-L$PWD/../win64"
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp .libs/libpng16.a ../win64/libpng.a
