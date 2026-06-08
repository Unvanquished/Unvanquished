#! /usr/bin/env bash

# ===========================================================================
#
# Copyright (c) 2017-2026 Unvanquished Developers
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# ===========================================================================

known_options=('data_dir')

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
. "${script_dir}/common.sh"

parse_args "${@}"
set_paths

generate_data_pot () (
	if [ -z "${option_data_dir:-}" ]
	then
		error "missing data directory"
	fi

	if [ ! -d "${option_data_dir}" ]
	then
		error "not a directory: ${option_data_dir}"
	fi

	for pak_dir in $(find "${option_data_dir}/pkg" -type d -name 'map-*.dpkdir')
	do
	(
		cd "${pak_dir}"

		find maps -type f -name '*.map' \
		| sort \
		| xargs -I{} \
			"${script_dir}/generate_map_pot.py" {} \
			>> "${temp_pot_file}"

		find meta -type f -name '*.arena' \
		| sort \
		| xargs -I{} \
			"${script_dir}/generate_arena_pot.py" {} \
			>> "${temp_pot_file}"
	)
	done

	# HACK: Let xgettext reprocess the file to deduplicate comments.
	echo '' | xgettext --from-code=UTF-8 \
		-j -o "${temp_pot_file}" \
		-f -
)

generate_game_pot () (
	(
		cd "${dpk_dir}"

		find -type f -name '*.rml' \
		| sort \
		| xargs -I'{}' \
			"${script_dir}/generate_rml_pot.py" '{}' \
			>> "${temp_pot_file}"

		find -type f -name '*.attr.cfg' \
		| sort \
		| xargs -I'{}' \
			"${script_dir}/generate_attr_pot.py" '{}' \
			>> "${temp_pot_file}"
	)

	"${script_dir}/generate_gender_pot.pl" \
		'src/cgame/cg_event.cpp' \
		>> "${temp_pot_file}"

	find 'src/cgame' 'src/sgame' 'src/shared' \
		-name '*.cpp' \
		-a ! -name sg_admin.cpp \
		-a ! -name sg_cmds.cpp \
		-a ! -name sg_maprotation.cpp \
	| sort \
	| xgettext --from-code=UTF-8 \
		-j -o "${temp_pot_file}" \
		-k_ -kN_ -kP_:1,2 -k -f -
)

generate_commands_pot () (
	xgettext --from-code=UTF-8 \
		-o "${temp_pot_file}" \
		-k_ -kN_ -kP_:1,2 -k \
		src/sgame/sg_admin.cpp \
		src/sgame/sg_cmds.cpp \
		src/sgame/sg_maprotation.cpp
)

cd "${repo_dir}"

for name in "${translations[@]}"
do
	temp_pot_file="$(mktemp)"

	"generate_${name}_pot"

	pot_file="${pot_dir}/${name}.pot"

	mkdir -p "${pot_dir}"

	mv "${temp_pot_file}" "${pot_file}"

	"${script_dir}/denoise_po_diff.sh" "${pot_file}"
done
