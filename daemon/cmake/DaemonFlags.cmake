################################################################################
# Configuration options
################################################################################

option( USE_LTO "Use link-time optimization for release builds" 0 )
cmake_dependent_option( USE_SLIM_LTO "Generate slim LTO objects, improves build times" 1 "USE_LTO AND ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU" 0 )
option( USE_HARDENING "Use stack protection and other hardening flags" 0 )
option( USE_WERROR "Tell the compiler to make the build fail when warnings are present" 0 )
option( USE_PEDANTIC "Tell the compiler to be pedantic" 0 )
option( USE_DEBUG_OPTIMIZE "Try to optimize the debug build" 1 )
option( USE_PRECOMPILED_HEADER "Improve build times by using a precompiled header" 1 )
option( USE_ADDRESS_SANITIZER "Try to use the address sanitizer" 0 )
option( BE_VERBOSE "Tell the compiler to report all warnings" 0 )
if( BE_VERBOSE )
  set( WARNMODE "no-error=" )
else()
  set( WARNMODE "no-" )
endif()

################################################################################
# Compile and link flags
################################################################################

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Set flag without checking, optional argument specifies build type
macro( set_c_flag FLAG )
  if( ${ARGC} GREATER 1 )
    set( CMAKE_C_FLAGS_${ARGV1} "${CMAKE_C_FLAGS_${ARGV1}} ${FLAG}" )
  else()
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" )
  endif()
endmacro()
macro( set_cxx_flag FLAG )
  if( ${ARGC} GREATER 1 )
    set( CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}" )
  else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" )
  endif()
endmacro()
macro( set_c_cxx_flag FLAG )
  set_c_flag( ${FLAG} ${ARGN} )
  set_cxx_flag( ${FLAG} ${ARGN} )
endmacro()
macro( set_linker_flag FLAG )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}" )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}" )
  set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}" )
endmacro()

# Try flag and set if it works, optional argument specifies build type
macro( try_cxx_flag PROP FLAG )
  check_CXX_compiler_flag( ${FLAG} FLAG_${PROP} )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_c_cxx_flag PROP FLAG )
  # Only try the flag once on the C++ compiler
  try_cxx_flag( ${PROP} ${FLAG} ${ARGV2} )
  if( FLAG_${PROP} )
    set_c_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
# Clang prints a warning when if it doesn't support a flag, so use -Werror to detect
macro( try_cxx_flag_werror PROP FLAG )
  set( CMAKE_REQUIRED_FLAGS "-Werror" )
  check_CXX_compiler_flag( ${FLAG} FLAG_${PROP} )
  set( CMAKE_REQUIRED_FLAGS "" )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_c_cxx_flag_werror PROP FLAG )
  try_cxx_flag_werror( ${PROP} ${FLAG} ${ARGV2} )
  if( FLAG_${PROP} )
    set_c_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_linker_flag PROP FLAG )
  # Check it with the C compiler
  set( CMAKE_REQUIRED_FLAGS ${FLAG} )
  check_C_compiler_flag( ${FLAG} FLAG_${PROP} )
  set( CMAKE_REQUIRED_FLAGS "" )
  if( FLAG_${PROP} )
    set_linker_flag( ${FLAG} ${ARGN} )
  endif()
endmacro()

if( MSVC )
  set_c_cxx_flag( "/MP" )
  set_c_cxx_flag( "/fp:fast" )
  set_c_cxx_flag( "/d2Zi+" RELWITHDEBINFO )
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "/arch:SSE2" )
  endif()
  if( USE_LTO )
    set_c_cxx_flag( "/GL" MINSIZEREL )
    set_c_cxx_flag( "/GL" RELWITHDEBINFO )
    set_c_cxx_flag( "/GL" RELEASE )
    set_linker_flag( "/LTCG" MINSIZEREL )
    set_linker_flag( "/LTCG" RELWITHDEBINFO )
    set_linker_flag( "/LTCG" RELEASE )
  endif()
  set_linker_flag( "/LARGEADDRESSAWARE" )

  # Turn off C4503:, e.g:
  # warning C4503: 'std::_Tree<std::_Tmap_traits<_Kty,_Ty,_Pr,_Alloc,false>>::_Insert_hint' : decorated name length exceeded, name was truncated
  # No issue will be caused from this error as long as no two symbols become identical by being truncated.
  # In practice this rarely happens and even the standard libraries are affected as in the example. So there really is not
  # much that can to done about it and the warnings about each truncation really just make it more likely
  # that other more real issues might get missed. So better to remove the distraction when it really is very unlikey to happen.
  set_c_cxx_flag( "/wd4503" )

  # Turn off warning C4996:, e.g:
  # warning C4996: 'open': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. See online help for details.  set_c_cxx_flag( "/wd4996" )
  # open seems far more popular than _open not to mention nicer. There doesn't seem to be any reason or will to change to _open.
  # So until there is a specific plan to tackle all of these type of warnings it's best to turn them off to the distraction.
  set_c_cxx_flag( "/wd4996" )

