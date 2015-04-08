#!/bin/bash
APP_BUNDLE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PARENT_DIR="$(cd "${APP_BUNDLE}/.." && pwd)"
cd "${APP_BUNDLE}/Contents/MacOS"
exec "${APP_BUNDLE}/Contents/MacOS/daemon" -pakpath "${PARENT_DIR}/pkg" "$@"
