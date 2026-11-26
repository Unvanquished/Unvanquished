#! /usr/bin/env bash

# ===========================================================================
#
# Copyright (c) 2026 Unvanquished Developers
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

error () {
	echo "ERROR: ${1}" >&2
	false
}

parse_options () {
	if [[ -z ${options[@]} ]]
	then
		return
	fi

	if [[ -z ${known_options[@]} ]]
	then
		return
	fi

	local option
	for option in "${options[@]}"
	do
		case "${option}" in
			'--'*'='*)
				local option_name="$(echo "${option}" | cut -f1 -d= | cut -c3-)"
				local option_name="${option_name//-/_}"
				local option_value="$(echo "${option}" | cut -f2 -d=)"

				local found='false'
				for known_option in "${known_options[@]}"
				do
					if [ "${option_name}" = "${known_option}" ]
					then
						found='true'
					fi
				done

				if ! "${found}"
				then
					error "unknown option: ${option}"
				fi

				case "${option_name}" in
					*'_dir')
						case "${option_value}" in
							'~'*)
								option_value="${option_value/\~/${HOME}}"
							;;
						esac
					;;
				esac

				eval "option_${option_name}='${option_value}'"
			;;
			*)
				error "unknown option: ${option}"
			;;
		esac
	done
}

parse_args () {
	options=()
	translations=()

	while [ -n "${1:-}" ]
	do
		case "${1}" in
			-*)
				options+=("${1}")
				shift
			;;
			*)
				translations+=("${1}")
				shift
			;;
		esac
	done

	if [[ -z ${translations[@]} ]]
	then
		. "${script_dir}/translation.conf"
	fi

	parse_options
}

set_paths () {
	repo_dir="$(realpath "${script_dir}/../..")"
	dpk_dir="${repo_dir}/pkg/unvanquished_src.dpkdir"
	pot_dir="${dpk_dir}/translation"
}
