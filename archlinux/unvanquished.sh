#!/bin/sh
# launcher script for unvanquished client
# handles unv:// URI's as first parameter securely

if echo "$1" | grep -q '^unv://'; then
	uri=`echo "$1" | grep -o '^unv://[^[:space:]+;]*'`

	if [ -p ~/.Unvanquished/main/pipe ]; then
		echo "connect $uri" > ~/.Unvanquished/main/pipe
	else
		"$0" +connect $uri
	fi
else
	exec /usr/lib/unvanquished/daemon -libpath /usr/lib/unvanquished -pakpath /usr/share/unvanquished/pkg "$@"
fi
