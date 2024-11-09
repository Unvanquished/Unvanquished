#! /usr/bin/env bash

# ===========================================================================
#
# Copyright (c) 2024 Unvanquished Developers
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

action_file="${dpk_dir}/.urcheon/action/build.txt"
action_list="$(grep -v -E '^ignore translation/' "${action_file}")"
echo "${action_list}" > "${action_file}"

cd "${pot_dir}"

for name in 'game'
do
	for po_file in $(find "${name}" -name '*.po' | sort)
	do
		lang="$(echo "${po_file}" | cut -f2 -d'/' | cut -f1 -d'.')"

		po_entries="$(sed 'N;s/ ""\n"/ "/g;P;D;' "${po_file}" | grep -E '^msgid "|^msgstr "')"

		count_source="$(echo "${po_entries}" | grep -c -E '^msgid "' || true)"
		count_translation="$(echo "${po_entries}" | grep -c -E '^msgstr "' || true)"
		count_empty_source="$(echo "${po_entries}" | grep -c -E '^msgid ""' || true)"
		count_empty_translation="$(echo "${po_entries}" | grep -c -E '^msgstr ""' || true)"

		total_source="$((${count_source} - ${count_empty_source}))"
		total_translation="$((${count_translation} - ${count_empty_translation}))"

		percentage="$((${total_translation} * 100 / ${total_source}))"

		if [ "${percentage}" -lt 50 ]
		then
			printf 'ignore translation/%s\n' "${po_file}" >> "${action_file}"
		fi

		printf '%s/%s\t%+3s%%,\tsources: %s (%s-%s),\ttranslations: %-4s (%s-%s)\n' \
			"${name}" "${lang}:" "${percentage}" \
			"${total_source}" "${count_source}" "${count_empty_source}" \
			"${total_translation}" "${count_translation}" "${count_empty_translation}"
	done
done | column -t -s $'\t' -o ' '
