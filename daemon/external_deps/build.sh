#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Dependencies version. This number must be updated every time the version
# numbers below change, or packages are added/removed.
DEPS_VERSION=4

# Package versions
PKGCONFIG_VERSION=0.28
NASM_VERSION=2.11.08
ZLIB_VERSION=1.2.8
GMP_VERSION=6.0.0
NETTLE_VERSION=3.1.1
GEOIP_VERSION=1.6.4
CURL_VERSION=7.43.0
SDL2_VERSION=2.0.3
GLEW_VERSION=1.12.0
PNG_VERSION=1.6.18
JPEG_VERSION=1.4.1
WEBP_VERSION=0.4.3
FREETYPE_VERSION=2.6
OPENAL_VERSION=1.16.0
OGG_VERSION=1.3.2
VORBIS_VERSION=1.3.5
SPEEX_VERSION=1.2rc1
THEORA_VERSION=1.1.1
OPUS_VERSION=1.1
OPUSFILE_VERSION=0.6
LUA_VERSION=5.3.1
NACLSDK_VERSION=44.0.2403.155
NCURSES_VERSION=6.0

# Extract an archive into the given subdirectory of the build dir and cd to it
# Usage: extract <filename> <directory>
extract() {
	rm -rf "${BUILD_DIR}/${2}"
	mkdir -p "${BUILD_DIR}/${2}"
	case "${1}" in
	*.tar.bz2)
		tar xjf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.tar.gz|*.tgz)
		tar xzf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.zip)
		unzip -d "${BUILD_DIR}/${2}" "${DOWNLOAD_DIR}/${1}"
		;;
	*.cygtar.bz2)
		# Some Windows NaCl SDK packages have incorrect symlinks, so use
		# cygtar to extract them.
		"${ROOT_DIR}/cygtar.py" -xjf "${DOWNLOAD_DIR}/${1}" -C "${BUILD_DIR}/${2}"
		;;
	*.dmg)
		mkdir -p "${BUILD_DIR}/${2}-dmg"
		hdiutil attach -mountpoint "${BUILD_DIR}/${2}-dmg" "${DOWNLOAD_DIR}/${1}"
		cp -R "${BUILD_DIR}/${2}-dmg/"* "${BUILD_DIR}/${2}/"
		hdiutil detach "${BUILD_DIR}/${2}-dmg"
		rmdir "${BUILD_DIR}/${2}-dmg"
		;;
	*)
		echo "Unknown archive type for ${1}"
		exit 1
		;;
	esac
	cd "${BUILD_DIR}/${2}"
}

# Download a file if it doesn't exist yet, and extract it into the build dir
# Usage: download <filename> <URL> <dir>
download() {
	if [ ! -f "${DOWNLOAD_DIR}/${1}" ]; then
		curl -L -o "${DOWNLOAD_DIR}/${1}" "${2}"
	fi
	extract "${1}" "${3}"
}

# Build pkg-config
build_pkgconfig() {
	download "pkg-config-${PKGCONFIG_VERSION}.tar.gz" "http://pkgconfig.freedesktop.org/releases/pkg-config-${PKGCONFIG_VERSION}.tar.gz" pkgconfig
	cd "pkg-config-${PKGCONFIG_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" --with-internal-glib
	make
	make install
}

# Build NASM
build_nasm() {
	case "${PLATFORM}" in
	macosx*)
		download "nasm-${NASM_VERSION}-macosx.zip" "http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/macosx/nasm-${NASM_VERSION}-macosx.zip" nasm
		cp "nasm-${NASM_VERSION}/nasm" "${PREFIX}/bin"
		;;
	mingw*|msvc*)
		download "nasm-${NASM_VERSION}-win32.zip" "http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/win32/nasm-${NASM_VERSION}-win32.zip" nasm
		cp "nasm-${NASM_VERSION}/nasm.exe" "${PREFIX}/bin"
		;;
	*)
		echo "Unsupported platform for NASM"
		exit 1
		;;
	esac
}

