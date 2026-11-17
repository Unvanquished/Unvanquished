#!/usr/bin/env python3

# ===========================================================================
#
# Copyright (c) 2021-2026 Unvanquished Developers
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

def process(translation_dict, rml_content, filename, is_debug):
    line = 1
    errors = 0

    def error(msg):
        print(f"Error in file {filename} on line {line}: {msg}", file=sys.stderr)
        nonlocal errors
        errors += 1

    text_start = None
    line_count_position = 0

    for m in re.finditer("<(/?)translate([^>]*)>", rml_content, re.DOTALL | re.IGNORECASE):
        line += rml_content.count("\n", line_count_position, m.start())
        line_count_position = m.start()

        if m.group(1) == '/': # closing tag
            if text_start is None:
                error('unmatched </translate>')
            else:
                content = rml_content[text_start:m.start()]
                if content != content.strip():
                    error("leading or trailing whitespace in translated string")
                if "\n" in content:
                    error("translated string should not contain newlines")
                translation_dict[content].append(f"{filename}:{line}")
                text_start = None

        else:
            if text_start is not None:
                error("multiple consecutive <translate> opening tags")

            text_start = m.end()

            if not set(m.group(2).split()) <= {"quake", "emoticons"}:
                error("unexpected attribute in <translate>")

    if text_start is not None:
        error("<translate> open at EOF")

    return errors

pot_printer.run(process)
