#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Dynamic libraries for inclusion in app bundle
DEPS_VERSION=2
SDL2_VERSION=2.0.3
GLEW_VERSION=1.10.0
OPENAL_VERSION=1.15.1

# Usage
if [ "${#}" -ne "2" ]; then
	echo "usage: ${0} <32-bit build> <64-bit build>"
	echo "Script to build a universal .app for Mac OS X"
	exit 1
fi

# Get paths
SOURCE_PATH="`dirname "${BASH_SOURCE[0]}"`"
DEPS32_PATH="${SOURCE_PATH}/external_deps/macosx32-${DEPS_VERSION}"
DEPS64_PATH="${SOURCE_PATH}/external_deps/macosx64-${DEPS_VERSION}"
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
make_universal sel_ldr 755
make_universal game-nacl-native-exe 755
make_universal cgame-qvm-native.so 644
make_universal ui-qvm-native.so 644
install -m 644 "${BUILD32_PATH}/irt_core-x86.nexe" "${DEST_PATH}/Contents/MacOS/irt_core-x86.nexe"
install -m 644 "${BUILD64_PATH}/irt_core-x86_64.nexe" "${DEST_PATH}/Contents/MacOS/irt_core-x86_64.nexe"

# Create a universal version of GLEW and OpenAL and add it to the bundle
lipo -create -o "${DEST_PATH}/Contents/MacOS/libGLEW.${GLEW_VERSION}.dylib" "${DEPS32_PATH}/lib/libGLEW.${GLEW_VERSION}.dylib" "${DEPS64_PATH}/lib/libGLEW.${GLEW_VERSION}.dylib"
chmod 644 "${DEST_PATH}/Contents/MacOS/libGLEW.${GLEW_VERSION}.dylib"
lipo -create -o "${DEST_PATH}/Contents/MacOS/libopenal.${OPENAL_VERSION}.dylib" "${DEPS32_PATH}/lib/libopenal.${OPENAL_VERSION}.dylib" "${DEPS64_PATH}/lib/libopenal.${OPENAL_VERSION}.dylib"
chmod 644 "${DEST_PATH}/Contents/MacOS/libopenal.${OPENAL_VERSION}.dylib"

# SDL is already compiled as a universal binary, copy as is
cp -a "${DEPS64_PATH}/SDL2.framework" "${DEST_PATH}/Contents/MacOS/"

