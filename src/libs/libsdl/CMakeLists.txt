# Create SDL and SDLMain for use in VS project files. This should eventually
# be extended to work for linux/mac generators as well.

if( NOT WIN32 )
  message( FATAL_ERROR "USE_INTERNAL_SDL is only support for Windows" )
endif()

# Add files needed to compile SDL
file( GLOB SDLLIST
  ${SDL_DIR}/src/*.c
  ${SDL_DIR}/src/audio/*.c
  ${SDL_DIR}/src/cdrom/*.c
  ${SDL_DIR}/src/cpuinfo/*.c
  ${SDL_DIR}/src/events/*.c
  ${SDL_DIR}/src/file/*.c
  ${SDL_DIR}/src/stdlib/*.c
  ${SDL_DIR}/src/thread/*.c
  ${SDL_DIR}/src/timer/*.c
  ${SDL_DIR}/src/video/*.c
  ${SDL_DIR}/src/joystick/*.c
  ${SDL_DIR}/src/video/dummy/*.c
  ${SDL_DIR}/src/audio/disk/*.c
  ${SDL_DIR}/src/audio/dummy/*.c
  ${SDL_DIR}/src/cpuinfo/*.c )
set( SDLLIST ${SDLLIST}
  ${SDL_DIR}/src/thread/generic/SDL_syscond.c
)

# Append windows specific files  
if( WIN32 )
  file( GLOB SDLLISTWIN
    ${SDL_DIR}/src/video/wincommon/*.c
    ${SDL_DIR}/src/video/windib/*.c
    ${SDL_DIR}/src/video/windx5/*.c
    ${SDL_DIR}/src/audio/windib/*.c
    ${SDL_DIR}/src/audio/windx5/*.c
    ${SDL_DIR}/src/joystick/win32/*.c
    ${SDL_DIR}/src/cdrom/win32/*.c
    ${SDL_DIR}/src/timer/win32/*.c
    ${SDL_DIR}/src/loadso/win32/*.c
  )
  set( SDLLIST ${SDLLIST}
    ${SDLLISTWIN}
    ${SDL_DIR}/src/thread/win32/SDL_syssem.c
    ${SDL_DIR}/src/thread/win32/SDL_systhread.c
    ${SDL_DIR}/src/thread/win32/SDL_sysmutex.c
  )
endif()

# Add files needed to compile SDLMain
set( SDLMAINLIST
  ${SDL_DIR}/src/main/win32/SDL_win32_main.c
)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )

include_directories( ${SDL_DIR}/include )
add_library( INTERNAL_SDL STATIC ${SDLLIST} )
target_link_libraries( INTERNAL_SDL winmm )

if( ${BUILD_ARCH} STREQUAL "x86_64" )
  target_link_libraries( INTERNAL_SDL ${SDL_DIR}/libs/x64/dxguid.lib )
else()
  target_link_libraries( INTERNAL_SDL ${SDL_DIR}/libs/x32/dxguid.lib )
endif()
add_library( INTERNAL_SDLMain STATIC ${SDLMAINLIST} )