################################################################################
# File lists
################################################################################

# Used by daemon and gamelogic
set( MINIZIPLIST
  ${LIB_DIR}/minizip/ioapi.c
  ${LIB_DIR}/minizip/ioapi.h
  ${LIB_DIR}/minizip/unzip.c
  ${LIB_DIR}/minizip/unzip.h
)

# Used by daemon
set( TINYGETTEXTLIST
  ${LIB_DIR}/tinygettext/dictionary_manager.hpp
  ${LIB_DIR}/tinygettext/file_system.hpp
  ${LIB_DIR}/tinygettext/iconv.cpp
  ${LIB_DIR}/tinygettext/plural_forms.hpp
  ${LIB_DIR}/tinygettext/tinygettext.cpp
  ${LIB_DIR}/tinygettext/tinygettext.hpp
  ${LIB_DIR}/tinygettext/dictionary.cpp
  ${LIB_DIR}/tinygettext/dictionary.hpp
  ${LIB_DIR}/tinygettext/dictionary_manager.cpp
  ${LIB_DIR}/tinygettext/iconv.hpp
  ${LIB_DIR}/tinygettext/language.cpp
  ${LIB_DIR}/tinygettext/language.hpp
  ${LIB_DIR}/tinygettext/log.cpp
  ${LIB_DIR}/tinygettext/log.hpp
  ${LIB_DIR}/tinygettext/log_stream.hpp
  ${LIB_DIR}/tinygettext/plural_forms.cpp
  ${LIB_DIR}/tinygettext/po_parser.cpp
  ${LIB_DIR}/tinygettext/po_parser.hpp
  ${LIB_DIR}/findlocale/findlocale.c
)

# Used by daemon on windows
set( PDCURSESLIST
  ${LIB_DIR}/pdcurses/pdcurses/addch.c
  ${LIB_DIR}/pdcurses/pdcurses/addchstr.c
  ${LIB_DIR}/pdcurses/pdcurses/addstr.c
  ${LIB_DIR}/pdcurses/pdcurses/attr.c
  ${LIB_DIR}/pdcurses/pdcurses/beep.c
  ${LIB_DIR}/pdcurses/pdcurses/bkgd.c
  ${LIB_DIR}/pdcurses/pdcurses/border.c
  ${LIB_DIR}/pdcurses/pdcurses/clear.c
  ${LIB_DIR}/pdcurses/pdcurses/color.c
  ${LIB_DIR}/pdcurses/pdcurses/debug.c
  ${LIB_DIR}/pdcurses/pdcurses/delch.c
  ${LIB_DIR}/pdcurses/pdcurses/deleteln.c
  ${LIB_DIR}/pdcurses/pdcurses/deprec.c
  ${LIB_DIR}/pdcurses/pdcurses/getch.c
  ${LIB_DIR}/pdcurses/pdcurses/getstr.c
  ${LIB_DIR}/pdcurses/pdcurses/getyx.c
  ${LIB_DIR}/pdcurses/pdcurses/inch.c
  ${LIB_DIR}/pdcurses/pdcurses/inchstr.c
  ${LIB_DIR}/pdcurses/pdcurses/initscr.c
  ${LIB_DIR}/pdcurses/pdcurses/inopts.c
  ${LIB_DIR}/pdcurses/pdcurses/insch.c
  ${LIB_DIR}/pdcurses/pdcurses/insstr.c
  ${LIB_DIR}/pdcurses/pdcurses/instr.c
  ${LIB_DIR}/pdcurses/pdcurses/kernel.c
  ${LIB_DIR}/pdcurses/pdcurses/mouse.c
  ${LIB_DIR}/pdcurses/pdcurses/move.c
  ${LIB_DIR}/pdcurses/pdcurses/outopts.c
  ${LIB_DIR}/pdcurses/pdcurses/overlay.c
  ${LIB_DIR}/pdcurses/pdcurses/pad.c
  ${LIB_DIR}/pdcurses/pdcurses/panel.c
  ${LIB_DIR}/pdcurses/pdcurses/printw.c
  ${LIB_DIR}/pdcurses/pdcurses/refresh.c
  ${LIB_DIR}/pdcurses/pdcurses/scanw.c
  ${LIB_DIR}/pdcurses/pdcurses/scr_dump.c
  ${LIB_DIR}/pdcurses/pdcurses/scroll.c
  ${LIB_DIR}/pdcurses/pdcurses/slk.c
  ${LIB_DIR}/pdcurses/pdcurses/termattr.c
  ${LIB_DIR}/pdcurses/pdcurses/terminfo.c
  ${LIB_DIR}/pdcurses/pdcurses/touch.c
  ${LIB_DIR}/pdcurses/pdcurses/util.c
  ${LIB_DIR}/pdcurses/pdcurses/window.c
  ${LIB_DIR}/pdcurses/win32a/pdcclip.c
  ${LIB_DIR}/pdcurses/win32a/pdcdisp.c
  ${LIB_DIR}/pdcurses/win32a/pdcgetsc.c
  ${LIB_DIR}/pdcurses/win32a/pdckbd.c
  ${LIB_DIR}/pdcurses/win32a/pdcscrn.c
  ${LIB_DIR}/pdcurses/win32a/pdcsetsc.c
  ${LIB_DIR}/pdcurses/win32a/pdcutil.c
)

