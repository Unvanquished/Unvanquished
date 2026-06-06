#!/usr/bin/env python3

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
