#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build theora, requires ogg and vorbis
cd libtheora-1.1.1
make distclean
export CPPFLAGS="-I$PWD/../include"
export LDFLAGS="-L$PWD/../win32"
./configure --host=i686-w64-mingw32 --enable-static --disable-shared --disable-examples --disable-encode
make -j8
mkdir -p ../include/theora
cp include/theora/*.h ../include/theora
cp lib/.libs/{libtheora.a,libtheorafile.a,libtheoraenc.a} ../win32
make distclean
export LDFLAGS="-L$PWD/../win64"
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared --disable-examples --disable-encode
make -j8
cp lib/.libs/{libtheora.a,libtheorafile.a,libtheoraenc.a} ../win64
