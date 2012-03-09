#! /bin/sh
cd /usr/lib/games/unvanquished
exec ./daemon.@ARCH@ +set fs_basepath "$PWD" "$@"