else()
  set_c_cxx_flag( "-ffast-math" )
  set_c_cxx_flag( "-fno-strict-aliasing" )

  # Set arch on x86 to SSE2 minimum and enable CMPXCHG16B
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "-m32" )
    set_c_cxx_flag( "-msse2" )
    set_c_cxx_flag( "-mtune=generic" )
    try_c_cxx_flag_werror( MFPMATH_SSE "-mfpmath=sse" )
  elseif( ARCH STREQUAL "x86_64" )
    set_c_cxx_flag( "-m64" )
    set_c_cxx_flag( "-mtune=generic" )
    try_c_cxx_flag_werror( MCX16 "-mcx16" )
  endif()

  # Use hidden symbol visibility if possible
  try_c_cxx_flag( FVISIBILITY_HIDDEN "-fvisibility=hidden" )

  # Extra debug flags
  set_c_cxx_flag( "-g3" DEBUG )
  set_c_cxx_flag( "-g3" RELWITHDEBINFO )
  if( USE_DEBUG_OPTIMIZE )
    try_c_cxx_flag( OPTIMIZE_DEBUG "-Og" DEBUG )
  endif()

  # C++11 support
  try_cxx_flag( GNUXX11 "-std=gnu++11" )
  if( NOT FLAG_GNUXX11 )
    try_cxx_flag( GNUXX0X "-std=gnu++0x" )
    if( NOT FLAG_GNUXX0X )
      message( FATAL_ERROR "C++11 not supported by compiler" )
    endif()
  endif()

  # Use MSVC-compatible bitfield layout
  if( WIN32 )
    set_c_cxx_flag( "-mms-bitfields" )
  endif()

  # Use libc++ on Mac because the shipped libstdc++ version is too old
  if( APPLE )
    set_c_cxx_flag( "-stdlib=libc++" )
    set_linker_flag( "-stdlib=libc++" )
  endif()

  # Hardening, don't set _FORTIFY_SOURCE in debug builds
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELEASE )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELWITHDEBINFO )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" MINSIZEREL )
  if( USE_HARDENING )
    try_c_cxx_flag( FSTACK_PROTECTOR_STRONG "-fstack-protector-strong" )
    if( NOT FLAG_FSTACK_PROTECTOR_STRONG )
      try_c_cxx_flag( FSTACK_PROTECTOR_ALL "-fstack-protector-all" )
    endif()
    try_c_cxx_flag( FNO_STRICT_OVERFLOW "-fno-strict-overflow" )
    try_c_cxx_flag( WSTACK_PROTECTOR "-Wstack-protector" )
    try_c_cxx_flag( FPIE "-fPIE" )
    try_linker_flag( LINKER_PIE "-pie" )
  endif()

  # Linker flags
  if( NOT APPLE )
    try_linker_flag( LINKER_O1 "-Wl,-O1" )
    try_linker_flag( LINKER_SORT_COMMON "-Wl,--sort-common" )
    try_linker_flag( LINKER_AS_NEEDED "-Wl,--as-needed" )
    if( NOT USE_ADDRESS_SANITIZER )
      try_linker_flag( LINKER_NO_UNDEFINED "-Wl,--no-undefined" )
    endif()
    try_linker_flag( LINKER_Z_RELRO "-Wl,-z,relro" )
    try_linker_flag( LINKER_Z_NOW "-Wl,-z,now" )
  endif()
  if( WIN32 )
    try_linker_flag( LINKER_DYNAMICBASE "-Wl,--dynamicbase" )
    try_linker_flag( LINKER_NXCOMPAT "-Wl,--nxcompat" )
    try_linker_flag( LINKER_LARGE_ADDRESS_AWARE "-Wl,--large-address-aware" )
  endif()

  # The -pthread flag sets some preprocessor defines, it is also used to link
  # with libpthread on Linux
  try_c_cxx_flag( PTHREAD "-pthread" )
  if( LINUX )
    set_linker_flag( "-pthread" )
  endif()

  # Warning options
  set_cxx_flag( "-Wall" )
  set_cxx_flag( "-Wextra" )
  if( USE_PEDANTIC )
    set_c_cxx_flag( "-pedantic" )
  endif()
  if( USE_WERROR )
    set_cxx_flag( "-Werror" )
    if( USE_PEDANTIC )
      set_cxx_flag( "-pedantic-errors" )
    endif()
  endif()
  try_cxx_flag_werror( WNO_DEPRECATED_REGISTER        "-Wno-error=deprecated-register" )
  try_cxx_flag_werror( WNO_INVALID_SOURCE_ENCODING    "-Wno-error=invalid-source-encoding" )
  try_cxx_flag_werror( WNO_MAYBE_UNINITIALIZED        "-Wno-error=maybe-uninitialized" )
  try_cxx_flag_werror( WNO_MISSING_FIELD_INITIALIZERS "-W${WARNMODE}missing-field-initializers" )
  try_cxx_flag_werror( WNO_PARENTHESES                "-Wno-error=parentheses" )
  try_cxx_flag_werror( WNO_POINTER_BOOL_CONVERSION    "-Wno-error=pointer-bool-conversion" )
  try_cxx_flag_werror( WNO_PRAGMAS                    "-Wno-error=pragmas" )
  try_cxx_flag_werror( WNO_SIGN_COMPARE               "-W${WARNMODE}sign-compare" )
  try_cxx_flag_werror( WNO_STRICT_OVERFLOW            "-W${WARNMODE}strict-overflow" )
  try_cxx_flag_werror( WNO_SWITCH                     "-Wno-error=switch" )
  try_cxx_flag_werror( WNO_UNINITIALIZED              "-Wno-error=uninitialized" )
  try_cxx_flag_werror( WNO_UNUSED_BUT_SET_PARAMETER   "-W${WARNMODE}unused-but-set-parameter" )
  try_cxx_flag_werror( WNO_UNUSED_BUT_SET_VARIABLE    "-W${WARNMODE}unused-but-set-variable" )
  try_cxx_flag_werror( WNO_UNUSED_FUNCTION            "-W${WARNMODE}unused-function" )
  try_cxx_flag_werror( WNO_UNUSED_PARAMETER           "-W${WARNMODE}unused-parameter" )
  try_cxx_flag_werror( WNO_UNUSED_PRIVATE_FIELD       "-W${WARNMODE}unused-private-field" )
  try_cxx_flag_werror( WNO_UNUSED_RESULT              "-W${WARNMODE}unused-result" )
  try_cxx_flag_werror( WNO_UNUSED_VARIABLE            "-W${WARNMODE}unused-variable" )

  if( USE_ADDRESS_SANITIZER )
    set_cxx_flag( "-fsanitize=address" )
    set_linker_flag( "-fsanitize=address" )
  endif()

  # Link-time optimization
  if( USE_LTO )
    set_c_cxx_flag( "-flto" )
    set_linker_flag( "-flto" )

    # For LTO compilation we must send a copy of all compile flags to the linker
    set_linker_flag( "${CMAKE_CXX_FLAGS}" )

    # Use gcc-ar and gcc-ranlib instead of ar and ranlib so that we can use
    # slim LTO objects. This requires a recent version of GCC and binutils.
    if( ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU )
      if( USE_SLIM_LTO )
        string(REGEX MATCH "^([0-9]+.[0-9]+)" _version
           "${CMAKE_CXX_COMPILER_VERSION}")
        get_filename_component(COMPILER_BASENAME "${CMAKE_C_COMPILER}" NAME)
        if (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
          set(TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
        endif()

        find_program(GCC_AR NAMES
          "${TOOLCHAIN_PREFIX}gcc-ar"
          "${TOOLCHAIN_PREFIX}gcc-ar-${_version}"
          DOC "gcc provided wrapper for ar which adds the --plugin option"
        )

        find_program(GCC_RANLIB NAMES
          "${TOOLCHAIN_PREFIX}gcc-ranlib"
          "${TOOLCHAIN_PREFIX}gcc-ranlib-${_version}"
          DOC "gcc provided wrapper for ranlib which adds the --plugin option"
        )

        mark_as_advanced( GCC_AR GCC_RANLIB )

        # Override standard ar and ranlib with the gcc- versions
        if( GCC_AR )
          set( CMAKE_AR ${GCC_AR} )
        endif()
        if( GCC_RANLIB )
          set( CMAKE_RANLIB ${GCC_RANLIB} )
        endif()

        try_c_cxx_flag( NO_FAT_LTO_OBJECTS "-fno-fat-lto-objects" )
      else()
        try_c_cxx_flag( FAT_LTO_OBJECTS "-ffat-lto-objects" )
      endif()
    endif()
  endif()

endif()

# Windows-specific definitions
if( WIN32 )
  # Minimum Windows version: XP
  # Define WIN32 for compatibility (compiler defines _WIN32)
  # Define NOMINMAX to prevent conflics between std::min/max and the min/max macros in WinDef.h
  # Enable STRICT type checking for windows.h
  add_definitions( -DWINVER=0x501 -DWIN32 -DNOMINMAX -DSTRICT )
  set( CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "" "lib" )
endif()
if( MSVC )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

# Mac-specific definitions
if( APPLE )
  add_definitions( -DMACOS_X )
  set_linker_flag( "-Wl,-no_pie" )
  set( CMAKE_INSTALL_RPATH "@executable_path/;${DEPS_DIR};${DEPS_DIR}/lib" )
  set( CMAKE_BUILD_WITH_INSTALL_RPATH ON )
endif()
