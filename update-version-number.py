#!/usr/bin/env python
"""Updates the Unvanquished/Daemon version number.

Compatible with Python 2 or 3.
"""


# (Filename relative to Unvanquished/, regex pattern to match, replacement)
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
    (
        "download-paks",
        r"current_unvanquished_version='[^']*'",
        r"current_unvanquished_version='{version}'"
    ),
    # This could also go in MAJOR_REPLACEMENTS as it should already be false in a minor release
    (
        "daemon/src/common/IPC/Common.h",
        r"DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES = (true|false)",
        r"DAEMON_HAS_COMPATIBILITY_BREAKING_SYSCALL_CHANGES = false"
    ),
]

MAJOR_REPLACEMENTS = [
    (
        "daemon/src/common/IPC/Common.h",
        r'SYSCALL_ABI_VERSION = "[^"]*"',
        r'SYSCALL_ABI_VERSION = "{version}"'
    ),
]

import argparse
import difflib
import os
import re


def update_version(version, majorness, dry_run):
    changes = {} # filename -> [original content, new content]
    root = os.path.dirname(__file__)
    replacements = {
        "minor": REPLACEMENTS,
        "major": REPLACEMENTS + MAJOR_REPLACEMENTS,
    }[majorness]

    for filename, pattern, replacement in replacements:
        filename = os.path.join(root, filename)
        if filename not in changes:
            with open(filename) as f:
                content = f.read()
            changes[filename] = [content, content]
        if not re.search(pattern, changes[filename][1]):
            raise Exception("Replacement for %s matched nothing" % filename)
        changes[filename][1] = re.sub(
            pattern, replacement.format(version=version), changes[filename][1])

    if dry_run:
        for filename, (old, new) in changes.items():
            diff = difflib.unified_diff(
                old.splitlines(), new.splitlines(), filename, filename, lineterm="")
            for line in diff:
                print(line)
    else:
        for filename, (_, contents) in changes.items():
            with open(filename, "w") as f:
                f.write(contents)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Update Unvanquished and Daemon version numbers")
    parser.add_argument("version")
    parser.add_argument("minorOrMajor", choices=["minor", "major"])
    parser.add_argument("-d", "--dry-run", action="store_true", help="Show diffs instead of writing files")
    args = parser.parse_args()
    update_version(args.version, args.minorOrMajor, args.dry_run)
