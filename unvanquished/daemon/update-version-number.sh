#!/bin/bash

# Exit on error
# Error on undefined variable
set -e
set -u

# Usage
if [ "${#}" -ne "1" ]; then
	echo "usage: ${0} <version>"
	echo "Update all references to the version number"
	exit 1
fi

# Determine source path
SOURCE_PATH="`dirname "${BASH_SOURCE[0]}"`"

# Get version number
VERSION="${1}"
VERSION_SHORT="$(echo "$VERSION" | cut -d. -f1,2)"

# Update q_shared.h
TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
sed "s|\(#define \+PRODUCT_VERSION *\"\)[^\"]*\"|\1${VERSION}\"|" "${SOURCE_PATH}/src/engine/qcommon/q_shared.h" > "${TMP_FILE}"
mv "${TMP_FILE}" "${SOURCE_PATH}/src/engine/qcommon/q_shared.h"

# Update Info.plist
TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
sed "/<key>CFBundleVersion<\/key>/{N;s|<string>[^<]*</string>|<string>${VERSION}</string>|}" "${SOURCE_PATH}/macosx/Info.plist" > "${TMP_FILE}"
mv "${TMP_FILE}" "${SOURCE_PATH}/macosx/Info.plist"
