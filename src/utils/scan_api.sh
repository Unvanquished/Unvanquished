#! /bin/sh
SELF="$(dirname "$0")"
cd "$SELF"
SELF="$PWD"
cd ../..
echo 'Generating UI API asm file...'
perl "$SELF/scan_api.pl" src/gamelogic/ui/ui_api.c    src/engine/client/ui_api.h >src/gamelogic/ui/ui_api.asm
echo 'Generating client game API asm file...'
perl "$SELF/scan_api.pl" src/gamelogic/cgame/cg_api.c src/engine/client/cg_api.h >src/gamelogic/cgame/cg_api.asm
echo 'Generating server game API asm file...'
perl "$SELF/scan_api.pl" src/gamelogic/game/g_api.c   src/engine/server/g_api.h  >src/gamelogic/game/g_api.asm
echo 'Done.'
