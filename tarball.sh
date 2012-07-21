#! /bin/sh

set -e

cd "$(dirname "$0")"

PACKAGE_NAME="$(sed -e '/^#define PRODUCT_NAME_LOWER/! d; s/[^\"]*\"//; s/\".*$//; s/[^0-9a-z.]/-/g' src/engine/qcommon/q_shared.h)"
PACKAGE_VERSION="$(sed -e '/^#define PRODUCT_VERSION/! d; s/[^\"]*\"//; s/\".*$//; s/[^0-9a-z.]/-/g' src/engine/qcommon/q_shared.h)"
PACKAGE="$PACKAGE_NAME-$PACKAGE_VERSION"

STRIP_SOURCES=
STRIP_BINARIES=
STRIP_ASSETS=
STRIP_DEAD=
FORMAT=tar.xz

while test $# != 0; do
  case "$1" in
    strip-sources)
      STRIP_SOURCES=1
      ;;
    strip-binaries)
      STRIP_BINARIES=1
      ;;
    strip-assets)
      STRIP_ASSETS=1
      ;;
    strip-unused)
      STRIP_UNUSED=1
      ;;
    strip)
      STRIP_SOURCES=1
      STRIP_BINARIES=1
      STRIP_ASSETS=1
      STRIP_DEAD=1
      ;;
    7z|zip|tar.gz|tar.bz2|tar.xz)
      FORMAT="$1"
      ;;
    -\?|-h|--help)
      echo "$(basename "$0") [options]"
      echo '  builds an optionally stripped-down source archive file'
      echo 'Source control options (at least one):'
      echo '  strip-sources  - remove library code copies'
      echo '  strip-binaries - remove DLLs etc.'
      echo '  strip-assets   - remove various images etc. found in the pak files'
      echo '  strip-unused   - remove some "dead" code'
      echo '  strip          - all of the above'
      echo 'Archive file formats (select one):'
      echo '  7z zip tar.gz tar.bz2 tar.xz'
      echo 'Other options:'
      exit 0
      ;;
    *)
      echo 'Unrecognised argument "'"$1"'"'
      exit 2
      ;;
  esac
  shift
done

cleanup()
{
  rm -rf "$PACKAGE"
}

trap cleanup EXIT 

if test ! -d .git || test "$(git clean -ndf | grep -v '\.#'; git status -s)" != ''; then
  echo 'I need a clean tree!'
  exit 2
fi

# copy the files
mkdir $PACKAGE
cp -al $(ls -1 | grep -v $PACKAGE) $PACKAGE
rm $PACKAGE/tarball.sh

# kill backup files etc.
find $PACKAGE \( -name '.*' -o -name '*~' -o -name '*.orig' -o -name '*.rej' \) -delete

# purge all but *required* libraries
test "$STRIP_SOURCES" = '' ||
  rm -rf $(ls -1d $PACKAGE/src/libs/* | grep -v '/\(cpuinfo\|openexr\|picomodel\|glsl-optimizer\|findlocale\|tinygettext\)')

# purge binaries
test "$STRIP_BINARIES" = '' || {
  rm -rf $PACKAGE/bin
  find $PACKAGE \( -name '*.dll' -o -name '*.lib' \) -delete
}

# purge (most) assets
test "$STRIP_ASSETS" = '' || {
  rm -rf $PACKAGE/main $PACKAGE/main/ui/*[^h]
}

# remove some disabled-by-default code
test "$STRIP_DEAD" = '' || {
  : # nothing to do?
}

# kill empty directories
rmdir --ignore-fail-on-non-empty $(find $PACKAGE -type d | sort -r)

# now build the archive
case "$FORMAT" in
  7z)
    7z a -r $PACKAGE.7z $PACKAGE
    ;;
  zip)
    zip -9r $PACKAGE.zip $PACKAGE
    ;;
  tar.*)
    tar cvaf $PACKAGE.$FORMAT $PACKAGE
    ;;
esac
