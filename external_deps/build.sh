#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Package versions
GMP_VERSION=5.1.3
NETTLE_VERSION=2.7.1
ZLIB_VERSION=1.2.8
GEOIP_VERSION=1.6.0
SDL2_VERSION=2.0.1
CURL_VERSION=7.33.0
GLEW_VERSION=1.10.0
PNG_VERSION=1.6.6
JPEG_VERSION=1.3.0
WEBP_VERSION=0.3.1
FREETYPE_VERSION=2.5.0.1
OPENAL_VERSION=1.15.1
OGG_VERSION=1.3.1
VORBIS_VERSION=1.3.3
SPEEX_VERSION=1.2rc1
THEORA_VERSION=1.1.1
OPUS_VERSION=1.0.3
OPUSFILE_VERSION=0.4

# Download and extract a file
# Usage: download <filename> <URL>
download() {
	if [ ! -f "${1}" ]; then
		curl -L -o "${1}" "${2}"
		case "${1##*\.}" in
		xz)
			tar xJf "${1}"
			;;
		bz2)
			tar xjf "${1}"
			;;
		gz|tgz)
			tar xzf "${1}"
			;;
		zip)
			unzip "${1}"
			;;
		*)
			echo "Unknown archive type for ${1}"
			exit 1
			;;
		esac
	fi
}

# Build GMP
build_gmp() {
	download gmp-${GMP_VERSION}.tar.xz ftp://ftp.gmplib.org/pub/gmp/gmp-${GMP_VERSION}.tar.xz
	cd gmp-${GMP_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build Nettle
build_nettle() {
	download nettle-${NETTLE_VERSION}.tar.gz http://www.lysator.liu.se/\~nisse/archive/nettle-${NETTLE_VERSION}.tar.gz
	cd nettle-${NETTLE_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build zlib
build_zlib() {
	download zlib-${ZLIB_VERSION}.tar.xz http://zlib.net/zlib-${ZLIB_VERSION}.tar.xz
	cd zlib-${ZLIB_VERSION}
	make -f win32/Makefile.gcc clean
	make -f win32/Makefile.gcc PREFIX="${HOST}-"
	make -f win32/Makefile.gcc install BINARY_PATH="${PREFIX}/bin" LIBRARY_PATH="${PREFIX}/lib" INCLUDE_PATH="${PREFIX}/include" SHARED_MODE=1
	cd ..
}

# Build GeoIP
build_geoip() {
	download GeoIP-${GEOIP_VERSION}.tar.gz http://www.maxmind.com/download/geoip/api/c/GeoIP-${GEOIP_VERSION}.tar.gz
	cd GeoIP-${GEOIP_VERSION}
	export ac_cv_func_malloc_0_nonnull=yes
	export ac_cv_func_realloc_0_nonnull=yes
	make distclean || true
	local OLD_LDFLAGS="${LDFLAGS}"
	case "${PLATFORM}" in
	mingw32|mingw64)
		export LDFLAGS="${LDFLAGS} -lws2_32"
		;;
	esac
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	export LDFLAGS="${OLD_LDFLAGS}"
	cd ..
}

# Build SDL2
build_sdl2() {
	case "${PLATFORM}" in
	mingw32|mingw64)
		# Use precompiled binaries to avoid depending on the DirectX SDK
		download SDL2-devel-${SDL2_VERSION}-mingw.tar.gz http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz
		cd SDL2-${SDL2_VERSION}
		make install-package arch="${HOST}" prefix="${PREFIX}"
		cd ..
		;;
	*)
		echo "Unsupported platform for SDL2"
		exit 1
		;;
	esac
}

# Build cURL
build_curl() {
	download curl-${CURL_VERSION}.tar.bz2 http://curl.haxx.se/download/curl-${CURL_VERSION}.tar.bz2
	cd curl-${CURL_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --disable-static --enable-shared
	make
	make install
	cd ..
}

# Build GLEW
build_glew() {
	download glew-${GLEW_VERSION}.tgz http://downloads.sourceforge.net/project/glew/glew/${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz
	cd glew-${GLEW_VERSION}
	make distclean || true
	case "${PLATFORM}" in
	mingw32|mingw64)
		SYSTEM=mingw
		;;
	*)
		echo "Unsupported platform for GLEW"
		exit 1
		;;
	esac
	make SYSTEM="${SYSTEM}" CC="${HOST}-gcc" AR="${HOST}-ar" RANLIB="${HOST}-ranlib" STRIP="${HOST}-strip" LD="${HOST}-gcc"
	make install GLEW_DEST="${PREFIX}" SYSTEM="${SYSTEM}" CC="${HOST}-gcc" AR="${HOST}-ar" RANLIB="${HOST}-ranlib" STRIP="${HOST}-strip" LD="${HOST}-gcc"
	cd ..
}

