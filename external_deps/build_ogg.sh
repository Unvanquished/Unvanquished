#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build ogg
cd libogg-1.3.1
make distclean
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
mkdir -p ../include/ogg
cp include/ogg/*.h ../include/ogg
cp src/.libs/libogg.a ../win32
make distclean
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp src/.libs/libogg.a ../win64
