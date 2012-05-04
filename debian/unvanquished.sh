#! /bin/sh
cd /usr/lib/games/unvanquished
exec ./unvanquished +set fs_libpath "$PWD" +set fs_basepath /var/lib/games/unvanquished "$@"
