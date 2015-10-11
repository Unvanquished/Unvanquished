#! /bin/bash
# Retrieve Unvanquished resources from Sourceforge
# Arguments:
# 1. Destination directory
# 2. Cache file directory
# For system-wide installation, you probably want something like
# /usr/lib/games/unvanquished/pkg
# /var/cache/games/unvanquished

# Requirements: GNU coreutils, grep, sed, diff; curl.
# (On GNU/Linux, only curl is likely missing by default.)

set -e

# Work around a reported locale problem
export LANG=C

# Version of Unvanquished for which this script is built
VERSION=0.44

# Download from here
CDN_URL="http://cdn.unvanquished.net/$VERSION/pkg/"
BASE_URL='http://downloads.sourceforge.net/project/unvanquished/Assets/'
MIRROR_DOMAIN='dl.sourceforge.net'
MIRROR_URL="http://%s.$MIRROR_DOMAIN/project/unvanquished/Assets/"

MD5SUMS_BASE="md5sums$VERSION"
MD5SUMS_CDN='md5sums'
MD5SUMS="$MD5SUMS_BASE"

# Default destination directory
case "$(uname -s)" in
  Darwin)
    DEFAULT_DEST_DIR=~/Library/Application\ Support/Unvanquished/pkg
    ;;
  *)
    DEFAULT_DEST_DIR=~/.unvanquished/pkg
    ;;
esac

# Option flags
RUN_DOWNLOAD=1

DONE=
while test -z "$DONE"; do
  case "$1" in
    --help|-h|-\?)
      echo "$0: download Unvanquished game files"
      echo "Usage: $0 [option] DESTINATION_DIR [CACHE_DIR]]"
      echo "Default destination directory is $DEFAULT_DEST_DIR"
      echo "Options:"
      echo "  -v, --verify     Verify downloaded .pk3s"
      echo "  --source MIRROR  Download from MIRROR"
      echo "  --cdn            Download from cdn.unvanquished.net"
      echo
      echo "Mirror URL examples (not real URLs!):"
      echo "  http://example.com/unvanquished/"
      echo "    -> full URL"
      echo "  example"
      echo "    -> $(printf "$MIRROR_URL" example)"

      exit 0
      ;;

    --verify|-v)
      RUN_DOWNLOAD=
      shift
      ;;

    --source)
      shift
      BASE_URL="$1"
      shift
      # Single word? Expand it
      if echo "$BASE_URL" | grep -vq '[./]'; then
        BASE_URL="$(printf "$MIRROR_URL" "$1")"
      fi
      MD5SUMS="$MD5SUMS_BASE"
      ;;

    --cdn)
      shift
      BASE_URL="$CDN_URL"
      MD5SUMS="$MD5SUMS_CDN"
      ;;

    *)
      DONE=1
      ;;
  esac
done

if test -n "$RUN_DOWNLOAD"; then
  if test "${BASE_URL/#*:\/\/*\//://}" != '://' -o "$BASE_URL" = '://'; then
    echo "$0: --source $BASE_URL is not valid"
    exit 2
  fi
fi

# Paths passed to script
DEST_DIR="${1:-$DEFAULT_DEST_DIR}"
CACHE="${2:-${DEST_DIR}}"

# Set up for clean-up on exit
# NOTE THAT THIS INDISCRIMINATELY REMOVES *.tmp FROM DEST & CACHE DIRS
cleanup ()
{
  RET="$?"
  test "$RET" = 0 || echo '[1;31mFAILED.[m'
  rm -f -- "$CACHE/"*.tmp "$DEST_DIR/"*.tmp
  return "$RET"
}
trap cleanup EXIT

echo "[1;32mCache directory:    [33m$CACHE
[1;32mDownload directory: [33m$DEST_DIR[m"

if test -n "$RUN_DOWNLOAD"; then
  echo "[1;32mDownloading from:   [33m$BASE_URL[m"
fi

# Create directories if needed
mkdir -p -- "$DEST_DIR" "$CACHE"

# Canonicalise the pathnames
CURDIR="$PWD"
cd "$DEST_DIR"
DEST_DIR="$PWD"
cd "$CURDIR"
cd "$CACHE"
CACHE="$PWD"
cd "$CURDIR"

# Utility function for downloading. Saves in a temp dir.
# $1 = destination directory
# $2 = leafname
# $3 = retry using / save as (defaults to $2)
download ()
{
  echo "[0;33mDownloading $2 ${3:+as $3 }to $1...[m"
  if test "$3" != ''; then
    curl --retry 5 -fLo "$1/$3.tmp" "$BASE_URL$2" || {
      echo "[0;33mDownloading $3 instead...[m"
      curl --retry 5 -fLo "$1/$3.tmp" "$BASE_URL$3" || return 1
    }
  else
    curl --retry 5 -fLo "$1/$2.tmp" "$BASE_URL$2" || return 1
  fi
  mv -f "$1/${3:-$2}.tmp" "$1/${3:-$2}"
  echo '[32mDownloaded.[m'
}

# Utility function for file verification
verify_md5sums()
{
  (
    set -e
    cd "$DEST_DIR"
    while read record; do
      filename=`echo "$record" | sed 's/^.* [ *]//'`
      sum=`echo "$record" | cut -d" " -f1`
      if [ ! -f $filename ]; then
        echo "$filename"
      else
        if [ "`md5sum $filename | cut -d" " -f1`" != "$sum" ]; then
          echo "$filename"
        fi
      fi
    done < "$CACHE/md5sums"
  )
}

if test -n "$RUN_DOWNLOAD"; then
  # Get the md5sum checksums
  test -f "$CACHE/md5sums" && mv -f "$CACHE/md5sums" "$CACHE/md5sums.old"
  download "$CACHE" "$MD5SUMS" "md5sums"
elif test ! -f "$CACHE/md5sums"; then
  echo '[1;31mNo md5sums file found.[m'
  exit 1
fi

# Check that the file is properly formatted
echo '[33mVerifying md5sums integrity...[m'
if grep -sqve '^[0-9a-f]\{32\} [ *][^ .\\/][^ \\/]*$' "$CACHE/md5sums"; then
  rm -f "$CACHE/md5sums"
  test -f "$CACHE/md5sums.old" && mv -f "$CACHE/md5sums.old" "$CACHE/md5sums"
  exit 1
fi
echo '[32mSuccessful.[m'

# List the files whose checksums are listed
echo '[33mListed files:[m'
cut -d' ' -f2- <"$CACHE/md5sums"

if test -n "$RUN_DOWNLOAD"; then
  echo "[33mDownloading missing or updated files...[m"
  # Download those files unless the local copies match the checksums
  verify_md5sums |
    cut -d: -f1 |
    while read i; do
      download "$DEST_DIR" "$(basename "$i")" || : # carry on with the next file if this one fails
    done

  if test -f "$CACHE/md5sums.old"; then
    echo "[33mRemoving previously referenced files...[m"
    # Remove files listed in the old md5sums file
    diff <(sed -e 's/^.* [ *]//' "$CACHE/md5sums.old") <(sed -e 's/^.* [ *]//' "$CACHE/md5sums") |
      sed -e '/^< /! d; s/^< //' |
      while read i; do
        i="$(basename "$i")"
        echo " $i"
        rm -f "$DEST_DIR/$i"
      done
  fi
fi

echo "[0;33mVerifying files...[m"
if verify_md5sums; then
  echo "[0;32mVerified.[m"
else
  echo "[0;31mErrors found during verification. Re-downloading is recommended.[m"
fi

# All done :-)
echo '[1;32mFinished.[m'

exit 0
