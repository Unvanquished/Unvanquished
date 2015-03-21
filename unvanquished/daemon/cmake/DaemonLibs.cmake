################################################################################
# Libraries
################################################################################

# Native client
if( APPLE )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=1 )
elseif( LINUX )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=1 -DNACL_ANDROID=0 -DNACL_OSX=0 )
elseif( WIN32 )
  add_definitions( -DNACL_WINDOWS=1 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=0 )
endif()
if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
  add_definitions( -DNACL_BUILD_ARCH=x86 )
else()
  add_definitions( -DNACL_BUILD_ARCH=${ARCH} )
endif()
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  add_definitions( -DNACL_BUILD_SUBARCH=64 )
else()
  add_definitions( -DNACL_BUILD_SUBARCH=32 )
endif()
include_directories( ${LIB_DIR}/nacl )
set( LIBSRC_NACL ${LIBSRC_NACL} ${NACLLIST_NATIVE} )

if( NACL_RUNTIME_PATH )
  add_definitions( "-DNACL_RUNTIME_PATH=${NACL_RUNTIME_PATH}" )
endif()
