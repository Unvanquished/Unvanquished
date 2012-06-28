set( LIB_DIR ${CMAKE_SOURCE_DIR}/../src/libs )

set( PNG_PNG_INCLUDE_DIR ${LIB_DIR}/libpng/ CACHE STRING "PNG include directory" FORCE )
set( PNG_LIBRARY ${LIB_DIR}/libpng/libs/win32/libpng.lib CACHE STRING "PNG library" FORCE )
set( FREETYPE_INCLUDE_DIRS ${LIB_DIR}/freetype/include/freetype2 ${LIB_DIR}/freetype/include CACHE STRING "Freetupe include directory" FORCE )
set( FREETYPE_LIBRARY ${LIB_DIR}/freetype/lib/freetype.lib CACHE STRING "Freetype library" FORCE )
set( ZLIB_INCLUDE_DIR ${LIB_DIR}/zlibwapi/include CACHE STRING "ZLIB include directory" FORCE )
set( ZLIB_LIBRARY ${LIB_DIR}/zlibwapi/lib/x32/zlibwapi.lib CACHE STRING "ZLIB library" FORCE )
set( GMP_INCLUDE_DIR ${LIB_DIR}/gmp/include CACHE STRING "GMP include directory" FORCE )
set( GMP_LIBRARY ${LIB_DIR}/GMP/libs/win32/mpir.lib CACHE STRING "GMP library" FORCE )
set( CURL_INCLUDE_DIR ${LIB_DIR}/curl-7.21.6/include CACHE STRING "cURL include directory" FORCE )
set( CURL_LIBRARY optimized ${LIB_DIR}/curl-7.21.6/lib/win32/release/libcurl_imp.lib debug ${LIB_DIR}/curl-7.21.6/lib/win32/debug/libcurld_imp.lib  CACHE STRING "cURL library" FORCE )
set( WEBP_INCLUDE_DIR ${LIB_DIR}/libwebp/Include CACHE STRING "webP include directory" FORCE )
set( WEBP_LIBRARY optimized ${LIB_DIR}/libwebp/Lib/x86/release/libwebp_a.lib debug ${LIB_DIR}/libwebp/Lib/x86/debug/libwebp_a_debug.lib CACHE STRING "webP library" FORCE )
set( VORBIS_INCLUDE_DIR ${LIB_DIR}/libvorbis/include CACHE STRING "Vorbis include directory" FORCE )
set( VORBIS_LIBRARY optimized ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbis.lib debug ${LIB_DIR}/libvorbis/libs/Win32/Debug/libvorbis.lib CACHE STRING "Vorbis library" FORCE )
set( VORBISFILE_LIBRARY optimized ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbisfile.lib debug ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbisfile.lib CACHE STRING "VorbisFile library" FORCE )
set( OGG_INCLUDE_DIR ${LIB_DIR}/libogg/include CACHE STRING "Ogg include directory" FORCE )
set( OGG_LIBRARY optimized ${LIB_DIR}/libogg/libs/win32/release/libogg.lib debug ${LIB_DIR}/libogg/libs/win32/debug/libogg.lib CACHE STRING "Ogg library" FORCE )
set( THEORA_INCLUDE_DIR ${LIB_DIR}/libtheora/include CACHE STRING "Theora include directory" FORCE )
set( THEORA_LIBRARY optimized ${LIB_DIR}/libtheora/libs/win32/release/libtheora.lib debug ${LIB_DIR}/libtheora/libs/win32/debug/libtheora.lib CACHE STRING "Theora library" FORCE )
set( XVID_INCLUDE_DIR ${LIB_DIR}/xvidcore/include CACHE STRING "Xvid include directory" FORCE )
set( XVID_LIBRARY ${LIB_DIR}/xvidcore/lib/xvidcore.lib CACHE STRING "Xvid library" FORCE )
set( BZIP2_INCLUDE_DIR ${LIB_DIR}/libbz2/include CACHE STRING "Bunzip2 include directory" FORCE )
set( BZIP2_LIBRARY ${LIB_DIR}/libbz2/libs/x32/libbz2.lib CACHE STRING "Bunzip2 library" FORCE )
set( GLIB_LIBRARY ${LIB_DIR}/glib/lib/x32/glib-2.0.lib CACHE STRING "GLIB library" FORCE )
set( GLIB_INCLUDE_DIR ${LIB_DIR}/glib/include/glib-2.0 CACHE STRING "GLIB include directory" FORCE )
set( GLIBCONFIG_INCLUDE_DIR ${LIB_DIR}/glib/lib/x32/glib-2.0/include CACHE STRING "GLIB config include directory" FORCE )
set( OPENAL_LIBRARY ${LIB_DIR}/openal/libs/win32/OpenAL32.lib CACHE STRING "OpenAL library" FORCE )
set( OPENAL_INCLUDE_DIR ${LIB_DIR}/openal/include CACHE STRING "OpenAL include directory" FORCE )

# Release
file( COPY
  ${LIB_DIR}/libpng/libs/win32/libpng14-14.dll
  ${LIB_DIR}/zlibwapi/lib/x32/zlibwapi.dll
  ${LIB_DIR}/curl-7.21.6/lib/win32/release/libcurl.dll
  ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbis.dll
  ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbisfile.dll
  ${LIB_DIR}/libogg/libs/win32/release/libogg.dll
  ${LIB_DIR}/libtheora/libs/win32/release/libtheora.dll
  ${LIB_DIR}/libbz2/libs/x32/libbz2.dll
  ${LIB_DIR}/freetype/lib/freetype6.dll
  DESTINATION Release
)
  
# Debug
file( COPY
  ${LIB_DIR}/libpng/libs/win32/libpng14-14.dll
  ${LIB_DIR}/zlibwapi/lib/x32/zlibwapi.dll
  ${LIB_DIR}/curl-7.21.6/lib/win32/debug/libcurld.dll
  ${LIB_DIR}/libvorbis/libs/Win32/Debug/libvorbis.dll
  ${LIB_DIR}/libvorbis/libs/Win32/Debug/libvorbisfile.dll
  ${LIB_DIR}/libogg/libs/win32/debug/libogg.dll
  ${LIB_DIR}/libtheora/libs/win32/debug/libtheora.dll
  ${LIB_DIR}/libbz2/libs/x32/libbz2.dll
  ${LIB_DIR}/freetype/lib/freetype6.dll
  DESTINATION Debug
)