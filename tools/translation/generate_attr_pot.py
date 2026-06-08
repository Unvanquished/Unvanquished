#!/usr/bin/env python3

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

import re
import sys

import pot_printer

def process(translation_dict, content, filename, is_debug):
    offset = 0
    line = 1
    errors = 0

    def error(msg):
        print(f"Error in file {filename} on line {line}: {msg}", file=sys.stderr)
        nonlocal errors
        errors += 1

    for m in re.finditer(r'\b(description|humanName)\s*"([^"]+)"', content, re.IGNORECASE):
        line += content.count("\n", offset, m.start(2))
        offset = m.start(2) # beginning of string
        string = m.group(2)
        if string.endswith("\\"):
            # There is a wacky COM_Parse semantic where the \" escape sequence is also a toggle switch
            # that toggles whether a double quote ends the string. Not bothering to emulate this
            error(r"""'\"' encountered: COM_Parse 'string-in-string' parsing not implemented""")
        elif string != "null":
            translation_dict[string].append(f"{filename}:{line}")

    return errors

pot_printer.run(process)
