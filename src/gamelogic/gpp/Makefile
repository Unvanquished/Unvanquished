# Author: Zoltán Kéri <k.z0lik4@gmail.com>
# Heavily rewritten by Darren Salt

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

ifeq ($(VERBOSE),)
    ECHO := @echo
    ECHOCOMPILE = @echo "   LLVMCC" $@
    ECHOLINK = @echo "   LLVMLD" $@
    AT := @
else
    ECHO := @:
    ECHOCOMPILE := @
    ECHOLINK := @
    AT :=
endif

# CCFLAGS := -w -DNDEBUG -O4 -emit-llvm -S
CCFLAGS += -I. -I../../../src/engine/qcommon -Isrc/game -Isrc/cgame -Isrc/ui

GAMEFLAGS  := -DGAMEDLL
CGAMEFLAGS := -DCGAMEDLL -DCGAME
UIFLAGS    := -DUIDLL

# Main targets

all pk3: vms-unv.pk3

vms-unv.pk3: bitcode
	zip -9 vms-unv.pk3 vm/*.bc

bitcode: vm/ui.bc # vm/game.bc vm/cgame.bc

vm/cgame.bc vm/game.bc vm/ui.bc: vm/%.bc: vm vm/%_32.bc vm/%_64.bc

vm/cgame_32.bc vm/game_32.bc vm/ui_32.bc: ARCHFLAG = -m32
vm/cgame_32.bc vm/game_32.bc vm/ui_32.bc: APPEND = _32

vm/cgame_64.bc vm/game_64.bc vm/ui_64.bc: ARCHFLAG = -m64
vm/cgame_64.bc vm/game_64.bc vm/ui_64.bc: APPEND = _64

# Sources

build/q_shared_32.game.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_shared_64.game.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_shared_32.cgame.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_shared_64.cgame.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_shared_32.ui.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_shared_64.ui.bc: ../../../src/engine/qcommon/q_shared.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_32.game.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_64.game.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_32.cgame.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_64.cgame.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_32.ui.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/q_math_64.ui.bc: ../../../src/engine/qcommon/q_math.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_32.game.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_64.game.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_32.cgame.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_64.cgame.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_32.ui.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

build/%_64.ui.bc: %.c
	$(ECHOCOMPILE)
	$(AT)$(CC) $(CCFLAGS) $(ARCHFLAG) $(TARGETFLAGS) $< -o $@

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

GAME_OBJS_32 := $(addprefix build/,$(subst ../../../src/engine/qcommon/,,$(GAME_SOURCES:.c=_32.game.bc)))
GAME_OBJS_64 := $(subst _32,_64,$(GAME_OBJS_32))

vm/game_32.bc vm/game_64.bc: TARGETFLAGS=$(GAMEFLAGS)

vm/game_32.bc: build vm $(GAME_OBJS_32)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(GAME_OBJS_32) -o $@

vm/game_64.bc: build vm $(GAME_OBJS_64)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(GAME_OBJS_64) -o $@

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

CGAME_OBJS_32 := $(addprefix build/,$(subst ../../../src/engine/qcommon/,,$(CGAME_SOURCES:.c=_32.cgame.bc)))
CGAME_OBJS_64 := $(subst _32,_64,$(CGAME_OBJS_32))

vm/cgame_32.bc vm/cgame_64.bc: TARGETFLAGS=$(CGAMEFLAGS)

vm/cgame_32.bc: vm $(CGAME_OBJS_32)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(CGAME_OBJS_32) -o $@

vm/cgame_64.bc: vm $(CGAME_OBJS_64)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(CGAME_OBJS_64) -o $@

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

UI_OBJS_32 := $(addprefix build/,$(subst ../../../src/engine/qcommon/,,$(UI_SOURCES:.c=_32.ui.bc)))
UI_OBJS_64 := $(subst _32,_64,$(UI_OBJS_32))

vm/ui_32.bc vm/ui_64.bc: TARGETFLAGS=$(UIFLAGS)

vm/ui_32.bc: vm $(UI_OBJS_32)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(UI_OBJS_32) -o $@

vm/ui_64.bc: vm $(UI_OBJS_64)
	$(ECHOLINK)
	$(AT)$(SHLINK) $(UI_OBJS_64) -o $@

vm: build
	$(ECHO) "   MKDIR  vm"
	$(AT)mkdir -p vm

$(GAME_OBJS) $(CGAME_OBJS) $(UI_OBJS): build

build:
	$(ECHO) "   MKDIR  build tree"
	$(AT)mkdir -p $(dir $(GAME_OBJS_32) $(CGAME_OBJS_32) $(UI_OBJS_32))

clean:
	$(ECHO) Removing "build" directory from ./
	$(AT)rm -rf build/
	$(ECHO) Removing "*.bc" files
	$(AT)find -name '*.bc' -delete
	$(AT)rm -rf vm
