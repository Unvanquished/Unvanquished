################################################################################
# Determine platform
################################################################################

# When adding a new platform, look at all the places WIN32, APPLE and LINUX are used
if( CMAKE_SYSTEM_NAME MATCHES "Linux" )
  set( LINUX ON )
elseif( NOT WIN32 AND NOT APPLE )
  message( FATAL_ERROR "Platform not supported" )
endif()

################################################################################
# Determine architecture
################################################################################

# When adding a new architecture, look at all the places ARCH is used
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( ARCH "x86_64" )
else()
  set( ARCH "x86" )
endif()

################################################################################
# Set FULL_OUTPUT_DIR
################################################################################

if( CMAKE_CFG_INTDIR STREQUAL "." )
  set( FULL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} )
else()
  set( FULL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR} )
endif()
