#!/bin/sh

set -e

cd $(dirname "$0")

# relative path to code generator
generator="../../utils/cbse/CBSE.py"

# (absolute pathes to) generator arguments
output_dir="$(pwd)"
definitions="${output_dir}/entities.yaml"

cd $(dirname "${generator}")

"${generator}" -s -o "${output_dir}" "${definitions}"

