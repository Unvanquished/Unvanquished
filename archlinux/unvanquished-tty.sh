#!/bin/sh
# launcher script for unvanquished tty client

exec /usr/lib/unvanquished/daemon-tty -libpath /usr/lib/unvanquished -pakpath /usr/share/unvanquished/pkg "$@"
