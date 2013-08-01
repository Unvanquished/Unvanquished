# - Find the curses include file and library (wide version)
#
#  CURSESW_FOUND - system has Curses
#  CURSESW_INCLUDE_DIR - the Curses include directory
#  CURSESW_LIBRARIES - The libraries needed to use Curses
#  CURSESW_HAVE_CURSES_H - true if curses.h is available
#  CURSESW_HAVE_NCURSES_H - true if ncurses.h is available
#  CURSESW_HAVE_NCURSESW_NCURSES_H - true if ncurses/ncurses.h is available
#  CURSESW_HAVE_NCURSESW_CURSES_H - true if ncurses/curses.h is available
#  CURSESW_LIBRARY - set for backwards compatibility with 2.4 CMake

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
# Copyright 2012 Darren Salt
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# No idea if plain curses provides a wide version...
# ncurses definitely does, though.

FIND_LIBRARY(CURSESW_CURSESW_LIBRARY NAMES cursesw )
FIND_LIBRARY(CURSESW_NCURSESW_LIBRARY NAMES ncursesw )

IF(CURSESW_NCURSESW_LIBRARY  AND NOT  CURSESW_CURSESW_LIBRARY)
  SET(CURSESW_USE_NCURSESW TRUE)
ENDIF()

# Not sure the logic is correct here.
# If NCursesW is required, use the function wsyncup() to check if the library
# has NCursesW functionality (at least this is where it breaks on NetBSD).
# If wsyncup is in curses, use this one.
# If not, try to find ncursesw and check if this has the symbol.
# Once the ncursesw library is found, search the ncurses.h header first, but
# some web pages also say that even with ncurses there is not always a ncurses.h:
# http://osdir.com/ml/gnome.apps.mc.devel/2002-06/msg00029.html
# So at first try ncurses.h, if not found, try to find curses.h under the same
# prefix as the library was found, if still not found, try curses.h with the 
# default search paths.
IF(CURSESW_CURSESW_LIBRARY  AND  CURSESW_NEED_NCURSESW)
  INCLUDE(CheckLibraryExists)
  CHECK_LIBRARY_EXISTS("${CURSESW_CURSESW_LIBRARY}" 
    wsyncup "" CURSESW_CURSES_HAS_WSYNCUP)

  IF(CURSESW_NCURSESW_LIBRARY  AND NOT  CURSESW_CURSES_HAS_WSYNCUP)
    CHECK_LIBRARY_EXISTS("${CURSESW_NCURSESW_LIBRARY}" 
      wsyncup "" CURSESW_NCURSES_HAS_WSYNCUP)
    IF( CURSESW_NCURSES_HAS_WSYNCUP)
      SET(CURSESW_USE_NCURSESW TRUE)
    ENDIF( CURSESW_NCURSES_HAS_WSYNCUP)
  ENDIF(CURSESW_NCURSESW_LIBRARY  AND NOT  CURSESW_CURSES_HAS_WSYNCUP)

ENDIF()


IF(NOT CURSESW_USE_NCURSESW)
  GET_FILENAME_COMPONENT(_cursesLibDir "${CURSESW_CURSESW_LIBRARY}" PATH)
  GET_FILENAME_COMPONENT(_cursesParentDir "${_cursesLibDir}" PATH)

  FIND_FILE(CURSESW_HAVE_CURSESW_CURSES_H  cursesw/curses.h
    PATHS "${_cursesParentDir}/include")
  FIND_PATH(CURSESW_CURSESW_H_PATH cursesw/curses.h
    PATHS "${_cursesParentDir}/include")

  SET(CURSESW_INCLUDE_PATH "${CURSESW_CURSES_H_PATH}/cursesw" 
    CACHE FILEPATH "The curses include path")
  SET(CURSESW_LIBRARY "${CURSESW_CURSESW_LIBRARY}"
    CACHE FILEPATH "The curses library")
ELSE()
# we need to find ncurses
  GET_FILENAME_COMPONENT(_cursesLibDir "${CURSESW_NCURSESW_LIBRARY}" PATH)
  GET_FILENAME_COMPONENT(_cursesParentDir "${_cursesLibDir}" PATH)

  FIND_FILE(CURSESW_HAVE_NCURSESW_NCURSES_H ncursesw/ncurses.h
    PATHS "${_cursesParentDir}/include")
  FIND_FILE(CURSESW_HAVE_NCURSESW_CURSES_H  ncursesw/curses.h
    PATHS "${_cursesParentDir}/include")

  FIND_PATH(CURSESW_NCURSESW_INCLUDE_PATH   ncursesw/ncurses.h
    PATHS "${_cursesParentDir}/include")

  SET(CURSESW_INCLUDE_PATH "${CURSESW_NCURSESW_INCLUDE_PATH}/ncursesw" 
    CACHE FILEPATH "The curses include path")
  SET(CURSESW_LIBRARY "${CURSESW_NCURSESW_LIBRARY}"
    CACHE FILEPATH "The curses library")

ENDIF()

FIND_LIBRARY(CURSESW_EXTRA_LIBRARY cur_colrw HINTS "${_cursesLibDir}")
FIND_LIBRARY(CURSESW_EXTRA_LIBRARY cur_colrw )

FIND_LIBRARY(CURSESW_FORM_LIBRARY formw HINTS "${_cursesLibDir}")
FIND_LIBRARY(CURSESW_FORM_LIBRARY formw )

# for compatibility with older FindCurses.cmake this has to be in the cache
# FORCE must not be used since this would break builds which preload a cache
# with these variables set
SET(FORM_LIBRARY "${CURSESW_FORM_LIBRARY}"         
  CACHE FILEPATH "The curses form library")

# Need to provide the *_LIBRARIES
SET(CURSESW_LIBRARIES ${CURSESW_LIBRARY})

IF(CURSESW_EXTRA_LIBRARY)
  SET(CURSESW_LIBRARIES ${CURSESW_LIBRARIES} ${CURSESW_EXTRA_LIBRARY})
ENDIF()

IF(CURSESW_FORM_LIBRARY)
  SET(CURSESW_LIBRARIES ${CURSESW_LIBRARIES} ${CURSESW_FORM_LIBRARY})
ENDIF()

# Proper name is *_INCLUDE_DIR
SET(CURSESW_INCLUDE_DIR ${CURSESW_INCLUDE_PATH})

# handle the QUIETLY and REQUIRED arguments and set CURSESW_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CursesW DEFAULT_MSG
  CURSESW_LIBRARY CURSESW_INCLUDE_PATH)

MARK_AS_ADVANCED(
  CURSESW_INCLUDE_PATH
  CURSESW_LIBRARY
  CURSESW_HAVE_CURSESW_CURSES_H
  CURSESW_CURSESW_H_PATH
  CURSESW_CURSESW_INCLUDE_PATH
  CURSESW_CURSESW_LIBRARY
  CURSESW_HAVE_NCURSESW_NCURSES_H
  CURSESW_HAVE_NCURSESW_CURSES_H
  CURSESW_NCURSESW_INCLUDE_PATH
  CURSESW_NCURSESW_LIBRARY
  CURSESW_EXTRA_LIBRARY
  CURSESW_FORM_LIBRARY
  CURSESW_LIBRARIES
  CURSESW_INCLUDE_DIR
  CURSESW_CURSES_HAS_WSYNCUP
  CURSESW_NCURSES_HAS_WSYNCUP
  FORM_LIBRARY
)

