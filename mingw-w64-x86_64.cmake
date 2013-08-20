# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 ${CMAKE_SOURCE_DIR}/external_deps)
SET(CMAKE_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/external_deps)
SET(CMAKE_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external_deps/win64)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# extra definitions for included libraries
add_definitions(-DCURL_STATICLIB -DGLEW_STATIC)
SET(CURL_LIBRARY ${CMAKE_SOURCE_DIR}/external_deps/win64/libcurl.a wldap32 ${CMAKE_SOURCE_DIR}/external_deps/win64/libz.a ws2_32)
SET(SDL_LIBRARY mingw32 ${CMAKE_SOURCE_DIR}/external_deps/win64/libSDLmain.a ${CMAKE_SOURCE_DIR}/external_deps/win64/libSDL.a -mwindows m user32 gdi32 winmm dxguid)
