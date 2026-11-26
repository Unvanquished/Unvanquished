#! /usr/bin/env bash

# ===========================================================================
#
# Copyright (c) 2024-2026 Unvanquished Developers
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

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
. "${script_dir}/common.sh"

parse_args "${@}"
set_paths

cd "${pot_dir}"

for name in "${translations[@]}"
do
	pot_file="${pot_dir}/${name}.pot"

	if [ -f "${pot_file}" -a -d "${name}" ]
	then
		find "${name}" -type f -name '*.po' -print0 \
		| xargs -0 -r -I'{}' -P"$(nproc)" \
			msgmerge --no-fuzzy-matching -o {} {} "${pot_file}"

		for po_file in $(find "${name}" -type f -name '*.po')
		do
			"${script_dir}/denoise_po_diff.sh" "${po_file}"
		done
	fi
done
