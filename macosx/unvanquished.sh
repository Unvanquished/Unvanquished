#!/bin/bash
APP_BUNDLE="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "${APP_BUNDLE}/Contents/MacOS"
exec "${APP_BUNDLE}/Contents/MacOS/daemon" +set fs_basepath "${APP_BUNDLE}/Contents/Resources" +set fs_libpath "${APP_BUNDLE}/Contents/MacOS" "$@"
