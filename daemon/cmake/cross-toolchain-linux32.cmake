# Target operating system and architecture
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR x86 )

# C/C++ compilers
set( CMAKE_C_COMPILER gcc -m32 )
set( CMAKE_CXX_COMPILER g++ -m32 )

# Only look for libraries in lib32
set( CMAKE_SYSTEM_LIBRARY_PATH /lib32 /usr/lib32 /usr/local/lib32 )
set_property( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF )
set( CMAKE_IGNORE_PATH /lib /usr/lib /usr/local/lib )
