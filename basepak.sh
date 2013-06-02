#! /bin/sh
#
# Creates pak.pre.pk3 (containing assets and client translations)
# and vms.pre.pk3 (containing QVMs and game translations)
#
# Usage: ./basepak.sh TAG
# TAG is the version tag for the *current* release.
#
# If there is a directory named 'pak' at the top of the source tree,
# its content is included in pak.pre.pk3.

set -e

PAKBASE="$1"
shift

if test "$PAKBASE" = ''; then
  echo 'missing reference version' >&2
  exit 2
fi

cd "$(dirname "$0")"/main
rm -f ../pak.pre.pk3 ../vms.pre.pk3

echo '[33mBuilding pak.pre.pk3[m'
zip -9 "../pak.pre.pk3" $(git diff --name-only "$PAKBASE" . | sed -e 's%main/%%' | grep -v '^translation/[^c]')
if test -d ../pak; then
  echo '[33m(including contents of directory '\'pak\'')[m'
  cd ../pak
  zip -9 "../pak.pre.pk3" $(find -type f | sort)
  cd - >/dev/null
fi
echo '[33mBuilding vms.pre.pk3[m'
zip -9 "../vms.pre.pk3" translation/game/* vm/*qvm

cd ..
if which advzip &>/dev/null; then
  echo '[33mOptimising[m'
  advzip -z4 pak.pre.pk3 vms.pre.pk3
fi

echo '[32mDone![m'
