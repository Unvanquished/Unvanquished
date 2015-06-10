# Dependencies version, this must match the number in external_deps/build.sh
set( DEPS_VERSION 3 )

set ( EXTERNAL_DEPS ${Daemon_SOURCE_DIR}/external_deps )

if( MSVC )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${EXTERNAL_DEPS}/msvc64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${EXTERNAL_DEPS}/msvc32-${DEPS_VERSION} )
  endif()
elseif( WIN32 )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${EXTERNAL_DEPS}/mingw64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${EXTERNAL_DEPS}/mingw32-${DEPS_VERSION} )
  endif()
elseif( APPLE )
  set( DEPS_EXT ".tar.bz2" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${EXTERNAL_DEPS}/macosx64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${EXTERNAL_DEPS}/macosx32-${DEPS_VERSION} )
  endif()
elseif( LINUX )
  set( DEPS_EXT ".tar.bz2" )
  # Our minimal NaCl .debs put the files in /usr/lib/nacl, so check that first
  if( EXISTS "/usr/lib/nacl/nacl_loader" )
    set( DEPS_DIR "/usr/lib/nacl" )
  elseif( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${EXTERNAL_DEPS}/linux64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${EXTERNAL_DEPS}/linux32-${DEPS_VERSION} )
  endif()
endif()

# Import external dependencies
if( DEPS_DIR )
  # Download them if they not available
  if( NOT EXISTS ${DEPS_DIR} )
    get_filename_component( BASENAME ${DEPS_DIR} NAME )
    set( REMOTE "http://dl.unvanquished.net/deps/${BASENAME}${DEPS_EXT}" )
    message( STATUS "Downloading dependencies from '${REMOTE}'" )
    file( DOWNLOAD ${REMOTE} ${OBJ_DIR}/${BASENAME}${DEPS_EXT} SHOW_PROGRESS
      STATUS DOWNLOAD_RESULT LOG DOWNLOAD_LOG )
    list( GET DOWNLOAD_RESULT 0 DOWNLOAD_STATUS )
    list( GET DOWNLOAD_RESULT 1 DOWNLOAD_STRING )
    if( NOT DOWNLOAD_STATUS EQUAL 0 )
      message( FATAL_ERROR "Error downloading '${REMOTE}':
        Status code: ${DOWNLOAD_STATUS}
        Error string: ${DOWNLOAD_STRING}
        Download log: ${DOWNLOAD_LOG}" )
    endif()
    message( STATUS "Download completed successfully" )

    # Extract the downloaded archive
    execute_process( COMMAND ${CMAKE_COMMAND} -E tar xjf ${OBJ_DIR}/${BASENAME}${DEPS_EXT}
      WORKING_DIRECTORY ${EXTERNAL_DEPS} RESULT_VARIABLE EXTRACT_RESULT )
    if( NOT EXTRACT_RESULT EQUAL 0 )
      message( FATAL_ERROR "Could not extract ${BASENAME}${DEPS_EXT}" )
    endif()
  endif()

  # Add to paths
  set( CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH} )
  set( CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH} )
  set( CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH} )
  set( CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH} )
endif()
