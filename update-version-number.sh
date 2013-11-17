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

# Update q_shared.h
sed -i "s|\(#define \+PRODUCT_VERSION *\"\)[^\"]*\"|\1${VERSION}\"|" "${SOURCE_PATH}/src/engine/qcommon/q_shared.h"

# Update download-pk3.sh
sed -i "s|VERSION=.*|VERSION=${VERSION}|" "${SOURCE_PATH}/download-pk3.sh"

# Update Info.plist
sed -i "/<key>CFBundleVersion<\/key>/{N;s|<string>[^<]*</string>|<string>${VERSION}</string>|}" "${SOURCE_PATH}/macosx/Info.plist"
