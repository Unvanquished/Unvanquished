# - Find vorbis library
# Find the native Vorbis headers and libraries.Vorbis depends on Ogg and will
# provide Ogg headers/libraries as well.
#
#  VORBIS_INCLUDE_DIRS   - where to find vorbis/vorbis.h, ogg/ogg.h, etc
#  VORBIS_LIBRARIES      - List of libraries when using libvorbis
#  VORBIS_FOUND          - True if vorbis is found.


#=============================================================================
#Copyright 2000-2009 Kitware, Inc., Insight Software Consortium
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#* Redistributions of source code must retain the above copyright notice, 
#this list of conditions and the following disclaimer.
#
#* Redistributions in binary form must reproduce the above copyright notice, 
#this list of conditions and the following disclaimer in the documentation 
#and/or other materials provided with the distribution.
#
#* Neither the names of Kitware, Inc., the Insight Software Consortium, nor 
#the names of their contributors may be used to endorse or promote products 
#derived from this software without specific prior written  permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
#ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
#LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
#CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
#INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
#ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
#POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# Find Ogg
if( Vorbis_FIND_REQUIRED )
  set( OGG_ARG "REQUIRED" )
elseif( Vorbis_FIND_QUIETLY )
  set( OGG_ARG "QUIETLY" )
endif()

find_package( Ogg ${OGG_ARG} )

# Look for the vorbisfile header file.
find_path( VORBIS_INCLUDE_DIR
  NAMES vorbis/vorbisfile.h
  DOC "Vorbis include directory" )
mark_as_advanced( VORBIS_INCLUDE_DIR )

# Look for the vorbisfile library.
find_library( VORBISFILE_LIBRARY
  NAMES vorbisfile
  DOC "Path to VorbisFile library" )
mark_as_advanced( VORBISFILE_LIBRARY )

# Look for the vorbis library.
find_library( VORBIS_LIBRARY
  NAMES vorbis
  DOC "Path to Vorbis library" )
mark_as_advanced( VORBIS_LIBRARY )



# handle the QUIETLY and REQUIRED arguments and set VORBISFILE_FOUND to TRUE if 
# all listed variables are TRUE
include( ${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( Vorbis DEFAULT_MSG VORBIS_LIBRARY VORBIS_INCLUDE_DIR )

set( VORBIS_LIBRARIES ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARIES} )
set( VORBIS_INCLUDE_DIRS ${VORBIS_INCLUDE_DIR} ${OGG_INCLUDE_DIRS} )
