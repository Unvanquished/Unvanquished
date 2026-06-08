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

short_po_diff () {
	git diff --unified=0 --no-prefix -- "${1}" \
	| sed '/^diff --git/d;/^index /d;/^--- /d;/^\+\+\+ /d;/^@@ /d' \
	| grep -vE '^[-+]"POT-Creation-Date: '
}

error () {
	echo "ERROR: ${1}" >&2
	false
}

if [ -z "${1:-}" ]
then
	error 'missing file'
fi

if ! [ -f "${1}" ]
then
	error "not a file: ${1}"
fi

# Denoise the diff only if the file is already tracked by Git.
if git ls-files --error-unmatch "${1}" >/dev/null 2>&1
then
	# Revert the file if only the POT date changed.
	if [ -z "$(short_po_diff "${1}")" ]
	then
		git checkout "${1}" >/dev/null 2>&1
	fi
fi
