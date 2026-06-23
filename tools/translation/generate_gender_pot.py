#!/usr/bin/env python3
# ===========================================================================
#
# Copyright (c) 2012 Unvanquished Developers
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

import re
import sys

def main():
    if len(sys.argv) < 2:
        print(f"{sys.argv[0]} <source file with gender context>")
        sys.exit(1)

    source_file = sys.argv[1]
    context = {}

    try:
        with open(source_file, 'r') as f:
            for linenum, line in enumerate(f, start=1):
                match = re.search(r'G_\((.*?)\)', line)
                if match:
                    context[linenum] = match.group(1)
    except OSError as e:
        print(e)
        sys.exit(1)

    print()

    for key in sorted(context.keys()):
        for gender in ("male", "female", "neuter"):
            print(f"#: {source_file}:{key}")
            print(f'msgctxt "{gender}"')
            print(f"msgid {context[key]}")
            print('msgstr ""\n')

if __name__ == "__main__":
    main()