# Build zlib
build_zlib() {
	download "zlib-${ZLIB_VERSION}.tar.gz" "http://zlib.net/zlib-${ZLIB_VERSION}.tar.gz" zlib
	cd "zlib-${ZLIB_VERSION}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		make -f win32/Makefile.gcc PREFIX="${CROSS}"
		make -f win32/Makefile.gcc install BINARY_PATH="${PREFIX}/bin" LIBRARY_PATH="${PREFIX}/lib" INCLUDE_PATH="${PREFIX}/include" SHARED_MODE=1
		;;
	linux*)
		./configure --prefix="${PREFIX}" --static --const
		make
		make install
		;;
	*)
		echo "Unsupported platform for zlib"
		exit 1
		;;
	esac
}

# Build GMP
build_gmp() {
	download "gmp-${GMP_VERSION}a.tar.bz2" "https://gmplib.org/download/gmp/gmp-${GMP_VERSION}a.tar.bz2" gmp
	cd "gmp-${GMP_VERSION}"
	case "${PLATFORM}" in
	msvc*)
		# Configure script gets confused if we override the compiler. Shouldn't
		# matter since gmp doesn't use anything from libgcc.
		local CC_BACKUP="${CC}"
		local CXX_BACKUP="${CXX}"
		unset CC
		unset CXX
		;;
	esac
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
	case "${PLATFORM}" in
	msvc*)
		export CC="${CC_BACKUP}"
		export CXX="${CXX_BACKUP}"
		;;
	esac
}

# Build Nettle
build_nettle() {
	download "nettle-${NETTLE_VERSION}.tar.gz" "http://www.lysator.liu.se/~nisse/archive/nettle-${NETTLE_VERSION}.tar.gz" nettle
	cd "nettle-${NETTLE_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --enable-static
	make
	make install
}

# Build GeoIP
build_geoip() {
	download "GeoIP-${GEOIP_VERSION}.tar.gz" "https://github.com/maxmind/geoip-api-c/archive/v${GEOIP_VERSION}.tar.gz" geoip
	cd "geoip-api-c-${GEOIP_VERSION}"
	autoreconf -vi
	export ac_cv_func_malloc_0_nonnull=yes
	export ac_cv_func_realloc_0_nonnull=yes
	# GeoIP needs -lws2_32 in LDFLAGS
	local TEMP_LDFLAGS="${LDFLAGS}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		TEMP_LDFLAGS="${LDFLAGS} -lws2_32"
		;;
	esac
	LDFLAGS="${TEMP_LDFLAGS}" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build cURL
build_curl() {
	download "curl-${CURL_VERSION}.tar.bz2" "http://curl.haxx.se/download/curl-${CURL_VERSION}.tar.bz2" curl
	cd "curl-${CURL_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" --without-ssl --without-libssh2 --without-librtmp --without-libidn --disable-file --disable-ldap --disable-crypto-auth ${MSVC_SHARED[@]}
	make
	make install
}

# Build SDL2
build_sdl2() {
	case "${PLATFORM}" in
	mingw*)
		download "SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" "http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz" sdl2
		cd "SDL2-${SDL2_VERSION}"
		make install-package arch="${HOST}" prefix="${PREFIX}"
		;;
	msvc*)
		download "SDL2-devel-${SDL2_VERSION}-VC.zip" "http://www.libsdl.org/release/SDL2-devel-${SDL2_VERSION}-VC.zip" sdl2
		cd "SDL2-${SDL2_VERSION}"
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
		;;
	macosx*)
		download "SDL2-${SDL2_VERSION}.dmg" "http://libsdl.org/release/SDL2-${SDL2_VERSION}.dmg" sdl2
		cp -R "SDL2.framework" "${PREFIX}"
		;;
	linux*)
		download "SDL2-${SDL2_VERSION}.tar.gz" "https://www.libsdl.org/release/SDL2-${SDL2_VERSION}.tar.gz" sdl2
		cd "SDL2-${SDL2_VERSION}"
		./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
		make
		make install
		;;
	esac
}

