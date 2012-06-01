#! /bin/sh
cd /var/games/unvanquished-server
exec /usr/lib/games/unvanquished/unvanquishedded +set fs_libpath /usr/lib/games/unvanquished +set fs_basepath /var/games/unvanquished +exec server.cfg "$@"
