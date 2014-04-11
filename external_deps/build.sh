#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Dependencies version. This number must be updated every time the version
# numbers below change, or packages are added/removed.
DEPS_VERSION=1

# Package versions
PKGCONFIG_VERSION=0.28
NASM_VERSION=2.11.02
ZLIB_VERSION=1.2.8
GMP_VERSION=6.0.0
NETTLE_VERSION=2.7.1
GEOIP_VERSION=1.6.0
CURL_VERSION=7.36.0
SDL2_VERSION=2.0.3
GLEW_VERSION=1.10.0
PNG_VERSION=1.6.10
JPEG_VERSION=1.3.1
WEBP_VERSION=0.4.0
FREETYPE_VERSION=2.5.3
OPENAL_VERSION=1.15.1
OGG_VERSION=1.3.1
VORBIS_VERSION=1.3.4
SPEEX_VERSION=1.2rc1
THEORA_VERSION=1.1.1
OPUS_VERSION=1.1
OPUSFILE_VERSION=0.5
NACLSDK_VERSION=34.0.1825.4

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
		dmg)
			;;
		*)
			echo "Unknown archive type for ${1}"
			exit 1
			;;
		esac
	fi
}

# Build pkg-config
build_pkgconfig() {
	download "pkg-config-${PKGCONFIG_VERSION}.tar.gz" "http://pkgconfig.freedesktop.org/releases/pkg-config-${PKGCONFIG_VERSION}.tar.gz"
	cd "pkg-config-${PKGCONFIG_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --with-internal-glib
	make clean
	make
	make install
	cd ..
}

# Build NASM
build_nasm() {
	case "${PLATFORM}" in
	macosx*)
		download "nasm-${NASM_VERSION}-macosx.zip" "http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/macosx/nasm-${NASM_VERSION}-macosx.zip"
		[ -d "nasm-${NASM_VERSION}-macosx" ] || mv "nasm-${NASM_VERSION}" "nasm-${NASM_VERSION}-macosx"
		cp "nasm-${NASM_VERSION}-macosx/nasm" "${PREFIX}/bin"
		;;
	mingw*|msvc*)
		download "nasm-${NASM_VERSION}-win32.zip" "http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/win32/nasm-${NASM_VERSION}-win32.zip"
		[ -d "nasm-${NASM_VERSION}-win32" ] || mv "nasm-${NASM_VERSION}" "nasm-${NASM_VERSION}-win32"
		cp "nasm-${NASM_VERSION}-win32/nasm.exe" "${PREFIX}/bin"
		;;
	*)
		echo "Unsupported platform for NASM"
		exit 1
		;;
	esac
}

# Build zlib
build_zlib() {
	download "zlib-${ZLIB_VERSION}.tar.xz" "http://zlib.net/zlib-${ZLIB_VERSION}.tar.xz"
	cd "zlib-${ZLIB_VERSION}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		make -f win32/Makefile.gcc clean
		make -f win32/Makefile.gcc PREFIX="${HOST}-"
		make -f win32/Makefile.gcc install BINARY_PATH="${PREFIX}/bin" LIBRARY_PATH="${PREFIX}/lib" INCLUDE_PATH="${PREFIX}/include" SHARED_MODE=1
		;;
	*)
		echo "Unsupported platform for zlib"
		exit 1
		;;
	esac
	cd ..
}

# Build GMP
build_gmp() {
	download "gmp-${GMP_VERSION}a.tar.xz" "ftp://ftp.gmplib.org/pub/gmp/gmp-${GMP_VERSION}a.tar.xz"
	cd "gmp-${GMP_VERSION}"
	make distclean || true
	case "${PLATFORM}" in
	msvc*)
		# Configure script gets confused if we overide the compiler. Shouldn't
		# matter since gmp doesn't use anything from libgcc.
		local CC_BACKUP="${CC}"
		local CXX_BACKUP="${CXX}"
		unset CC
		unset CXX
		;;
	esac
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	case "${PLATFORM}" in
	msvc*)
		export CC="${CC_BACKUP}"
		export CXX="${CXX_BACKUP}"
		;;
	esac
	cd ..
}

