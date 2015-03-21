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

# Update download-pk3.sh
TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
sed "s|VERSION=.*|VERSION=${VERSION_SHORT}|" "${SOURCE_PATH}/download-pk3.sh" > "${TMP_FILE}"
mv "${TMP_FILE}" "${SOURCE_PATH}/download-pk3.sh"
chmod +x "${SOURCE_PATH}/download-pk3.sh"

# Update menu_main.rml
TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
sed "/blocklink.*Alpha/s/Alpha \([0-9\\.]\+\)/Alpha ${VERSION_SHORT}/" "${SOURCE_PATH}/main/ui/menu_main.rml" > ${TMP_FILE}
mv "${TMP_FILE}" "${SOURCE_PATH}/main/ui/menu_main.rml"

# Update m# Update menu_main.rml
TMP_FILE="`mktemp /tmp/config.XXXXXXXXXX`"
sed "/blocklink.*Alpha/s/Alpha \([0-9\\.]\+\)/Alpha ${VERSION_SHORT}/" "${SOURCE_PATH}/main/ui/menu_ingame.rml" > ${TMP_FILE}
mv "${TMP_FILE}" "${SOURCE_PATH}/main/ui/menu_ingame.rml"
