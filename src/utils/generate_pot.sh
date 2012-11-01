#!/bin/sh
SELF="$(dirname "$0")"
cd "$SELF"
SELF="$PWD"
cd ../..

find src/engine/server/ src/engine/client/ src/engine/qcommon/ | grep .c[p]*$ | xgettext --from-code=UTF-8 -o messages_client.pot -k_ -kN_ -kC_:2 -f -
touch messages_game.pot
for i in main/ui/*.menu
do
	src/utils/generate_menu_pot.pl $i | xgettext --from-code=UTF-8 -a --no-location -C -c -j -o messages_game.pot -
done

src/utils/gender_context.pl src/gamelogic/cgame/cg_event.c >> messages_game.pot

find src/gamelogic/ | grep .c$ | xgettext --from-code=UTF-8 -j -o messages_game.pot -k_ -kN_ -k -f -

find main/ui/ | grep .menu$ | xgettext --from-code=UTF-8 -C -o messages_game.pot -j \
-kCVAR:4:1 \
-kMULTI:5:1 \
-kCOMBO:3:1 \
-kSLIDER:6:1 \
-kYESNO:4:1 \
-kMMBUTTON \
-kCVAR:5:2 \
-kMULTI:6:2 \
-kMULTIX:2 \
-kCOMBO:4:2 \
-kCOMBOX:2 \
-kSLIDER:7:2 \
-kYESNO:5:2 \
-kBUTTON:4:2 \
-kBUTTONX:2 \
-kBIND:2 \
-kBINDX:2 \
-kSLIDERX:2 \
-kTEXTX:2 \
-f -
