#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build GLEW
cd glew-1.10.0
make distclean
make -j8 SYSTEM=mingw CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar RANLIB=i686-w64-mingw32-ranlib STRIP=i686-w64-mingw32-strip LD=i686-w64-mingw32-gcc
cp -r include/GL ../include
cp lib/{libglew32.a,libglew32mx.a} ../win32
make distclean
make -j8 SYSTEM=mingw CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar RANLIB=x86_64-w64-mingw32-ranlib STRIP=x86_64-w64-mingw32-strip LD=x86_64-w64-mingw32-gcc
cp lib/{libglew32.a,libglew32mx.a} ../win64