# Build GLEW
build_glew() {
	download "glew-${GLEW_VERSION}.tgz" "http://downloads.sourceforge.net/project/glew/glew/${GLEW_VERSION}/glew-${GLEW_VERSION}.tgz" glew
	cd "glew-${GLEW_VERSION}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		make SYSTEM=mingw GLEW_DEST="${PREFIX}" CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" STRIP="${CROSS}strip" LD="${CROSS}gcc" CFLAGS.EXTRA="${CFLAGS:-}" LDFLAGS.EXTRA="${LDFLAGS:-}"
		make install SYSTEM=mingw GLEW_DEST="${PREFIX}" CC="${CROSS}gcc" AR="${CROSS}ar" RANLIB="${CROSS}ranlib" STRIP="${CROSS}strip" LD="${CROSS}gcc" CFLAGS.EXTRA="${CFLAGS:-}" LDFLAGS.EXTRA="${LDFLAGS:-}"
		;;
	macosx*)
		make SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		make install SYSTEM=darwin GLEW_DEST="${PREFIX}" CC="clang" LD="clang" CFLAGS.EXTRA="${CFLAGS:-} -dynamic -fno-common" LDFLAGS.EXTRA="${LDFLAGS:-}"
		install_name_tool -id "@rpath/libGLEW.${GLEW_VERSION}.dylib" "${PREFIX}/lib/libGLEW.${GLEW_VERSION}.dylib"
		;;
	linux*)
		make GLEW_DEST="${PREFIX}"
		make install GLEW_DEST="${PREFIX}"
		;;
	*)
		echo "Unsupported platform for GLEW"
		exit 1
		;;
	esac
}

# Build PNG
build_png() {
	download "libpng-${PNG_VERSION}.tar.gz" "http://download.sourceforge.net/libpng/libpng-${PNG_VERSION}.tar.gz" png
	cd "libpng-${PNG_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build JPEG
build_jpeg() {
	download "libjpeg-turbo-${JPEG_VERSION}.tar.gz" "http://downloads.sourceforge.net/project/libjpeg-turbo/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz" jpeg
	cd "libjpeg-turbo-${JPEG_VERSION}"
	# JPEG doesn't set -O3 if CFLAGS is defined
	CFLAGS="${CFLAGS:-} -O3" ./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --with-jpeg8
	make
	make install
}

# Build WebP
build_webp() {
	download "libwebp-${WEBP_VERSION}.tar.gz" "http://downloads.webmproject.org/releases/webp/libwebp-${WEBP_VERSION}.tar.gz" webp
	cd "libwebp-${WEBP_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build FreeType
build_freetype() {
	download "freetype-${FREETYPE_VERSION}.tar.bz2" "http://download.savannah.gnu.org/releases/freetype/freetype-${FREETYPE_VERSION}.tar.bz2" freetype
	cd "freetype-${FREETYPE_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --without-bzip2 --without-png --with-harfbuzz=no
	make
	make install
	cp -a "${PREFIX}/include/freetype2" "${PREFIX}/include/freetype"
	mv "${PREFIX}/include/freetype" "${PREFIX}/include/freetype2/freetype"
}

# Build OpenAL
build_openal() {
	case "${PLATFORM}" in
	mingw*|msvc*)
		download "openal-soft-${OPENAL_VERSION}-bin.zip" "http://kcat.strangesoft.net/openal-soft-${OPENAL_VERSION}-bin.zip" openal
		cd "openal-soft-${OPENAL_VERSION}-bin"
		cp -r "include/AL" "${PREFIX}/include"
		case "${PLATFORM}" in
		*32)
			cp "libs/Win32/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win32/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		*64)
			cp "libs/Win64/libOpenAL32.dll.a" "${PREFIX}/lib"
			cp "bin/Win64/soft_oal.dll" "${PREFIX}/bin/OpenAL32.dll"
			;;
		esac
		;;
	macosx*)
		download "openal-soft-${OPENAL_VERSION}.tar.bz2" "http://kcat.strangesoft.net/openal-releases/openal-soft-${OPENAL_VERSION}.tar.bz2" openal
		cd "openal-soft-${OPENAL_VERSION}"
		case "${PLATFORM}" in
		macosx32)
			cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" -DCMAKE_BUILD_TYPE=Release
			;;
		macosx64)
			cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" -DCMAKE_BUILD_TYPE=Release -DALSOFT_EXAMPLES=OFF
			;;
		esac
		make
		make install
		install_name_tool -id "@rpath/libopenal.${OPENAL_VERSION}.dylib" "${PREFIX}/lib/libopenal.${OPENAL_VERSION}.dylib"
		;;
	linux*)
		download "openal-soft-${OPENAL_VERSION}.tar.bz2" "http://kcat.strangesoft.net/openal-releases/openal-soft-${OPENAL_VERSION}.tar.bz2" openal
		cd "openal-soft-${OPENAL_VERSION}"
		cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" -DALSOFT_EXAMPLES=OFF -DLIBTYPE=STATIC .
		make
		make install
		echo -ne "create libopenal-combined.a\naddlib libopenal.a\naddlib libcommon.a\nsave\nend\n" | ar -M
		cp "libopenal-combined.a" "${PREFIX}/lib/libopenal.a"
		;;
	*)
		echo "Unsupported platform for OpenAL"
		exit 1
		;;
	esac
}

