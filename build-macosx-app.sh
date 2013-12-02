#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# SDL and GLEW versions for inclusion in app bundle
SDL2_VERSION=2.0.1
GLEW_VERSION=1.10.0

# Usage
if [ "${#}" -ne "2" ]; then
	echo "usage: ${0} <32-bit build> <64-bit build>"
	echo "Script to build a universal .app for Mac OS X"
	exit 1
fi

# Get paths
SOURCE_PATH="`dirname "${BASH_SOURCE[0]}"`"
DEPS32_PATH="${SOURCE_PATH}/external_deps/macosx32"
DEPS64_PATH="${SOURCE_PATH}/external_deps/macosx64"
BUILD32_PATH="${1}"
BUILD64_PATH="${2}"
DEST_PATH="${PWD}/Unvanquished.app"

# Copy base files
install -d "${DEST_PATH}/Contents"
install -m 644 "${SOURCE_PATH}/macosx/Info.plist" "${DEST_PATH}/Contents"
install -d "${DEST_PATH}/Contents/Resources"
install -m 644 "${SOURCE_PATH}/macosx/Unvanquished.icns" "${DEST_PATH}/Contents/Resources"
install -d "${DEST_PATH}/Contents/MacOS"
install -m 755 "${SOURCE_PATH}/macosx/unvanquished.sh" "${DEST_PATH}/Contents/MacOS"

# Make universal binaries
make_universal() {
	lipo -create -o "${DEST_PATH}/Contents/MacOS/${1}" "${BUILD32_PATH}/${1}" "${BUILD64_PATH}/${1}"
	chmod "${2}" "${DEST_PATH}/Contents/MacOS/${1}"
}
make_universal daemon 755
make_universal daemonded 755
make_universal daemon-tty 755
make_universal rendererGL.so 644
make_universal rendererGL3.so 644
install -d "${DEST_PATH}/Contents/MacOS/main"
make_universal main/game.so 644
make_universal main/cgame.so 644
make_universal main/ui.so 644

# Create a universal version of GLEW and add it to the bundle
lipo -create -o "${DEST_PATH}/Contents/MacOS/libGLEW.${GLEW_VERSION}.dylib" "${DEPS32_PATH}/lib/libGLEW.${GLEW_VERSION}.dylib" "${DEPS64_PATH}/lib/libGLEW.${GLEW_VERSION}.dylib"
chmod 644 "${DEST_PATH}/Contents/MacOS/libGLEW.${GLEW_VERSION}.dylib"

# SDL is already compiled as a universal binary, copy as is
cp -a "${DEPS64_PATH}/SDL2.framework" "${DEST_PATH}/Contents/MacOS/"

