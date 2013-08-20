#!/bin/sh
curl -L http://www.libsdl.org/release/SDL-1.2.15.tar.gz | tar xz
curl -L http://downloads.sourceforge.net/project/glew/glew/1.10.0/glew-1.10.0.tgz | tar xz
curl -L http://download.savannah.gnu.org/releases/freetype/freetype-2.5.0.tar.bz2 | tar xj
curl -L http://zlib.net/zlib-1.2.8.tar.xz | tar xJ
curl -L http://download.sourceforge.net/libpng/libpng-1.6.3.tar.xz | tar xJ
curl -L http://www.ijg.org/files/jpegsrc.v9.tar.gz | tar xz
curl -L https://webp.googlecode.com/files/libwebp-0.3.1.tar.gz | tar xz
curl -L http://www.lysator.liu.se/\~nisse/archive/nettle-2.7.1.tar.gz | tar xz
curl -L ftp://ftp.gmplib.org/pub/gmp/gmp-5.1.2.tar.xz | tar xJ
curl -L http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.bz2 | tar xj
curl -L http://downloads.xiph.org/releases/ogg/libogg-1.3.1.tar.xz | tar xJ
curl -L http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.xz | tar xJ
curl -L http://downloads.xiph.org/releases/speex/speex-1.2rc1.tar.gz | tar xz
curl -L http://curl.haxx.se/download/curl-7.32.0.tar.bz2 | tar xj
curl -L http://kcat.strangesoft.net/openal-soft-1.15.1-bin.zip -o openal-soft-1.15.1-bin.zip
unzip openal-soft-1.15.1-bin.zip
rm openal-soft-1.15.1-bin.zip
