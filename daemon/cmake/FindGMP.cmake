# - Find gmp
# Find the native GMP includes and library
#
#  GMP_INCLUDE_DIR - where to find gmp.h, etc.
#  GMP_LIBRARIES   - List of libraries when using gmp.
#  GMP_FOUND       - True if gmp found.


IF (GMP_INCLUDE_DIR)
  # Already in cache, be silent
  SET(GMP_FIND_QUIETLY TRUE)
ENDIF (GMP_INCLUDE_DIR)

FIND_PATH(GMP_INCLUDE_DIR gmp.h
  /usr/local/include
  /usr/include
  /opt/local/include
)

SET(GMP_NAMES gmp)
FIND_LIBRARY(GMP_LIBRARY
  NAMES ${GMP_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

IF (GMP_INCLUDE_DIR AND GMP_LIBRARY)
   SET(GMP_FOUND TRUE)
    SET( GMP_LIBRARIES ${GMP_LIBRARY} )
ELSE (GMP_INCLUDE_DIR AND GMP_LIBRARY)
   SET(GMP_FOUND FALSE)
   SET( GMP_LIBRARIES )
ENDIF (GMP_INCLUDE_DIR AND GMP_LIBRARY)

IF (GMP_FOUND)
   IF (NOT GMP_FIND_QUIETLY)
      MESSAGE(STATUS "Found GMP: ${GMP_LIBRARY}")
   ENDIF (NOT GMP_FIND_QUIETLY)
ELSE (GMP_FOUND)
   IF (GMP_FIND_REQUIRED)
      MESSAGE(STATUS "Looked for gmp libraries named ${GMPS_NAMES}.")
      MESSAGE(FATAL_ERROR "Could NOT find gmp library")
   ENDIF (GMP_FIND_REQUIRED)
ENDIF (GMP_FOUND)

MARK_AS_ADVANCED (
  GMP_LIBRARY
  GMP_INCLUDE_DIR
)
