#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build SDL
cd SDL-1.2.15
make distclean
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp include/*.h ../include
cp build/.libs/libSDL{main,}.a ../win32
make distclean
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp build/.libs/libSDL{main,}.a ../win64