# Build Ogg
build_ogg() {
	download "libogg-${OGG_VERSION}.tar.gz" "http://downloads.xiph.org/releases/ogg/libogg-${OGG_VERSION}.tar.gz" ogg
	cd "libogg-${OGG_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build Vorbis
build_vorbis() {
	download "libvorbis-${VORBIS_VERSION}.tar.gz" "http://downloads.xiph.org/releases/vorbis/libvorbis-${VORBIS_VERSION}.tar.gz" vorbis
	cd "libvorbis-${VORBIS_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-examples
	make
	make install
}

# Build Speex
build_speex() {
	download "speex-${SPEEX_VERSION}.tar.gz" "http://downloads.xiph.org/releases/speex/speex-${SPEEX_VERSION}.tar.gz" speex
	cd "speex-${SPEEX_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
	sed "s/deplibs_check_method=.*/deplibs_check_method=pass_all/g" libtool > "${TMP_FILE}"
	mv "${TMP_FILE}" libtool
	make
	make install || make install
}

# Build Theora
build_theora() {
	download "libtheora-${THEORA_VERSION}.tar.bz2" "http://downloads.xiph.org/releases/theora/libtheora-${THEORA_VERSION}.tar.bz2" theora
	cd "libtheora-${THEORA_VERSION}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
		sed "s,EXPORTS,," "win32/xmingw32/libtheoradec-all.def" > "${TMP_FILE}"
		mv "${TMP_FILE}" "win32/xmingw32/libtheoradec-all.def"
		local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
		sed "s,EXPORTS,," "win32/xmingw32/libtheoraenc-all.def" > "${TMP_FILE}"
		mv "${TMP_FILE}" "win32/xmingw32/libtheoraenc-all.def"
		;;
	macosx*)
		local TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
		sed "s/-fforce-addr //" "configure" > "${TMP_FILE}"
		mv "${TMP_FILE}" "configure"
		chmod +x "configure"
		;;
	esac
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-examples --disable-encode
	make
	make install
}

# Build Opus
build_opus() {
	download "opus-${OPUS_VERSION}.tar.gz" "http://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz" opus
	cd "opus-${OPUS_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]}
	make
	make install
}

# Build OpusFile
build_opusfile() {
	download "opusfile-${OPUSFILE_VERSION}.tar.gz" "http://downloads.xiph.org/releases/opus/opusfile-${OPUSFILE_VERSION}.tar.gz" opusfile
	cd "opusfile-${OPUSFILE_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" ${MSVC_SHARED[@]} --disable-http
	make
	make install
}


# Build Lua
build_lua() {
	download "lua-${LUA_VERSION}.tar.gz" "http://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz" lua
	cd "lua-${LUA_VERSION}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		local LUA_PLATFORM=mingw
		;;
	macosx*)
		local LUA_PLATFORM=macosx
		;;
	linux*)
		local LUA_PLATFORM=linux
		;;
	*)
		echo "Unsupported platform for Lua"
		exit 1
		;;
	esac
	make "${LUA_PLATFORM}" CC="${CROSS}gcc" AR="${CROSS}ar rcu" RANLIB="${CROSS}ranlib" MYCFLAGS="${CFLAGS}" MYLDFLAGS="${LDFLAGS}"
	case "${PLATFORM}" in
	mingw*)
		make install TO_BIN="lua.exe luac.exe" TO_LIB="liblua.a" INSTALL_TOP="${PREFIX}"
		;;
	msvc*)
		make install TO_BIN="lua.exe luac.exe lua53.dll" TO_LIB="liblua.a" INSTALL_TOP="${PREFIX}"
		touch "${PREFIX}/lib/lua53.dll.a"
		;;
	*)
		make install INSTALL_TOP="${PREFIX}"
		;;
	esac
}

# Build ncurses
build_ncurses() {
	download "ncurses-${NCURSES_VERSION}.tar.gz" "http://ftp.gnu.org/pub/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" ncurses
	cd "ncurses-${NCURSES_VERSION}"
	./configure --host="${HOST}" --prefix="${PREFIX}" --enable-widec ${MSVC_SHARED[@]}
	make
	make install
}

