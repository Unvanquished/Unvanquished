# Create Glew for VS projects.

if( NOT WIN32 )
  message( FATAL_ERROR "USE_INTERNAL_SDL is only support for Windows" )
endif()

find_package( OpenGL REQUIRED )

include_directories( ${GLEW_DIR}/include )
add_library( INTERNAL_GLEW SHARED ${GLEW_DIR}/src/glew.c )
set_property( TARGET INTERNAL_GLEW PROPERTY COMPILE_DEFINITIONS GLEW_BUILD )

target_link_libraries( INTERNAL_GLEW ${OPENGL_LIBRARIES} )