# Build Nettle
build_nettle() {
	download "nettle-${NETTLE_VERSION}.tar.gz" "http://www.lysator.liu.se/~nisse/archive/nettle-${NETTLE_VERSION}.tar.gz"
	cd "nettle-${NETTLE_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --enable-static
	make clean
	make
	make install
	cd ..
}

# Build GeoIP
build_geoip() {
	download "GeoIP-${GEOIP_VERSION}.tar.gz" "http://www.maxmind.com/download/geoip/api/c/GeoIP-${GEOIP_VERSION}.tar.gz"
	cd "GeoIP-${GEOIP_VERSION}"
	export ac_cv_func_malloc_0_nonnull=yes
	export ac_cv_func_realloc_0_nonnull=yes
	make distclean || true
	# GeoIP needs -lws2_32 in LDFLAGS
	local TEMP_LDFLAGS="${LDFLAGS}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		TEMP_LDFLAGS="${LDFLAGS} -lws2_32"
		;;
	esac
	LDFLAGS="${TEMP_LDFLAGS}" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	cd ..
}

# Build cURL
build_curl() {
	download "curl-${CURL_VERSION}.tar.bz2" "http://curl.haxx.se/download/curl-${CURL_VERSION}.tar.bz2"
	cd "curl-${CURL_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-shared
	make clean
	make
	make install
	cd ..
}

