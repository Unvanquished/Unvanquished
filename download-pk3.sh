#! /bin/sh -e
# Retrieve Unvanquished resources from Sourceforge
# Arguments:
# 1. Destination directory
# 2. Cache file directory
# For system-wide installation, you probably want something like
# /usr/lib/games/unvanquished/main
# /var/cache/games/unvanquished

BASE_URL='http://downloads.sourceforge.net/project/unvanquished/Individual%20Game%20Pk3s/'
DEST_DIR="${1:-.}"
CACHE="${2:-${DEST_DIR}}"

mkdir -p "$DEST_DIR" "$CACHE"

# $1 = leafname
fetch ()
{
  # unlink old info if the target file is missing
  if ! test -f "$DEST_DIR/$1"; then
    rm -f "$CACHE/$1.data"
  fi
  echo "Checking whether $1 needs to be downloaded..."
  # retrieve information
  curl -LI "$BASE_URL$1" |
    grep -e '^\(Last-Modified\|Content-Length\):' |
    sort >"$CACHE/$1.data.new"
  # compare old with new, download if different/missing
  if ! cmp -s "$CACHE/$1.data" "$CACHE/$1.data.new" 2>/dev/null; then
    echo "Downloading $1..."
    curl -Lo "$DEST_DIR/$1" "$BASE_URL$1"
    mv -f "$CACHE/$1.data.new" "$CACHE/$1.data"
    echo 'Downloaded.'
  else
    rm -f "$CACHE/$1.data.new"
    echo 'No need -- already present.'
  fi
}

fetch pak0.pk3
fetch pak1.pk3
