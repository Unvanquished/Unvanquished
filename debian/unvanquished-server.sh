#! /bin/sh
cd /usr/lib/games/unvanquished
exec ./daemonded.@ARCH@ +set fs_libpath "$PWD" +set fs_basepath /var/lib/games/unvanquished "$@"
