#! /bin/sh
# Retrieve Unvanquished resources from Sourceforge
# Arguments:
# 1. Destination directory
# 2. Cache file directory
# For system-wide installation, you probably want something like
# /usr/lib/games/unvanquished/main
# /var/cache/games/unvanquished

set -e

# Download from here
BASE_URL='http://downloads.sourceforge.net/project/unvanquished/Assets/'

# Paths passed to script
DEST_DIR="${1:-.}"
CACHE="${2:-${DEST_DIR}}"

# Set up for clean-up on exit
# NOTE THAT THIS INDISCRIMINATELY REMOVES *.tmp FROM DEST & CACHE DIRS
cleanup ()
{
  test "$?" = 0 || echo '[1;31mFAILED.[m'
  rm -f "$CACHE/"*.tmp "$DEST_DIR/"*.tmp
}
trap cleanup EXIT

# Create directories if needed
mkdir -p "$DEST_DIR" "$CACHE"

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
download ()
{
  echo "[0;33mDownloading $2 to $1...[m"
  curl --retry 5 -fLo "$1/$2.tmp" "$BASE_URL$2"
  mv -f "$1/$2.tmp" "$1/$2"
  echo '[32mDownloaded.[m'
}

# Get the md5sum checksums
download "$CACHE" md5sums

# Check that the file is properly formatted
echo '[33mVerifying md5sums integrity...[m'
grep -sqve '^[0-9a-f]\{32\}  [^ ]\+$' "$CACHE/md5sums" && exit 1
echo '[32mSuccessful.[m'

# List the files whose checksums are listed
echo '[33mFound files:[m'
cut -d' ' -f2- <"$CACHE/md5sums"

# Download those files unless the local copies match the checksums
(set -e; cd "$DEST_DIR"; LANG=C md5sum -c "$CACHE/md5sums" 2>/dev/null) |
  sed -e '/: OK$/ d; s/: FAILED\( open or read\)\?$//' |
  while read i; do
    download "$DEST_DIR" "$(basename "$i")"
  done

# All done :-)
echo '[1;32mFinished.[m'

exit 0