# Build SDL2
build_sdl2() {
	case "${PLATFORM}" in
	mingw*)
		download "SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" "http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz"
		[ -d "SDL2-${SDL2_VERSION}-mingw" ] || mv "SDL2-${SDL2_VERSION}" "SDL2-${SDL2_VERSION}-mingw"
		cd "SDL2-${SDL2_VERSION}-mingw"
		make install-package arch="${HOST}" prefix="${PREFIX}"
		cd ..
		;;
	msvc*)
		download "SDL2-devel-${SDL2_VERSION}-VC.zip" "http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-VC.zip"
		[ -d "SDL2-${SDL2_VERSION}-msvc" ] || mv "SDL2-${SDL2_VERSION}" "SDL2-${SDL2_VERSION}-msvc"
		cd "SDL2-${SDL2_VERSION}-msvc"
		mkdir -p "${PREFIX}/include/SDL2"
		cp include/* "${PREFIX}/include/SDL2"
		case "${PLATFORM}" in
		msvc32)
			cp lib/x86/*.lib "${PREFIX}/lib"
			cp lib/x86/*.dll "${PREFIX}/bin"
			;;
		msvc64)
			cp lib/x64/*.lib "${PREFIX}/lib"
			cp lib/x64/*.dll "${PREFIX}/bin"
			;;
		esac
		cd ..
		;;
	macosx*)
		download "SDL2-${SDL2_VERSION}.dmg" "http://libsdl.org/release/SDL2-${SDL2_VERSION}.dmg"
		mkdir -p "SDL2-${SDL2_VERSION}-mac"
		hdiutil attach -mountpoint "SDL2-${SDL2_VERSION}-mac" "SDL2-${SDL2_VERSION}.dmg"
		rm -rf "${PREFIX}/SDL2.framework"
		cp -R "SDL2-${SDL2_VERSION}-mac/SDL2.framework" "${PREFIX}"
		hdiutil detach "SDL2-${SDL2_VERSION}-mac"
		;;
	*)
		echo "Unsupported platform for SDL2"
		exit 1
		;;
	esac
}

# Build GLEW
build_glew() {
	download "glew-${GLEW_VERSION}.tgz" "http://downloads.sourceforge.net/project/glew/glew/${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz"
	cd "glew-${GLEW_VERSION}"
	make distclean
	case "${PLATFORM}" in
	mingw*|msvc*)
		make SYSTEM=mingw GLEW_DEST="${PREFIX}" CC="${HOST}-gcc" AR="${HOST}-ar" RANLIB="${HOST}-ranlib" STRIP="${HOST}-strip" LD="${HOST}-gcc"
		make install SYSTEM=mingw GLEW_DEST="${PREFIX}" CC="${HOST}-gcc" AR="${HOST}-ar" RANLIB="${HOST}-ranlib" STRIP="${HOST}-strip" LD="${HOST}-gcc"
		;;
	macosx*)
		make SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		make install SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		install_name_tool -id "@rpath/libGLEW.${GLEW_VERSION}.dylib" "${PREFIX}/lib/libGLEW.${GLEW_VERSION}.dylib"
		;;
	*)
		echo "Unsupported platform for GLEW"
		exit 1
		;;
	esac
	cd ..
}

# Build PNG
build_png() {
	download "libpng-${PNG_VERSION}.tar.xz" "http://download.sourceforge.net/libpng/libpng-${PNG_VERSION}.tar.xz"
	cd "libpng-${PNG_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	cd ..
}

# Build JPEG
build_jpeg() {
	download "libjpeg-turbo-${JPEG_VERSION}.tar.gz" "http://downloads.sourceforge.net/project/libjpeg-turbo/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz"
	cd "libjpeg-turbo-${JPEG_VERSION}"
	make distclean || true
	# JPEG doesn't set -O3 if CFLAGS is defined
	CFLAGS="${CFLAGS:-} -O3" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --with-jpeg8
	make clean
	make
	make install
	cd ..
}

# Build WebP
build_webp() {
	download "libwebp-${WEBP_VERSION}.tar.gz" "https://webp.googlecode.com/files/libwebp-${WEBP_VERSION}.tar.gz"
	cd "libwebp-${WEBP_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	cd ..
}

# Build FreeType
build_freetype() {
	download "freetype-${FREETYPE_VERSION}.tar.bz2" "http://download.savannah.gnu.org/releases/freetype/freetype-${FREETYPE_VERSION}.tar.bz2"
	cd "freetype-${FREETYPE_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --without-bzip2 --without-png --with-harfbuzz=no
	make clean
	make
	make install
	cd ..
}

# Build OpenAL
build_openal() {
	case "${PLATFORM}" in
	mingw*|msvc*)
		download "openal-soft-${OPENAL_VERSION}-bin.zip" "http://kcat.strangesoft.net/openal-soft-${OPENAL_VERSION}-bin.zip"
		cd "openal-soft-${OPENAL_VERSION}-bin"
		cp -r "include/AL" "${PREFIX}/include"
		case "${PLATFORM}" in
		*32)
			cp "lib/Win32/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "Win32/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		*64)
			cp "lib/Win64/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "Win64/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		esac
		cd ..
		;;
	macosx*)
		download "openal-soft-${OPENAL_VERSION}.tar.bz2" "http://kcat.strangesoft.net/openal-releases/openal-soft-${OPENAL_VERSION}.tar.bz2"
		cd "openal-soft-${OPENAL_VERSION}"
		rm -rf CMakeCache.txt CMakeFiles
		case "${PLATFORM}" in
		macosx32)
			cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 -DCMAKE_BUILD_TYPE=Release
			;;
		macosx64)
			cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 -DCMAKE_BUILD_TYPE=Release
			;;
		esac
		make clean
		make
		make install
		install_name_tool -id "@rpath/libopenal.${OPENAL_VERSION}.dylib" "${PREFIX}/lib/libopenal.${OPENAL_VERSION}.dylib"
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
	download "libogg-${OGG_VERSION}.tar.xz" "http://downloads.xiph.org/releases/ogg/libogg-${OGG_VERSION}.tar.xz"
	cd "libogg-${OGG_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	cd ..
}

# Build Vorbis
build_vorbis() {
	download "libvorbis-${VORBIS_VERSION}.tar.xz" "http://downloads.xiph.org/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.xz"
	cd "libvorbis-${VORBIS_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-examples
	make clean
	make
	make install
	cd ..
}

# Build Speex
build_speex() {
	download "speex-${SPEEX_VERSION}.tar.gz" "http://downloads.xiph.org/releases/speex/speex-${SPEEX_VERSION}.tar.gz"
	cd "speex-${SPEEX_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
	sed "s/deplibs_check_method=.*/deplibs_check_method=pass_all/g" libtool > "${TMP_FILE}"
	mv "${TMP_FILE}" libtool
	make clean
	make
	make install
	cd ..
}

