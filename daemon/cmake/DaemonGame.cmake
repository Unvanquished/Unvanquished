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

include_directories(${MOUNT_DIR} ${LIB_DIR} ${LIB_DIR}/zlib)
include(DaemonPlatform)
include(DaemonNacl)
include(DaemonFlags)

if (NOT NACL)
    if (WIN32)
        add_definitions(-DNOMINMAX)
    endif()
    if (NOT MSVC)
        # C++11 support
        try_cxx_flag(GNUXX11 "-std=gnu++11")
        if (NOT FLAG_GNUXX11)
            try_cxx_flag(GNUXX0X "-std=gnu++0x")
            if (NOT FLAG_GNUXX0X)
                message(FATAL_ERROR "C++11 not supported by compiler")
            endif()
        endif()
    endif()
endif()

# Function to setup all the Sgame/Cgame libraries
include(CMakeParseArguments)
function(GAMEMODULE)
    # ParseArguments setup
    set(oneValueArgs NAME)
    set(multiValueArgs DEFINITIONS FLAGS FILES LIBS)
    cmake_parse_arguments(GAMEMODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT NACL)
        if (BUILD_GAME_NATIVE_DLL)
            add_library(${GAMEMODULE_NAME}-native-dll MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-dll nacl-source-libs ${LIBS_BASE} ${GAMEMODULE_LIBS})
            set_target_properties(${GAMEMODULE_NAME}-native-dll PROPERTIES
                PREFIX ""
                COMPILE_DEFINITIONS "VM_NAME=\"${GAMEMODULE_NAME}\";${GAMEMODULE_DEFINITIONS};BUILD_VM;BUILD_VM_IN_PROCESS"
                COMPILE_FLAGS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            # ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-dll)
        endif()

        if (BUILD_GAME_NATIVE_EXE)
            add_executable(${GAMEMODULE_NAME}-native-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-exe nacl-source-libs ${LIBS_BASE} ${GAMEMODULE_LIBS})
            set_target_properties(${GAMEMODULE_NAME}-native-exe PROPERTIES
                COMPILE_DEFINITIONS "VM_NAME=\"${GAMEMODULE_NAME}\";${GAMEMODULE_DEFINITIONS};BUILD_VM"
                COMPILE_FLAGS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            # ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-exe)
        endif()

        if (NOT FORK AND BUILD_GAME_NACL)
            set(FORK 1 PARENT_SCOPE)
            if (NOT CMAKE_HOST_WIN32)
                set(PythonInterp_FIND_VERSION 2)
                find_package(PythonInterp 2)
                if (NOT PYTHONINTERP_FOUND)
                    message("Please set the PNACLPYTHON environment variable to your Python2 executable")
                endif()
                set(PNACLPYTHON "PNACLPYTHON=${PYTHON_EXECUTABLE}")
            endif()
            include(ExternalProject)
            set(vm nacl-vms)
            ExternalProject_Add(${vm}
                SOURCE_DIR ${CMAKE_SOURCE_DIR}
                BUILD_IN_SOURCE 0
                CMAKE_ARGS
                    -DFORK=2
                    -DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/toolchain-pnacl.cmake
                    -DDEPS_DIR=${DEPS_DIR}
                    -DBUILD_GAME_NACL=1
                    -DBUILD_GAME_NATIVE_DLL=0
                    -DBUILD_GAME_NATIVE_EXE=0
                    -DBUILD_CLIENT=0
                    -DBUILD_TTY_CLIENT=0
                    -DBUILD_SERVER=0
                BUILD_COMMAND ${PNACLPYTHON} ${CMAKE_COMMAND} --build .
                INSTALL_COMMAND ""
            )
            ExternalProject_Add_Step(${vm} forcebuild
                COMMAND ${CMAKE_COMMAND} -E remove
                    ${CMAKE_CURRENT_BINARY_DIR}/${vm}-prefix/src/${vm}-stamp/${vm}-build
                COMMENT "Forcing build step for '${vm}'"
                DEPENDEES build
                ALWAYS 1
            )
        endif()
    else()
        if (FORK EQUAL 2)
            set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/../../..)
            set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        endif()

        add_executable(${GAMEMODULE_NAME}-nacl-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${COMMONLIST} ${NACLLIST_MODULE})
        target_link_libraries(${GAMEMODULE_NAME}-nacl-exe ${GAMEMODULE_LIBS} ${LIBS_BASE})
        set_target_properties(${GAMEMODULE_NAME}-nacl-exe PROPERTIES
            OUTPUT_NAME ${GAMEMODULE_NAME}.pexe
            COMPILE_DEFINITIONS "VM_NAME=\"${GAMEMODULE_NAME}\";${GAMEMODULE_DEFINITIONS};BUILD_VM"
            COMPILE_FLAGS "${GAMEMODULE_FLAGS}"
            FOLDER ${GAMEMODULE_NAME}
        )
        # ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-nacl-exe)

        # Generate NaCl executables for x86 and x86_64
        pnacl_translate(${GAMEMODULE_NAME}-nacl-exe i686 "-x86")
        pnacl_translate(${GAMEMODULE_NAME}-nacl-exe x86-64 "-x86-64")
    endif()
endfunction()
