#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build WebP
cd libwebp-0.3.1
make distclean
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp -r src/webp ../include
cp src/.libs/libwebp.a ../win32
make distclean
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp src/.libs/libwebp.a ../win64
