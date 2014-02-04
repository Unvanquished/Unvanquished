#!/bin/sh

if [ ! $EUID -eq 0 ]; then
	echo "This script updates the Unvanquished main installation and thus must be run as root."
	exit
fi

# Make sure unvanquished has read access
umask 033

exec /usr/lib/unvanquished/download-pk3.sh /var/lib/unvanquished/pkg /var/cache/unvanquished/update-paks
