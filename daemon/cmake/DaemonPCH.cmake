################################################################################
# Support for precompiled headers
################################################################################

# MSVC requires that an extra file be added to a project
if( USE_PRECOMPILED_HEADER AND MSVC )
  file( WRITE ${OBJ_DIR}/PrecompiledHeader.cpp "" )
  set( PCH_FILE ${OBJ_DIR}/PrecompiledHeader.cpp )
endif()

function( ADD_PRECOMPILED_HEADER Target Header )
  if( USE_PRECOMPILED_HEADER AND NOT CMAKE_VERSION VERSION_LESS 2.8.10 )
    # Get the common compile flags
    set( Flags ${CMAKE_CXX_FLAGS} )
    get_target_property( Type ${Target} TYPE )
    if( Type STREQUAL MODULE_LIBRARY )
        set( Flags ${Flags} ${CMAKE_SHARED_MODULE_CXX_FLAGS} )
    endif()
    separate_arguments( Flags )

    # Get the per-configuration compile flags
    foreach( Config Debug Release RelWithDebInfo MinSizeRel )
      string( TOUPPER ${Config} CONFIG )
      set( ConfigFlags ${CMAKE_CXX_FLAGS_${CONFIG}} )
      separate_arguments( ConfigFlags )
      foreach( Flag ${ConfigFlags} )
        set( Flags ${Flags} $<$<CONFIG:${Config}>:${Flag}> )
      endforeach()
    endforeach()

    # Get preprocessor options for the target and directory (global)
    get_directory_property( DirCompileDefs COMPILE_DEFINITIONS )
    get_directory_property( DirIncludeDirs INCLUDE_DIRECTORIES )
    get_target_property( TargetCompileDefs ${Target} COMPILE_DEFINITIONS )
    set( Defs )
    foreach( Def ${TargetCompileDefs} )
      set( Defs ${Defs} -D${Def} )
    endforeach()
    foreach( Def ${DirCompileDefs} )
      set( Defs ${Defs} -D${Def} )
    endforeach()
    foreach( Def ${DirIncludeDirs} )
      set( Defs ${Defs} -I${Def} )
    endforeach()

    # Specify minimum OSX version
    if( APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET )
      set( Flags ${Flags} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} )
    endif()

    # Compiler-specific PCH support
    if( CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
      add_custom_command( OUTPUT "${OBJ_DIR}/${Target}.h.gch"
        COMMAND ${CMAKE_CXX_COMPILER} ${Defs} ${Flags} -x c++-header ${Header} -o "${OBJ_DIR}/${Target}.h.gch"
        DEPENDS ${Header}
        IMPLICIT_DEPENDS CXX ${Header}
      )

      add_custom_target( ${Target}-pch DEPENDS "${OBJ_DIR}/${Target}.h.gch" )
      add_dependencies( ${Target} ${Target}-pch )

      if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
        set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "-include-pch ${OBJ_DIR}/${Target}.h.gch" )
      else()
        set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "-include ${OBJ_DIR}/${Target}.h -Winvalid-pch" )
      endif()
    elseif( MSVC )
      set_source_files_properties( ${PCH_FILE} PROPERTIES COMPILE_FLAGS "/Yc${Header}" )
      set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "/Yu${Header} /Fp${OBJ_DIR}/${Target}.pch /FI${Header}" )
    endif()
  endif()
endfunction()
