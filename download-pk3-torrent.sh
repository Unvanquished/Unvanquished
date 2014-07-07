#!/bin/sh
# Donwloads Unvanquished assets from torrent via aria2c.
# Requires: aria2c, grep, awk, tr, readlink, ls, mkdir
# Usage: $0 <target directory> <cache directory>

############
# SETTINGS #
############

if [ $# -gt 2 ]; then
    torrent_file="unvanquished_${3}.torrent"
else
    torrent_file="current.torrent"
fi

torrent_url=http://unvnet.net/unv-launcher/$torrent_file

default_cache_dir=/tmp/unv-paks-cache
last_assets_file=last-assets.txt

############

set -e

# check usage
if [ $# -lt 1 ]; then
    echo "Usage: $0 <target directory> [cache directory] [version]"
    exit
fi

# retrieve target directory
target_dir=$(readlink -f "$1")
if [ ! -d "$target_dir" ]; then
    echo "Not a directory: $target_dir"
    exit
fi

# retrieve cache directory
if [ $# -gt 1 ]; then
    cache_dir=$(readlink -f "$2")
else
    cache_dir="$default_cache_dir"
    mkdir -p "$cache_dir"
fi
if [ ! -d "$cache_dir" ]; then
    echo "Not a directory: $cache_dir"
    exit
fi

# enter cache subdirectory
cd "$cache_dir"

# download torrent file
echo "Downloading torrent..."
aria2c \
    --follow-torrent=false \
    --allow-overwrite=true \
    "$torrent_url" -d "$cache_dir"
echo

# get the contained asset path
path=$(aria2c -S "$torrent_file"|grep '/pkg/unvanquished_.*\.pk3'|head -1|awk -F'/' '{print $2}')

# delete old torrent directories
for dir in $(ls -c1|tr -d '/'); do
    if [ "$dir" != "$path" ] && [ -d "$dir" ]; then
        rm -r "$dir";
    fi
done

# symlink the extraction path to the target so the files land in the right place
if [ ! -d "$path" ]; then
    mkdir "$path" || exit
fi
rm -f "$path/pkg"
ln -s "$target_dir" "$path/pkg"

# build a comma seperated list of assets and their ids
asset_lst=$(aria2c -S "$torrent_file"|grep '.*/pkg/.*\.pk3'|awk -F'|' '{print $2}'|grep -o '[^/]*$')
asset_ids=$(aria2c -S "$torrent_file"|grep '.*/pkg/.*\.pk3'|awk -F'|' '{print $1}'|tr '\n' ','|grep -o '[0-9].*[0-9]')

# download assets
echo "Downloading assets..."
aria2c \
    --summary-interval=0 \
    --check-integrity=true \
    --seed-time=0 \
    --select-file="$asset_ids" \
    -T "$torrent_file" -d "$cache_dir"
echo

# delete all previously downloaded assets that aren't in this torrent
if [ -f "$last_assets_file" ]; then
    for asset in $(cat "$last_assets_file"); do
        if ! echo "$asset_lst"|grep -q "^$asset$"; then
            if [ -f "$target_dir/$asset" ];then
                echo "Deleting old file $asset."
                rm "$target_dir/$asset"
            fi
        fi
    done
fi

# save list of downloaded assets
echo "$assets_lst" > "$last_assets_file"

