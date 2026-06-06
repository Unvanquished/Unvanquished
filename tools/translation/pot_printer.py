#!/usr/bin/env python3

import argparse
import collections
import datetime
import sys

def load(filename):
    file = open(filename, "r", encoding="utf-8")
    content = file.read()
    file.close()
    return content

def run(process):
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", action="store_true", help="sort strings")
    parser.add_argument("-d", action="store_true", help="print debug information")
    parser.add_argument("file", nargs="*")
    argv = parser.parse_args()

    translation_dict = collections.defaultdict(list)
    errors = 0

    files = argv.file

    if not files:
        files = [ sys.stdin ]

    for filename in files:
        if filename == sys.stdin:
            file = filename
            content = file.read()
            filename = file.name
        else:
            content = load(filename)

        errors += process(translation_dict, content, filename, argv.d)

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
        print("msgid \"" + "\\n\"\n\"".join(translatable_string.split("\n")) + "\"")
        print("msgstr \"\"")

    if errors > 0:
        print(errors, "markup error(s) found", file=sys.stderr)
        exit(1)
