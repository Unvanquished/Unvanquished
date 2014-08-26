#! /bin/sh
SELF="$(dirname "$0")"
cd "$SELF"
SELF="$PWD"
cd ../..
echo 'Generating client game API asm file...'
perl "$SELF/scan_api.pl" src/gamelogic/cgame/cg_api.c src/engine/client/cg_api.h >src/gamelogic/cgame/cg_api.asm
echo 'Done.'
