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

cd "${pot_dir}"

printf '' > 'completion.txt'

# Only consider game translation for completeness.
for po_file in $(find 'game' -name '*.po')
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

	printf '%s\t%+3s%%,\tsources: %s (%s-%s),\ttranslations: %-4s (%s-%s)\n' \
		"${lang}:" "${percentage}" \
		"${total_source}" "${count_source}" "${count_empty_source}" \
		"${total_translation}" "${count_translation}" "${count_empty_translation}"

	printf '%s %s\n' "${percentage}" "${lang}" >> "completion.txt"

done | column -t -s $'\t' -o ' '
