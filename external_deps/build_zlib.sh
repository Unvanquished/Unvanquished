#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build zlib
cd zlib-1.2.8
make distclean
make -j8 -f win32/Makefile.gcc PREFIX=i686-w64-mingw32-
cp zlib.h zconf.h ../include
cp libz.a ../win32
make distclean
make -j8 -f win32/Makefile.gcc PREFIX=x86_64-w64-mingw32-
cp libz.a ../win64
