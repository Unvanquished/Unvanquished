#!/usr/bin/env python3

import os.path

exec(open(os.path.join(os.path.dirname(os.path.realpath(__file__)), "pot_printer.py")).read())

def process(translation_dict, content, filename, is_debug):
    line = 1
    errors = 0

    def error(msg):
        print(f"Error in file {filename} on line {line}: {msg}", file=sys.stderr)
        nonlocal errors
        errors += 1

    def debug(msg):
        if is_debug:
            print(f"Debug in file {filename} on line {line}: {msg}", file=sys.stderr)

    first_slash = False
    in_comment = False
    in_key = False
    in_value = False
    in_string = False
    key = ""
    value = ""
    string = ""

    for character in content:
        if character == '\n':
            line += 1

            if in_comment:
                in_comment = False

            if in_string:
                string += " "
                continue

            if in_key:
                debug(f"{key}")
                in_key = False
                key=""

            if in_value:
                debug(f"{key}={value}")
                in_value = False
                key = ""
                value = ""

            continue

        if in_comment:
            continue

        if character == '"':
            if not key:
                error("'\"' in key")
                continue

            if in_key:
                error("'\"' in key")
                continue

            if in_value:
                error("'\"' in value")
                continue

            if in_string:
                debug(f"{key}=\"{string}\"")

                if key in ["humanName", "description"]:
                    if string != "null":
                        translation_dict[string].append(f"{filename}:{line}")

                in_string = False
                key = ""
                string = ""
            else:
                in_string = True

            continue

        if in_string:
            string += character
            continue

        if character in [' ', '\t']:
            if first_slash:
                first_slash = False

            if in_key:
                in_key = False

            if in_value:
                debug(f"{key}={value}")
                in_value = False
                key = ""
                value = ""

            continue

        if character == '/':
            if first_slash:
                first_slash = False
                in_comment = True
            else:
                first_slash = True

            continue

        if in_key:
            key += character
            continue

        if in_value:
            value += character
            continue

        if not key:
            in_key = True
            key += character
            continue

        if not value:
            in_value = True
            value += character
            continue

    return errors

run()
