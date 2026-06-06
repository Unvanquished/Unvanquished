#!/usr/bin/env python3

import os.path

exec(open(os.path.join(os.path.dirname(os.path.realpath(__file__)), "pot_printer.py")).read())

import re

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

run()
