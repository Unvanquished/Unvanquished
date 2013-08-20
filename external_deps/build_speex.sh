#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build speex, requires ogg
cd speex-1.2rc1
make distclean
export CPPFLAGS="-I$PWD/../include"
export LDFLAGS="-L$PWD/../win32"
./configure --host=i686-w64-mingw32 --enable-static --disable-shared
make -j8
mkdir -p ../include/speex
cp include/speex/*.h ../include/speex
cp libspeex/.libs/{libspeex.a,libspeexdsp.a} ../win32
make distclean
export LDFLAGS="-L$PWD/../win64"
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared
make -j8
cp libspeex/.libs/{libspeex.a,libspeexdsp.a} ../win64
