# Locate xvid library  
# This module defines
# XVID_LIBRARY, the name of the library to link against
# XVID_FOUND, if false, do not try to link
# XVID_INCLUDE_DIR, where to find header
#

set( XVID_FOUND "NO" )

find_path( XVID_INCLUDE_DIR xvid.h
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

find_library( XVID_LIBRARY
  NAMES xvidcore
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

if(XVID_LIBRARY)
  set( XVID_FOUND "YES" )
endif(XVID_LIBRARY)
