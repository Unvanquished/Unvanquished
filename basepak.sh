#! /bin/sh
#
# Creates pak.pre.pk3 (containing assets and client translations)
# and vms.pre.pk3 (containing QVMs and game translations)
#
# Usage: ./basepak.sh TAG [BUILD_DIR]
# TAG is the version tag for the *current* release.
#
# If there is a directory named 'pak' at the top of the source tree,
# its content is included in pak.pre.pk3.

set -e

PAKBASE="$1"
shift
BUILDDIR="${1:-.}"
shift || :

if test "$PAKBASE" = ''; then
  echo 'the current release tag is needed' >&2
  exit 2
fi

cd "$(dirname "$0")"
BASEDIR="$PWD"

if test -n "$BUILDDIR" && test ! -d "$BASEDIR/$BUILDDIR"; then
  echo "directory '$BUILDDIR' not found" >&2
  exit 2
fi

rm -f -- pak.pre.pk3

echo '[33mGathering resource files[m'
cd "$BASEDIR/main"
if test -n "$(git diff --name-only "$PAKBASE" . | sed -e 's%main/%%' | grep -v '^translation/[^c]')"; then
  zip -9 "$BASEDIR/pak.pre.pk3" $(git diff --name-only "$PAKBASE" . | sed -e 's%main/%%' | grep -v '^translation/[^c]')
fi
if test -d "$BASEDIR/pak"; then
  echo '[33m(including contents of directory '\'pak\'')[m'
  cd "$BASEDIR/pak"
  zip -9 "$BASEDIR/pak.pre.pk3" $(find -type f -a ! -name '*~' | sort)
  cd - >/dev/null
fi

echo '[33mGathering VM code[m'
cd "$BASEDIR/$BUILDDIR"
zip -9 "$BASEDIR/pak.pre.pk3" vm/*qvm


if (which advzip && test -f pak.pre.pk3) &>/dev/null; then
  echo '[33mOptimising[m'
  cd "$BASEDIR"
  advzip -z4 pak.pre.pk3
fi

echo '[32mDone![m'