# Build the NaCl SDK
build_naclsdk() {
	case "${PLATFORM}" in
	mingw*|msvc*)
		local NACLSDK_PLATFORM=win
		local EXE=.exe
		local TAR_EXT=cygtar
		;;
	macosx*)
		local NACLSDK_PLATFORM=mac
		local EXE=
		local TAR_EXT=tar
		;;
	linux*)
		local NACLSDK_PLATFORM=linux
		local EXE=
		local TAR_EXT=tar
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
	download "naclsdk_${NACLSDK_PLATFORM}-${NACLSDK_VERSION}.${TAR_EXT}.bz2" "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/${NACLSDK_VERSION}/naclsdk_${NACLSDK_PLATFORM}.tar.bz2" naclsdk
	cp pepper_*"/tools/sel_ldr_${NACLSDK_ARCH}${EXE}" "${PREFIX}/sel_ldr${EXE}"
	cp pepper_*"/tools/irt_core_${NACLSDK_ARCH}.nexe" "${PREFIX}/irt_core-${DAEMON_ARCH}.nexe"
	cp pepper_*"/toolchain/${NACLSDK_PLATFORM}_x86_newlib/bin/x86_64-nacl-gdb${EXE}" "${PREFIX}/nacl-gdb${EXE}"
	rm -rf "${PREFIX}/pnacl"
	cp -a pepper_*"/toolchain/${NACLSDK_PLATFORM}_pnacl" "${PREFIX}/pnacl"
	rm -rf "${PREFIX}/pnacl/bin/"{i686,x86_64}-nacl-*
	rm -rf "${PREFIX}/pnacl/arm-nacl"
	rm -rf "${PREFIX}/pnacl/arm_bc-nacl"
	rm -rf "${PREFIX}/pnacl/docs"
	rm -rf "${PREFIX}/pnacl/i686_bc-nacl"
	rm -rf "${PREFIX}/pnacl/include"
	rm -rf "${PREFIX}/pnacl/x86_64-nacl"
	rm -rf "${PREFIX}/pnacl/x86_64_bc-nacl"
	case "${PLATFORM}" in
	mingw32|msvc32)
		cp pepper_*"/tools/sel_ldr_x86_64.exe" "${PREFIX}/sel_ldr64.exe"
		cp pepper_*"/tools/irt_core_x86_64.nexe" "${PREFIX}/irt_core-x86_64.nexe"
		;;
	linux32)
		cp pepper_*"/tools/nacl_helper_bootstrap_x86_32" "${PREFIX}/nacl_helper_bootstrap"
		;;
	linux64)
		cp pepper_*"/tools/nacl_helper_bootstrap_x86_64" "${PREFIX}/nacl_helper_bootstrap"
		;;
	esac
}

build_naclports() {
	download "naclports-${NACLSDK_VERSION}.tar.bz2" "https://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/${NACLSDK_VERSION}/naclports.tar.bz2" naclports
	mkdir -p "${PREFIX}/pnacl_deps/"{include,lib}
	cp pepper_*"/ports/include/"{lauxlib.h,lua.h,lua.hpp,luaconf.h,lualib.h} "${PREFIX}/pnacl_deps/include"
	cp -a pepper_*"/ports/include/freetype2" "${PREFIX}/pnacl_deps/include"
	cp pepper_*"/ports/lib/newlib_pnacl/Release/"{liblua.a,libfreetype.a,libpng16.a} "${PREFIX}/pnacl_deps/lib"
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
			DLL=`${CROSS}dlltool -I "${DLL_A}" 2> /dev/null || echo $(basename ${DLL_A} .dll.a).dll`
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
		;;
	*)
		echo "Unsupported platform for gendef"
		exit 1
		;;
	esac
}

# Install all the necessary files to the location expected by CMake
build_install() {
	PKG_PREFIX="${ROOT_DIR}/${PLATFORM}-${DEPS_VERSION}"
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

	# Strip libraries
	case "${PLATFORM}" in
	mingw*)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		;;
	msvc*)
		find "${PKG_PREFIX}/bin" -name '*.dll' -execdir "${CROSS}strip" --strip-unneeded -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.a' -execdir rm -f -- {} \;
		find "${PKG_PREFIX}/lib" -name '*.exp' -execdir rm -f -- {} \;
		;;
	esac
}

