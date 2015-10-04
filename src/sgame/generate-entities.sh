#!/bin/sh

set -e

cd $(dirname "$0")

# relative path to code generator
generator_name="CBSE.py"
generator="../utils/cbse/${generator_name}"

# (absolute pathes to) generator arguments
output_dir="$(pwd)"
definitions="${output_dir}/entities.yaml"

cd $(dirname "${generator}")

"./${generator_name}" -s -o "${output_dir}" "${definitions}"

