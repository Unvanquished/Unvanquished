#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build FreeType, depends on PNG
cd freetype-2.5.0
make distclean
export CPPFLAGS="-I$PWD/../include"
export LDFLAGS="-L$PWD/../win32"
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
cp -r include/* ../include
cp objs/.libs/libfreetype.a ../win32
make distclean
export LDFLAGS="-L$PWD/../win64"
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp objs/.libs/libfreetype.a ../win64
