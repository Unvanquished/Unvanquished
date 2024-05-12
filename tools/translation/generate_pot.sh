#! /usr/bin/env bash

# ===========================================================================
#
# Copyright (c) 2017-2024 Unvanquished Developers
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

set -u -e -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
repo_dir="$(realpath "${script_dir}/../..")"
dpk_dir="${repo_dir}/pkg/unvanquished_src.dpkdir"
pot_dir="${dpk_dir}/translation"

. "${script_dir}/translation.conf"

generate_game_pot () (
	(
		cd "${dpk_dir}"

		find -type f -name '*.rml' \
		| sort \
		| xargs -I'{}' \
			"${script_dir}/generate_rml_pot.py" '{}' \
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

for name in 'game' 'commands'
do
	if eval "\${${name}}"
	then
		temp_pot_file="$(mktemp)"

		"generate_${name}_pot"

		pot_file="${pot_dir}/${name}.pot"

		mkdir -p "${pot_dir}"

		mv "${temp_pot_file}" "${pot_file}"
	fi
done