# Build PNG
build_png() {
	download libpng-${PNG_VERSION}.tar.xz http://download.sourceforge.net/libpng/libpng-${PNG_VERSION}.tar.xz
	cd libpng-${PNG_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build JPEG
build_jpeg() {
	download libjpeg-turbo-${JPEG_VERSION}.tar.gz http://downloads.sourceforge.net/project/libjpeg-turbo/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz
	cd libjpeg-turbo-${JPEG_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared --with-jpeg8
	make
	make install
	cd ..
}

# Build WebP
build_webp() {
	download libwebp-${WEBP_VERSION}.tar.gz https://webp.googlecode.com/files/libwebp-${WEBP_VERSION}.tar.gz
	cd libwebp-${WEBP_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build FreeType
build_freetype() {
	download freetype-${FREETYPE_VERSION}.tar.bz2 http://download.savannah.gnu.org/releases/freetype/freetype-${FREETYPE_VERSION}.tar.bz2
	cd freetype-${FREETYPE_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build OpenAL
build_openal() {
	case "${PLATFORM}" in
	mingw32|mingw64)
		# Use precompiled binaries to link to OpenAL32.dll
		download openal-soft-${OPENAL_VERSION}-bin.zip http://kcat.strangesoft.net/openal-soft-${OPENAL_VERSION}-bin.zip
		cd openal-soft-${OPENAL_VERSION}-bin
		cp -r include/AL ${PREFIX}/include
		case "${PLATFORM}" in
		mingw32)
			cp lib/Win32/libOpenAL32.dll.a ${PREFIX}/lib
			cp Win32/soft_oal.dll ${PREFIX}/bin
			;;
		mingw64)
			cp lib/Win64/libOpenAL32.dll.a ${PREFIX}/lib
			cp Win64/soft_oal.dll ${PREFIX}/bin
			;;
		esac
		cd ..
		;;
	*)
		echo "Unsupported platform for OpenAL"
		exit 1
		;;
	esac
}

# Build Ogg
build_ogg() {
	download libogg-${OGG_VERSION}.tar.xz http://downloads.xiph.org/releases/ogg/libogg-${OGG_VERSION}.tar.xz
	cd libogg-${OGG_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build Vorbis
build_vorbis() {
	download libvorbis-${VORBIS_VERSION}.tar.xz http://downloads.xiph.org/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.xz
	cd libvorbis-${VORBIS_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared --disable-examples
	make
	make install
	cd ..
}

# Build Speex
build_speex() {
	download speex-${SPEEX_VERSION}.tar.gz http://downloads.xiph.org/releases/speex/speex-${SPEEX_VERSION}.tar.gz
	cd speex-${SPEEX_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build Theora
build_theora() {
	download libtheora-${THEORA_VERSION}.tar.bz2 http://downloads.xiph.org/releases/theora/libtheora-${THEORA_VERSION}.tar.bz2
	cd libtheora-${THEORA_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared --disable-examples --disable-encode
	make
	make install
	cd ..
}

# Build Opus
build_opus() {
	download opus-${OPUS_VERSION}.tar.gz http://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz
	cd opus-${OPUS_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared
	make
	make install
	cd ..
}

# Build OpusFile
build_opusfile() {
	download opusfile-${OPUSFILE_VERSION}.tar.gz http://downloads.xiph.org/releases/opus/opusfile-${OPUSFILE_VERSION}.tar.gz
	cd opusfile-${OPUSFILE_VERSION}
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-static --disable-shared --disable-http
	make
	make install
	cd ..
}

# Build all pacakges
build_all() {
	build_gmp
	build_nettle
	build_zlib
	build_geoip
	build_sdl2
	build_curl
	build_glew
	build_png
	build_jpeg
	build_webp
	build_freetype
	build_openal
	build_ogg
	build_vorbis
	build_speex
	build_theora
	build_opus
	build_opusfile
}

# Set up environment for 32-bit Windows
setup_mingw32() {
	HOST=i686-w64-mingw32
	PREFIX="${PWD}/mingw32"
	PLATFORM=mingw32
	export CPPFLAGS="-I${PREFIX}/include"
	export LDFLAGS="-L${PREFIX}/lib"
	mkdir -p mingw32
}

# Set up environment for 64-bit Windows
setup_mingw64() {
	HOST=x86_64-w64-mingw32
	PREFIX="${PWD}/mingw64"
	PLATFORM=mingw64
	export CPPFLAGS="-I${PREFIX}/include"
	export LDFLAGS="-L${PREFIX}/lib"
	mkdir -p mingw64
}

# Usage
if [ "$#" == "0" ]; then
	echo "usage: $0 <platform> [package]"
	echo "Platforms: mingw32 mingw64"
	echo "Packages: gmp nettle zlib geoip sdl2 curl glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile"
	exit 1
fi

# Enable parallel build
export MAKEFLAGS="-j8"

# Setup platform
setup_${1}

# Build package
build_${2:-all}
