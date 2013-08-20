#!/bin/sh
# Create result directories
mkdir -p include
mkdir -p win32
mkdir -p win64

# Build vorbis, requires ogg
cd libvorbis-1.3.3
make distclean
export CPPFLAGS="-I$PWD/../include"
export LDFLAGS="-L$PWD/../win32"
./configure --host=i686-w64-mingw32 --enable-static --disable-shared --disable-examples
make -j8
mkdir -p ../include/vorbis
cp include/vorbis/*.h ../include/vorbis
cp lib/.libs/{libvorbis.a,libvorbisfile.a,libvorbisenc.a} ../win32
make distclean
export LDFLAGS="-L$PWD/../win64"
./configure --host=x86_64-w64-mingw32 --enable-static --disable-shared --disable-examples
make -j8
cp lib/.libs/{libvorbis.a,libvorbisfile.a,libvorbisenc.a} ../win64