# Build Theora
build_theora() {
	download "libtheora-${THEORA_VERSION}.tar.bz2" "http://downloads.xiph.org/releases/theora/libtheora-${THEORA_VERSION}.tar.bz2"
	cd "libtheora-${THEORA_VERSION}"
	make distclean || true
	case "${PLATFORM}" in
	mingw*|msvc*)
		local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
		sed "s,EXPORTS,," "win32/xmingw32/libtheoradec-all.def" > "${TMP_FILE}"
		mv "${TMP_FILE}" "win32/xmingw32/libtheoradec-all.def"
		local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
		sed "s,EXPORTS,," "win32/xmingw32/libtheoraenc-all.def" > "${TMP_FILE}"
		mv "${TMP_FILE}" "win32/xmingw32/libtheoraenc-all.def"
		;;
	esac
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-examples --disable-encode
	make clean
	make
	make install
	cd ..
}

# Build Opus
build_opus() {
	download "opus-${OPUS_VERSION}.tar.gz" "http://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz"
	cd "opus-${OPUS_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make clean
	make
	make install
	cd ..
}

# Build OpusFile
build_opusfile() {
	download "opusfile-${OPUSFILE_VERSION}.tar.gz" "http://downloads.xiph.org/releases/opus/opusfile-${OPUSFILE_VERSION}.tar.gz"
	cd "opusfile-${OPUSFILE_VERSION}"
	make distclean || true
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-http
	make clean
	make
	make install
	cd ..
}

# Build the NaCl SDK
build_naclsdk() {
	case "${PLATFORM}" in
	mingw*|msvc*)
		local NACLSDK_PLATFORM=win
		local EXE=.exe
		;;
	macosx*)
		local NACLSDK_PLATFORM=mac
		local EXE=
		;;
	linux*)
		local NACLSDK_PLATFORM=linux
		local EXE=
		;;
	esac
	case "${PLATFORM}" in
	*32)
		local NACLSDK_ARCH=x86_32
		local DAEMON_ARCH=x86
		;;
	*64)
		local NACLSDK_ARCH=x86_64
		local DAEMON_ARCH=x86_64
		;;
	esac
	mkdir -p "naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}"
	cd "naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}"
	download "naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}.tar.bz2" "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/${NACLSDK_VERSION}/naclsdk_${NACLSDK_PLATFORM}.tar.bz2"
	cp pepper_*"/tools/sel_ldr_${NACLSDK_ARCH}${EXE}" "${PREFIX}/sel_ldr${EXE}"
	cp pepper_*"/tools/irt_core_${NACLSDK_ARCH}.nexe" "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
	cp -aT pepper_*"/toolchain/${NACLSDK_PLATFORM}_pnacl" "${PREFIX}/pnacl"
	case "${PLATFORM}" in
	linux32)
		cp pepper_*"/tools/nacl_helper_bootstrap_x86_32" "${PREFIX}/nacl_helper_bootstrap"
		rm -rf "${PREFIX}/pnacl/bin64"
		rm -rf "${PREFIX}/pnacl/host_x86_64"
		;;
	linux64)
		cp pepper_*"/tools/nacl_helper_bootstrap_x86_64" "${PREFIX}/nacl_helper_bootstrap"
		rm -rf "${PREFIX}/pnacl/bin"
		rm -rf "${PREFIX}/pnacl/host_x86_32"
		;;
	esac
	cd ..
}

