#! /bin/bash
#
# Creates pak.pre.pk3 (containing assets and client translations)
# and vms.pre.pk3 (containing QVMs and game translations)
#
# If there is a directory named 'pak' at the top of the source tree,
# its content is included in pak.pre.pk3.

set -e
shopt -s nullglob

PAKNAME=test
PAKVERSION=0
OLDVERSION=

while :; do
  case "$1" in
    -h|--help)
      echo "Usage: $0 [-n|--name PAK VERSION] [-d|--depends OLDVERSION] TAG [BUILD_DIR]"
      echo '  PAK is the base name for the .pk3 (defaults to "test")'
      echo '  VERSION is the version of the .pk3 (defaults to 0)'
      echo '  OLDVERSION is the previous version of the .pk3, used for dependency info'
      echo '  TAG identifies a git commit; it should normally be a release tag'
      echo '  BUILD_DIR is the build directory, relative to the current'
      exit 0
      ;;

    -n|--name)
      PAKNAME="$2"
      PAKVERSION="$3"
      # note: doesn't test for consecutive punctuation
      if test -z "$PAKNAME" -o -z "$PAKVERSION" -o \
              -z "$(echo "$PAKNAME" | LANG=C grep -e '^[[:alpha:]][-+~./[:alnum:]]*$')" -o \
              -n "$(echo "$PAKVERSION" | LANG=C tr -d -- '-+~.[:alnum:]')"; then
        echo 'you need a valid pak name and version' >&2
        exit 2
      fi
      shift 3
      ;;

    -d|--depends)
      OLDVERSION="$2"
      if test -n "$OLDVERSION" -a \
              -n "$(echo "$OLDVERSION" | LANG=C tr -d -- '-+~.[:alnum:]')"; then
        echo 'you need a valid pak name and version' >&2
        exit 2
      fi
      shift 2
      ;;

    -*)
      echo "unrecognised option '$1'" >&2
      exit 2
      ;;

    *)
      break
      ;;
  esac
done

PAKBASE="$1"
shift || :
BUILDDIR="${1:-.}"
shift || :

if test -z "$PAKBASE"; then
  echo 'the current release tag is needed' >&2
  exit 2
fi

PAKFILENAME="$(basename "$PAKNAME")_$PAKVERSION.pk3"

cd "$(dirname "$0")"
BASEDIR="$PWD"

if test -n "$BUILDDIR" && test ! -d "$BASEDIR/$BUILDDIR"; then
  echo "directory '$BUILDDIR' not found" >&2
  exit 2
fi

rm -f -- "$PAKFILENAME"

dozip()
{
  test $# == 0 || zip -9 "$BASEDIR/$PAKFILENAME" "$@"
}

echo '[33mGathering resource files[m'
cd "$BASEDIR/main"
if test -n "$(git diff --name-only "$PAKBASE" . | sed -e 's%main/%%' | grep -v '^translation/[^c]')"; then
  dozip $(git diff --name-only "$PAKBASE" . | sed -e 's%main/%%' | grep -v '^translation/[^c]')
fi

if test -n "$OLDVERSION"; then
  mkdir -p "$BASEDIR/pak"
  rm -f "$BASEDIR/pak/DEPS"
  echo "$PAKNAME $OLDVERSION" >"$BASEDIR/pak/DEPS"
fi

if test -d "$BASEDIR/pak"; then
  cd "$BASEDIR/pak"
  dozip $(find -type f -a ! -name '*~' | sort)
  cd - >/dev/null
fi

echo '[33mGathering VM code[m'
cd "$BASEDIR/$BUILDDIR"
dozip vm/*qvm

cd "$BASEDIR"

if (which advzip && test -f "$PAKFILENAME") &>/dev/null; then
  echo '[33mOptimising[m'
  advzip -z4 "$PAKFILENAME"
fi

if test -f "$PAKFILENAME"; then
  echo '[32mDone![m'
else
  echo '[32mNothing to do![m'
fi