# Create a redistributable package for the dependencies
build_package() {
	cd "${ROOT_DIR}"
	case "${PLATFORM}" in
	mingw*|msvc*)
		rm -f "${PLATFORM}-${DEPS_VERSION}.zip"
		zip -r "${PLATFORM}-${DEPS_VERSION}.zip" "${PLATFORM}-${DEPS_VERSION}"
		;;
	*)
		rm -f "${PLATFORM}-${DEPS_VERSION}.tar.bz2"
		tar cvjf "${PLATFORM}-${DEPS_VERSION}.tar.bz2" "${PLATFORM}-${DEPS_VERSION}"
		;;
	esac
}

# Common setup code
common_setup() {
	ROOT_DIR="${PWD}"
	DOWNLOAD_DIR="${PWD}/download_cache"
	BUILD_DIR="${PWD}/build-${PLATFORM}-${DEPS_VERSION}"
	PREFIX="${BUILD_DIR}/prefix"
	export PATH="${PATH}:${PREFIX}/bin"
	export PKG_CONFIG="pkg-config"
	export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
	export CPPFLAGS="${CPPFLAGS:-} -I${PREFIX}/include"
	export LDFLAGS="${LDFLAGS:-} -L${PREFIX}/lib"
	mkdir -p "${DOWNLOAD_DIR}"
	mkdir -p "${PREFIX}"
	mkdir -p "${PREFIX}/bin"
	mkdir -p "${PREFIX}/include"
	mkdir -p "${PREFIX}/lib"
}

# Set up environment for 32-bit Windows for Visual Studio (compile all as .dll)
setup_msvc32() {
	HOST=i686-w64-mingw32
	CROSS="${HOST}-"
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
	CROSS="${HOST}-"
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
	CROSS="${HOST}-"
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m32"
	export CXXFLAGS="-m32"
	export LDFLAGS="-m32"
	common_setup
}

# Set up environment for 64-bit Windows for MinGW (compile all as .a)
setup_mingw64() {
	HOST=x86_64-w64-mingw32
	CROSS="${HOST}-"
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64"
	export CXXFLAGS="-m64"
	export LDFLAGS="-m64"
	common_setup
}

# Set up environment for Mac OS X 32-bit
setup_macosx32() {
	HOST=i686-apple-darwin11
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export MACOSX_DEPLOYMENT_TARGET=10.7
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
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export MACOSX_DEPLOYMENT_TARGET=10.7
	export NASM="${PWD}/build-${PLATFORM}-${DEPS_VERSION}/prefix/bin/nasm" # A newer version of nasm is required for 64-bit
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
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m32"
	export CXXFLAGS="-m32"
	export LDFLAGS="-m32"
	common_setup
}

# Set up environment for 64-bit Linux
setup_linux64() {
	HOST=x86_64-unknown-linux-gnu
	CROSS=
	MSVC_SHARED=(--disable-shared --enable-static)
	export CFLAGS="-m64 -fPIC"
	export CXXFLAGS="-m64 -fPIC"
	export LDFLAGS="-m64"
	common_setup
}

# Usage
if [ "${#}" -lt "2" ]; then
	echo "usage: ${0} <platform> <package[s]...>"
	echo "Script to build dependencies for platforms which do not provide them"
	echo "Platforms: msvc32 msvc64 mingw32 mingw64 macosx32 macosx64 linux32 linux64"
	echo "Packages: pkgconfig nasm zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile lua naclsdk naclports"
	echo "Virtual packages:"
	echo "  install - create a stripped down version of the built packages that CMake can use"
	echo "  package - create a zip/tarball of the dependencies so they can be distributed"
	echo
	echo "Packages requires for each platform:"
	echo "Linux native compile: naclsdk naclports (and possibly others depending on what packages your distribution provides)"
	echo "Linux to Windows cross-compile: zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile lua naclsdk naclports"
	echo "Native Windows compile: pkgconfig nasm zlib gmp nettle geoip curl sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile lua naclsdk naclports"
	echo "Native Mac OS X compile: pkgconfig nasm gmp nettle geoip sdl2 glew png jpeg webp freetype openal ogg vorbis speex theora opus opusfile lua naclsdk naclports"
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
	cd "${ROOT_DIR}"
	"build_${pkg}"
done