# Used by detour lib
set( FASTLZLIST
  ${LIB_DIR}/fastlz/fastlz.c
)

# Used by daemon
set( DETOURLIST
  ${FASTLZLIST}
  ${LIB_DIR}/detour/DetourAlloc.cpp
  ${LIB_DIR}/detour/DetourCommon.cpp
  ${LIB_DIR}/detour/DetourNavMeshBuilder.cpp
  ${LIB_DIR}/detour/DetourNavMesh.cpp
  ${LIB_DIR}/detour/DetourNavMeshQuery.cpp
  ${LIB_DIR}/detour/DetourNode.cpp
  ${LIB_DIR}/detour/DetourPathCorridor.cpp
  ${LIB_DIR}/detour/DetourDebugDraw.cpp
  ${LIB_DIR}/detour/DebugDraw.cpp
  ${LIB_DIR}/detour/DetourTileCache.cpp
  ${LIB_DIR}/detour/DetourTileCacheBuilder.cpp
)

# Unused
set( RECASTLIST
  ${LIB_DIR}/recast/RecastAlloc.cpp
  ${LIB_DIR}/recast/RecastArea.cpp
  ${LIB_DIR}/recast/RecastContour.cpp
  ${LIB_DIR}/recast/Recast.cpp
  ${LIB_DIR}/recast/RecastFilter.cpp
  ${LIB_DIR}/recast/RecastLayers.cpp
  ${LIB_DIR}/recast/RecastMesh.cpp
  ${LIB_DIR}/recast/RecastMeshDetail.cpp
  ${LIB_DIR}/recast/RecastRasterization.cpp
  ${LIB_DIR}/recast/RecastRegion.cpp
  ${LIB_DIR}/recast/ChunkyTriMesh.cpp
)

if( APPLE )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/osx/nacl_imc.cc
  )
elseif( LINUX )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/linux/nacl_imc.cc
  )
elseif( WIN32 )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_imc.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_shm.cc
  )
endif()

################################################################################
# Libraries
################################################################################

# Native client
if( APPLE )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=1 )
elseif( LINUX )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=1 -DNACL_ANDROID=0 -DNACL_OSX=0 )
elseif( WIN32 )
  add_definitions( -DNACL_WINDOWS=1 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=0 )
endif()
if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
  add_definitions( -DNACL_BUILD_ARCH=x86 )
else()
  add_definitions( -DNACL_BUILD_ARCH=${ARCH} )
endif()
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  add_definitions( -DNACL_BUILD_SUBARCH=64 )
else()
  add_definitions( -DNACL_BUILD_SUBARCH=32 )
endif()
include_directories( ${LIB_DIR}/nacl )
set( LIBSRC_NACL ${LIBSRC_NACL} ${NACLLIST_NATIVE} )

if( NACL_RUNTIME_PATH )
  add_definitions( "-DNACL_RUNTIME_PATH=${NACL_RUNTIME_PATH}" )
endif()
