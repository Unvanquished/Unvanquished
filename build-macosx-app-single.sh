#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Dynamic libraries for inclusion in app bundle
DEPS_VERSION=3
SDL2_VERSION=2.0.3
GLEW_VERSION=1.12.0
OPENAL_VERSION=1.16.0

# Usage
if [ "${#}" -ne "2" ]; then
	echo "usage: ${0} <arch> <build>"
	echo "Script to build a x86 .app for Mac OS X"
	exit 1
fi

# Determine arch
if [ "${1}" -eq "32" ]; then
	PKG_ARCH="x86"
else
	PKG_ARCH="x86_64"
fi

# Get paths
SOURCE_PATH="`dirname "${BASH_SOURCE[0]}"`"
DEPS_PATH="${SOURCE_PATH}/daemon/external_deps/macosx${1}-${DEPS_VERSION}"
BUILD_PATH="${2}"
DEST_PATH="${PWD}/Unvanquished.app"

# Copy base files
install -d "${DEST_PATH}/Contents"
install -m 644 "${SOURCE_PATH}/macosx/Info.plist" "${DEST_PATH}/Contents"
install -d "${DEST_PATH}/Contents/Resources"
install -m 644 "${SOURCE_PATH}/macosx/Unvanquished.icns" "${DEST_PATH}/Contents/Resources"
install -d "${DEST_PATH}/Contents/MacOS"
install -m 755 "${SOURCE_PATH}/macosx/unvanquished.sh" "${DEST_PATH}/Contents/MacOS"

# Make (non-)universal binaries
make_universal() {
	install -m "${2}" "${BUILD_PATH}/${1}" "${DEST_PATH}/Contents/MacOS/${1}"
}
make_universal daemon 755
make_universal daemonded 755
make_universal daemon-tty 755
make_universal nacl_loader 755
install -m 644 "${BUILD_PATH}/irt_core-${PKG_ARCH}.nexe" "${DEST_PATH}/Contents/MacOS/irt_core-${PKG_ARCH}.nexe"

# Create a universal version of GLEW and OpenAL and add it to the bundle
install -m 644 "${DEPS_PATH}/lib/libGLEW.${GLEW_VERSION}.dylib"  "${DEST_PATH}/Contents/MacOS/libGLEW.${GLEW_VERSION}.dylib"
install -m 644 "${DEPS_PATH}/lib/libopenal.${OPENAL_VERSION}.dylib"  "${DEST_PATH}/Contents/MacOS/libopenal.${OPENAL_VERSION}.dylib"

# SDL is already compiled as a universal binary, just remove the headers
cp -a "${DEPS_PATH}/SDL2.framework" "${DEST_PATH}/Contents/MacOS/"
rm -f "${DEST_PATH}/Contents/MacOS/SDL2.framework/Headers"
rm -rf "${DEST_PATH}/Contents/MacOS/SDL2.framework/Versions/A/Headers"
