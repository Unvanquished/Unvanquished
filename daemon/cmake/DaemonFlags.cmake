# Daemon BSD Source Code
# Copyright (c) 2013-2014, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Set flag without checking, optional argument specifies build type
macro(set_c_flag FLAG)
    if (${ARGC} GREATER 1)
        set(CMAKE_C_FLAGS_${ARGV1} "${CMAKE_C_FLAGS_${ARGV1}} ${FLAG}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
    endif()
endmacro()
macro(set_cxx_flag FLAG)
    if (${ARGC} GREATER 1)
        set(CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
    endif()
endmacro()
macro(set_c_cxx_flag FLAG)
    set_c_flag(${FLAG} ${ARGN})
    set_cxx_flag(${FLAG} ${ARGN})
endmacro()
macro(set_linker_flag FLAG)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}")
endmacro()

function(try_flag LIST FLAG)
    string(REGEX REPLACE "[/=-]" "_" TEST ${FLAG})
    set(CMAKE_REQUIRED_FLAGS "-Werror")
    check_CXX_compiler_flag("${FLAG}" ${TEST})
    set(CMAKE_REQUIRED_FLAGS "")
    if (${TEST})
        set(${LIST} ${${LIST}} ${FLAG} PARENT_SCOPE)
    endif()
endfunction()

# Try flag and set if it works, optional argument specifies build type
macro(try_cxx_flag PROP FLAG)
    check_CXX_compiler_flag(${FLAG} FLAG_${PROP})
    if (FLAG_${PROP})
        set_cxx_flag(${FLAG} ${ARGV2})
    endif()
endmacro()
macro(try_c_cxx_flag PROP FLAG)
    # Only try the flag once on the C++ compiler
    try_cxx_flag(${PROP} ${FLAG} ${ARGV2})
    if (FLAG_${PROP})
        set_c_flag(${FLAG} ${ARGV2})
    endif()
endmacro()
# Clang prints a warning when if it doesn't support a flag, so use -Werror to detect
macro(try_cxx_flag_werror PROP FLAG)
    set(CMAKE_REQUIRED_FLAGS "-Werror")
    check_CXX_compiler_flag(${FLAG} FLAG_${PROP})
    set(CMAKE_REQUIRED_FLAGS "")
    if (FLAG_${PROP})
        set_cxx_flag(${FLAG} ${ARGV2})
    endif()
endmacro()
macro(try_c_cxx_flag_werror PROP FLAG)
    try_cxx_flag_werror(${PROP} ${FLAG} ${ARGV2})
    if (FLAG_${PROP})
        set_c_flag(${FLAG} ${ARGV2})
    endif()
endmacro()

macro(try_linker_flag PROP FLAG)
    # Check it with the C compiler
    set(CMAKE_REQUIRED_FLAGS ${FLAG})
    check_C_compiler_flag(${FLAG} FLAG_${PROP})
    set(CMAKE_REQUIRED_FLAGS "")
    if (FLAG_${PROP})
        set_linker_flag(${FLAG} ${ARGN})
    endif()
endmacro()
