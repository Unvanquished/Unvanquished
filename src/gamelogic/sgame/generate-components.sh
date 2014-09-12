#!/bin/sh

if [[ "$GEN" == "" ]];then
	echo "Usage: GEN=<path_to_generator> $0"
	exit -1
fi

base_dir="$(pwd)"
definition="${base_dir}/components.yaml"

cd $(dirname ${GEN}) || exit -2

${GEN} -o "${base_dir}" "${definition}"

