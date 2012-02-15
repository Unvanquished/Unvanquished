# - Try to find ogg
# Once done this will define
#
#  OGG_FOUND - system has ogg
#  OGG_INCLUDE_DIR
#  OGG_LIBRARY
#
# $OGGDIR is an environment variable used
# for finding ogg.
#

INCLUDE(FindPackageHandleStandardArgs)
INCLUDE(HandleLibraryTypes)

FIND_PATH(OGG_INCLUDE_DIR ogg/ogg.h
  PATHS $ENV{OGGDIR}
  PATH_SUFFIXES include
)
FIND_LIBRARY(OGG_LIBRARY_OPTIMIZED
  NAMES ogg
  PATHS $ENV{OGGDIR}
  PATH_SUFFIXES lib
)
FIND_LIBRARY(OGG_LIBRARY_DEBUG
  NAMES oggd ogg_d oggD ogg_D
  PATHS $ENV{OGGDIR}
  PATH_SUFFIXES lib
)

# Handle the REQUIRED argument and set OGG_FOUND
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ogg DEFAULT_MSG
  OGG_LIBRARY_OPTIMIZED
  OGG_INCLUDE_DIR
)

# Collect optimized and debug libraries
HANDLE_LIBRARY_TYPES(OGG)

MARK_AS_ADVANCED(
  OGG_INCLUDE_DIR
  OGG_LIBRARY_OPTIMIZED
  OGG_LIBRARY_DEBUG
)
