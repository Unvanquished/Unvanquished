#!/bin/sh
SELF="$(dirname "$0")"
cd "$SELF"
SELF="$PWD"
cd ../..

find src/engine/server/ src/engine/client/ src/engine/qcommon/ -name '*.c' -o -name '*.cpp' | sort | xgettext --from-code=UTF-8 -o messages_client.pot -k_ -kN_ -kC_:2 -f -
touch messages_game.pot
find main/ -name '*.menu' | sort | while read i
do
	src/utils/generate_menu_pot.pl "$i" | xgettext --from-code=UTF-8 -a --no-location -C -c -j -o messages_game.pot -
done

# generate_menu_pot.pl writes file references as code comments (#. file:line)
# but we want them as gettext file references (#: file:line)
sed 's/^#\. \(.\+:[0-9]\+\)$/#: \1/' < messages_game.pot > messages_game.pot.tmp
mv messages_game.pot.tmp messages_game.pot

src/utils/gender_context.pl src/gamelogic/cgame/cg_event.c >> messages_game.pot

find src/gamelogic/ -name '*.c' | sort | xgettext --from-code=UTF-8 -j -o messages_game.pot -k_ -kN_ -k -f -

find main/ui/ -name '*.menu' | sort | xgettext --from-code=UTF-8 -C -o messages_game.pot -j \
-kCVAR:1,4t \
-kMULTI:1,5t \
-kCOMBO:1,3t \
-kSLIDER:1,6t \
-kYESNO:1,4t \
-kMMBUTTON \
-kCVAR:2,5t \
-kCVAR_INT:2,5t \
-kMULTI:2,6t \
-kMULTIX:2 \
-kCOMBO:2,4t \
-kCOMBOX:2 \
-kSLIDER:2,7t \
-kYESNO:2,5t \
-kBUTTON:2,4t \
-kBUTTONX:2 \
-kBIND:2 \
-kBINDX:2 \
-kSLIDERX:2 \
-kTEXTX:2 \
-kVSAY_ITEM:1 \
-kYESNOX:2 \
-kSAY_ITEM:2 \
-f -
