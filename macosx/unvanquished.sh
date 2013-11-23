#!/bin/bash
APP_BUNDLE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PARENT_DIR="$(cd "${APP_BUNDLE}/.." && pwd)"
cd "${APP_BUNDLE}/Contents/MacOS"
exec "${APP_BUNDLE}/Contents/MacOS/daemon" +set fs_basepath "${PARENT_DIR}" +set fs_libpath "${APP_BUNDLE}/Contents/MacOS" "$@"