# For MSVC, we need to use the Microsoft LIB tool to generate import libraries,
# the import libraries generated by MinGW seem to have issues. Instead we
# generate a .bat file to be run using the Visual Studio tools command shell.
build_gendef() {
	case "${PLATFORM}" in
	msvc*)
		mkdir -p "${PREFIX}/def"
		cd "${PREFIX}/def"
		echo 'cd /d "%~dp0"' > "${PREFIX}/genlib.bat"
		for DLL_A in "${PREFIX}"/lib/*.dll.a; do
			DLL=`${HOST}-dlltool -I "${DLL_A}"`
			DEF=`basename ${DLL} .dll`.def
			LIB=`basename ${DLL_A} .dll.a`.lib
			MACHINE=`[ "${PLATFORM}" = msvc32 ] && echo x86 || echo x64`

			# Using gendef from mingw-w64-tools
			gendef "${PREFIX}/bin/${DLL}"

			# Fix some issues with gendef output
			TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
			sed "s/\(glew.*\)@4@4/\1@4/" "${DEF}" > "${TMP_FILE}"
			sed "s/ov_halfrate_p@0/ov_halfrate_p/" "${TMP_FILE}" > "${DEF}"
			rm -f "${TMP_FILE}"

			echo "lib /def:def\\${DEF} /machine:${MACHINE} /out:lib\\${LIB}" >> "${PREFIX}/genlib.bat"
		done
		cd ../..
		;;
	*)
		echo "Unsupported platform for gendef"
		exit 1
		;;
	esac
}

# Clean the target directory and build a package
build_package() {
	mkdir -p "${PWD}/pkg"
	PKG_PREFIX="${PWD}/pkg/${PLATFORM}-${DEPS_VERSION}"
	rm -rf "${PKG_PREFIX}"
	rsync -a --link-dest="${PREFIX}" "${PREFIX}/" "${PKG_PREFIX}"

	# Remove all unneeded files
	rm -rf "${PKG_PREFIX}/man"
	rm -rf "${PKG_PREFIX}/def"
	rm -rf "${PKG_PREFIX}/share"
	rm -f "${PKG_PREFIX}/genlib.bat"
	rm -rf "${PKG_PREFIX}/lib/pkgconfig"
	find "${PKG_PREFIX}/bin" -not -type d -not -name '*.dll' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.la' -execdir rm -f -- {} \;
	find "${PKG_PREFIX}/lib" -name '*.dll.a' -execdir bash -c 'rm -f -- "`basename "{}" .dll.a`.a"' \;
	find "${PKG_PREFIX}/lib" -name '*.dylib' -execdir bash -c 'rm -f -- "`basename "{}" .dylib`.a"' \;
	rmdir "${PKG_PREFIX}/bin" 2> /dev/null || true
	rmdir "${PKG_PREFIX}/include" 2> /dev/null || true
	rmdir "${PKG_PREFIX}/lib" 2> /dev/null || true
	case "${PLATFORM}" in
	mingw*)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		;;
	msvc*)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${HOST}-strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir rm -f -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.exp' -execdir rm -f -- {} \;
		;;
	esac

	cd pkg
	case "${PLATFORM}" in
	mingw*|msvc*)
		zip -r "${PLATFORM}-${DEPS_VERSION}.zip" "${PLATFORM}-${DEPS_VERSION}"
		;;
	*)
		tar cvjf "${PLATFORM}-${DEPS_VERSION}.tar.bz2" "${PLATFORM}-${DEPS_VERSION}"
		;;
	esac
	cd ..
}

# Common setup code
common_setup() {
	PREFIX="${PWD}/${PLATFORM}-${DEPS_VERSION}"
	export PATH="${PATH}:${PREFIX}/bin"
	export PKG_CONFIG="pkg-config"
	export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
	export CPPFLAGS="${CPPFLAGS:-} -I${PREFIX}/include"
	export LDFLAGS="${LDFLAGS:-} -L${PREFIX}/lib"
	mkdir -p "${PREFIX}"
	mkdir -p "${PREFIX}/bin"
	mkdir -p "${PREFIX}/include"
	mkdir -p "${PREFIX}/lib"
}

# Set up environment for 32-bit Windows for Visual Studio (compile all as .dll)
setup_msvc32() {
	HOST=i686-w64-mingw32
	MSVC_SHARED=(--enable-shared --disable-static)
	# Libtool bug prevents -static-libgcc from being set in LDFLAGS
	export CC="i686-w64-mingw32-gcc -static-libgcc"
	export CXX="i686-w64-mingw32-g++ -static-libgcc"
	export CFLAGS="-m32"
	export CXXFLAGS="-m32"
	export LDFLAGS="-m32"
	common_setup
}

# Set up environment for 64-bit Windows for Visual Studio (compile all as .dll)
setup_msvc64() {
	HOST=x86_64-w64-mingw32
	MSVC_SHARED=(--enable-shared --disable-static)
	# Libtool bug prevents -static-libgcc from being set in LDFLAGS
	export CC="x86_64-w64-mingw32-gcc -static-libgcc"
	export CXX="x86_64-w64-mingw32-g++ -static-libgcc"
	export CFLAGS="-m64"
	export CXXFLAGS="-m64"
	export LDFLAGS="-m64"
	common_setup
}

# Set up environment for 32-bit Windows for MinGW (compile all as .a)
setup_mingw32() {
	HOST=i686-w64-mingw32
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m32"
	export CXXFLAGS="-m32"
	export LDFLAGS="-m32"
	common_setup
}

# Set up environment for 64-bit Windows for MinGW (compile all as .a)
setup_mingw64() {
	HOST=x86_64-w64-mingw32
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64"
	export CXXFLAGS="-m64"
	export LDFLAGS="-m64"
	common_setup
}

# Set up environment for Mac OS X 32-bit
setup_macosx32() {
	HOST=i686-apple-darwin11
	MSVC_SHARED=(--disable-shared --enable-static)
	export MACOSX_DEPLOYMENT_TARGET=10.6
	export CC=clang
	export CXX=clang++
	export CFLAGS="-arch i386"
	export CXXFLAGS="-arch i386"
	export LDFLAGS="-arch i386"
	common_setup
}

# Set up environment for Mac OS X 64-bit
setup_macosx64() {
	HOST=x86_64-apple-darwin11
	MSVC_SHARED=(--disable-shared --enable-static)
	export MACOSX_DEPLOYMENT_TARGET=10.6
	export NASM="${PWD}/macosx64-${DEPS_VERSION}/bin/nasm" # A newer version of nasm is required for 64-bit
	export CC=clang
	export CXX=clang++
	export CFLAGS="-arch x86_64"
	export CXXFLAGS="-arch x86_64"
	export LDFLAGS="-arch x86_64"
	common_setup
}

# Set up environment for 32-bit Linux
setup_linux32() {
	HOST=i686-pc-linux-gnu
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m32"
	export CXXFLAGS="-m32"
	export LDFLAGS="-m32"
	common_setup
}

# Set up environment for 64-bit Linux
setup_linux64() {
	HOST=x86_64-unknown-linux-gnu
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64"
	export CXXFLAGS="-m64"
	export LDFLAGS="-m64"
	common_setup
}

# Usage
if [ "${#}" -lt "2" ]; then
	echo "usage: ${0} <platform> <package[s]...>"
	echo "Script to build dependencies for platforms which do not provide them"
	echo "Platforms: msvc32 msvc64 mingw32 mingw64 macosx32 macosx64"
	echo "Packages: pkgconfig nasm zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile naclsdk"
	echo
	echo "Packages requires for each platform:"
	echo "Linux native compile: naclsdk (and possibly others depending on what packages your distribution provides)"
	echo "Linux to Windows cross-compile: zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile naclsdk"
	echo "Native Windows compile: pkgconfig nasm zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile naclsdk"
	echo "Native Mac OS X compile: pkgconfig nasm gmp nettle geoip sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile naclsdk"
	exit 1
fi

# Enable parallel build
export MAKEFLAGS="-j`nproc 2> /dev/null || sysctl -n hw.ncpu 2> /dev/null || echo 1`"

# Setup platform
PLATFORM="${1}"
"setup_${PLATFORM}"

# Build packages
shift
for pkg in "${@}"; do
	"build_${pkg}"
done
