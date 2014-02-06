#!/bin/sh
# launcher script for unvanquished tty client

exec /usr/lib/unvanquished/daemon-tty +set fs_libpath /usr/lib/unvanquished +set fs_basepath /var/lib/unvanquished "$@"
