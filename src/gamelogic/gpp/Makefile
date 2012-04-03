# Author: Zoltán Kéri <k.z0lik4@gmail.com>
# Heavily rewritten by Darren Salt

all pk3: vms-unv.pk3

bitcode: all32 all64

CC       := clang
SHLINK   := llvm-link
MARCH    := $(shell uname -m)
PLATFORM := $(shell uname -s)

ECHO     := /bin/echo

ifndef DEBUG
    DEBUG := false
endif

ifeq ($(DEBUG), true)
    CCFLAGS := -Wall -D_DEBUG -DDEBUG -g -O0 -emit-llvm -c
else
    CCFLAGS := -w -DNDEBUG -DQ3_VM -g -O4 -emit-llvm -nostdinc -S
endif

ifeq ($(ARCH), 32)
    CCFLAGS += -m32
    APPEND = _32
endif
ifeq ($(ARCH), 64)
    CCFLAGS += -m64
    APPEND = _64
endif

# CCFLAGS := -w -DNDEBUG -O4 -emit-llvm -S
CCFLAGS += -I. -I../../../src/engine/qcommon -Isrc/game -Isrc/cgame -Isrc/ui

GAMEFLAGS  := -DGAMEDLL
CGAMEFLAGS := -DCGAMEDLL -DCGAME
UIFLAGS    := -DUIDLL

vms-unv.pk3: bitcode
	zip -9 vms-unv.pk3 *.bc

all32: build
	$(MAKE) ARCH=32 .all
all64: build
	$(MAKE) ARCH=64 .all

build:
	mkdir -p build

.all: ui$(APPEND).bc # game$(APPEND).bc cgame$(APPEND).bc

build/q_shared$(APPEND).game.bc: ../../../src/engine/qcommon/q_shared.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

build/q_shared$(APPEND).cgame.bc: ../../../src/engine/qcommon/q_shared.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

build/q_shared$(APPEND).ui.bc: ../../../src/engine/qcommon/q_shared.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

build/q_math$(APPEND).game.bc: ../../../src/engine/qcommon/q_math.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

build/q_math$(APPEND).cgame.bc: ../../../src/engine/qcommon/q_math.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

build/q_math$(APPEND).ui.bc: ../../../src/engine/qcommon/q_math.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

%$(APPEND).game.bc: %.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

%$(APPEND).cgame.bc: %.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

%$(APPEND).ui.bc: %.c
	@$(ECHO) Compiling: $<
	@$(CC) $(CCFLAGS) $(TARGETFLAGS) $< -o $@

GAME_SOURCES := \
	src/game/g_main.c \
	src/game/bg_alloc.c \
	src/game/bg_lib.c \
	src/game/bg_misc.c \
	src/game/bg_pmove.c \
	src/game/bg_slidemove.c \
	src/game/bg_voice.c \
	src/game/g_active.c \
	src/game/g_admin.c \
	src/game/g_buildable.c \
	src/game/g_client.c \
	src/game/g_cmds.c \
	src/game/g_combat.c \
	src/game/g_maprotation.c \
	src/game/g_misc.c \
	src/game/g_missile.c \
	src/game/g_mover.c \
	src/game/g_namelog.c \
	src/game/g_physics.c \
	src/game/g_session.c \
	src/game/g_spawn.c \
	src/game/g_svcmds.c \
	src/game/g_target.c \
	src/game/g_team.c \
	src/game/g_trigger.c \
	src/game/g_utils.c \
	src/game/g_weapon.c \
	../../../src/engine/qcommon/q_math.c \
	../../../src/engine/qcommon/q_shared.c \
	src/game/g_api.c

GAME_OBJS := $(subst ../../../src/engine/qcommon/,build/,$(GAME_SOURCES:.c=$(APPEND).game.bc))

game$(APPEND).bc: TARGETFLAGS=$(GAMEFLAGS)
game$(APPEND).bc: TARGET=.game
game$(APPEND).bc: $(GAME_OBJS)
	@$(ECHO) Linking: $@
	@$(SHLINK) $(GAME_OBJS) -o $@

CGAME_SOURCES := \
	src/cgame/cg_main.c \
	src/game/bg_alloc.c \
	src/game/bg_lib.c \
	src/game/bg_misc.c \
	src/game/bg_pmove.c \
	src/game/bg_slidemove.c \
	src/game/bg_voice.c \
	src/cgame/cg_animation.c \
	src/cgame/cg_animmapobj.c \
	src/cgame/cg_attachment.c \
	src/cgame/cg_buildable.c \
	src/cgame/cg_consolecmds.c \
	src/cgame/cg_draw.c \
	src/cgame/cg_drawtools.c \
	src/cgame/cg_ents.c \
	src/cgame/cg_event.c \
	src/cgame/cg_marks.c \
	src/cgame/cg_particles.c \
	src/cgame/cg_players.c \
	src/cgame/cg_playerstate.c \
	src/cgame/cg_predict.c \
	src/cgame/cg_scanner.c \
	src/cgame/cg_servercmds.c \
	src/cgame/cg_snapshot.c \
	src/cgame/cg_trails.c \
	src/cgame/cg_tutorial.c \
	src/cgame/cg_view.c \
	src/cgame/cg_weapons.c \
	src/ui/ui_shared.c \
	../../../src/engine/qcommon/q_math.c \
	../../../src/engine/qcommon/q_shared.c \
	src/cgame/cg_api.c

CGAME_OBJS := $(subst ../../../src/engine/qcommon/,build/,$(CGAME_SOURCES:.c=$(APPEND).cgame.bc))

cgame$(APPEND).bc: TARGETFLAGS=$(CGAMEFLAGS)
cgame$(APPEND).bc: TARGET=.cgame
cgame$(APPEND).bc: $(CGAME_OBJS)
	@$(ECHO) Linking: $@
	@$(SHLINK) $(CGAME_OBJS) -o $@

UI_SOURCES := \
	src/ui/ui_main.c \
	src/ui/ui_atoms.c \
	src/ui/ui_shared.c \
	src/ui/ui_gameinfo.c \
	src/game/bg_misc.c \
	src/game/bg_lib.c \
	../../../src/engine/qcommon/q_math.c \
	../../../src/engine/qcommon/q_shared.c \
	src/ui/ui_api.c

UI_OBJS := $(subst ../../../src/engine/qcommon/,build/,$(UI_SOURCES:.c=$(APPEND).ui.bc))

ui$(APPEND).bc: TARGETFLAGS=$(CGAMEFLAGS)
ui$(APPEND).bc: TARGET=.ui
ui$(APPEND).bc: $(UI_OBJS)
	@$(ECHO) Linking: $@
	@$(SHLINK) $(UI_OBJS) -o $@

clean:
	@$(ECHO) Removing "build" directory from ./
	@rm -rf build/
	@$(ECHO) Removing "*.bc" files
	@find -name '*.bc' -delete
