#! /bin/sh
cd "$(dirname $0)"/..
echo 'Generating UI API asm file...'
perl utils/scan_api.pl src/gamelogic/gpp/src/ui/ui_api.c    src/engine/client/ui_api.h >src/gamelogic/gpp/src/ui/ui_api.asm
echo 'Generating client game API asm file...'
perl utils/scan_api.pl src/gamelogic/gpp/src/cgame/cg_api.c src/engine/client/cg_api.h >src/gamelogic/gpp/src/cgame/cg_api.asm
echo 'Generating server game API asm file...'
perl utils/scan_api.pl src/gamelogic/gpp/src/game/g_api.c   src/engine/server/g_api.h  >src/gamelogic/gpp/src/game/g_api.asm
echo 'Done.'
