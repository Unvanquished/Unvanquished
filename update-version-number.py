#!/usr/bin/env python
"""Updates the Unvanquished/Daemon version number.

Compatible with Python 2 or 3.
"""


# (Filename relative to Unvanquished/, regex pattern to match, replacement)
# More than one replacement in the same file is not supported
REPLACEMENTS = [
    (
        "daemon/src/common/Defs.h",
        r'(#define\s+PRODUCT_VERSION\s+)".+"',
        r'\1"{version}"'
    ),
    (
        "macosx/Info.plist",
        r"(<key>CFBundleVersion</key>\s*)<string>[^<]+</string>",
        r"\1<string>{version}</string>"
    ),
]


import argparse
import difflib
import os
import re


def update_version(version, dry_run):
    changes = {} # filename -> new contents or diff
    root = os.path.dirname(__file__)

    for filename, pattern, replacement in REPLACEMENTS:
        filename = os.path.join(root, filename)
        assert filename not in changes
        original_content = open(filename).read()
        if not re.search(pattern, original_content):
            raise Exception("Replacement for %s matched nothing" % filename)
        new_content = re.sub(pattern, replacement.format(version=version), original_content)
        if dry_run:
            changes[filename] = difflib.unified_diff(
                original_content.splitlines(), new_content.splitlines(),
                filename, filename, lineterm="")
        else:
            changes[filename] = new_content

    if dry_run:
        for diff in changes.values():
            for line in diff:
                print(line)
    else:
        for filename, contents in changes.items():
            with open(filename, "w") as f:
                f.write(contents)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Update Unvanquished and Daemon version numbers")
    parser.add_argument("version")
    parser.add_argument("-d", "--dry-run", action="store_true", help="Show diffs instead of writing files")
    args = parser.parse_args()
    update_version(args.version, args.dry_run)
