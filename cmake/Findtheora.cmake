# Locate libtheora libraries
# This module defines
# THEORA_LIBRARY, the name of the library to link against
# THEORA_FOUND, if false, do not try to link
# THEORA_INCLUDE_DIR, where to find header
#

set( THEORA_FOUND "NO" )

find_path( THEORA_INCLUDE_DIR theora/theora.h
  HINTS
  PATH_SUFFIXES include 
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw
)

find_library( THEORA_LIBRARY
  NAMES theora theoraenc theoradec 
  HINTS
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

if(THEORA_LIBRARY)
set( THEORA_FOUND "YES" )
endif(THEORA_LIBRARY)
