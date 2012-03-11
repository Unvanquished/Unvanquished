#! /bin/sh
cd /usr/lib/games/unvanquished
exec ./daemonded.@ARCH@ +set fs_basepath "$PWD" "$@"