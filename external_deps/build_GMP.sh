#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build GMP
cd gmp-5.1.2
make distclean
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp gmp.h gmpxx.h ../include
cp .libs/libgmp.a ../win32
make distclean
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp .libs/libgmp.a ../win64
