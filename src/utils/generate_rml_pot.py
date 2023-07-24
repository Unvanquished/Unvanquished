#!/usr/bin/env python3

import argparse
import collections
import datetime
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-s", action="store_true", help="sort strings")
parser.add_argument("file", nargs="*")
argv = parser.parse_args()

translation_dict = collections.defaultdict(list)
errors = 0

def process_file(rml_file):
    global translation_dict
    rml_content = rml_file.read()
    rml_file.close()
    errors = 0
    def error(msg):
        print(f"Error in file {rml_file.name} on line {line}: {msg}", file=sys.stderr)
        global errors
        errors += 1
    text_start = None
    line = 1
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
                translation_dict[content].append(f"{rml_file.name}:{line}")
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

if argv.file:
    for filename in argv.file:
        process_file(open(filename))
else:
    process_file(sys.stdin)

datetime_string = datetime.datetime.now().astimezone().strftime("%Y-%m-%d %H:%M%z")
print(rf'''# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR THE PACKAGE'S COPYRIGHT HOLDER
# This file is distributed under the same license as the PACKAGE package.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid ""
msgstr ""
"POT-Creation-Date: {datetime_string}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"''')

if argv.s:
    translation_dict = dict(sorted(translation_dict.items()))
for translatable_string, filenames in translation_dict.items():
    translatable_string = translatable_string.replace("\"", "\\\"")
    print()
    print("#: " + " ".join(filenames))
    print("msgid \"" + r"\\n\"\n\"".join(translatable_string.split("\n")) + "\"")
    print("msgstr \"\"")

if errors > 0:
    print(errors, "markup error(s) found", file=sys.stderr)
    exit(1)
