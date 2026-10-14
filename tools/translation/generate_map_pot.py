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
import subprocess
import sys

import pot_printer

def load(filename):
    return subprocess.run(
        ["esquirel", "map", "--input-map", filename, "--list-strings"],
        capture_output=True, text=True).stdout.strip()

def process(translation_dict, content, filename, is_debug):
    line = 0
    errors = 0

    def error(msg):
        print(f"Error in file {filename}#strings on line {line}: {msg}", file=sys.stderr)
        nonlocal errors
        errors += 1

    def debug(msg):
        if is_debug:
            print(f"Debug in file {filename}#strings on line {line}: {msg}", file=sys.stderr)

    lines = str(content).split('\n')

    pattern = re.compile("(?P<num>[0-9]*): (?P<string>.*)")

    for entry in lines:
        line += 1

        match = pattern.match(entry)
        if not match:
            error(f"malformed input: {entry}")
            continue

        num = match.group("num")
        string = match.group("string")
        debug(f"{num}: {string}")
        translation_dict[string].append(f"{filename}:{num}")

    return errors

pot_printer.run(process, load=load)
