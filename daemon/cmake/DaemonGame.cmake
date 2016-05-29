# Daemon BSD Source Code
# Copyright (c) 2013-2016, Daemon Developers
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

# Function to setup all the Sgame/Cgame libraries
include(CMakeParseArguments)
function(GAMEMODULE)
    # ParseArguments setup
    set(oneValueArgs NAME)
    set(multiValueArgs DEFINITIONS FLAGS FILES LIBS)
    cmake_parse_arguments(GAMEMODULE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (NOT NACL)
        if (BUILD_GAME_NATIVE_DLL)
            add_library(${GAMEMODULE_NAME}-native-dll MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-dll ${GAMEMODULE_LIBS} ${LIBS_BASE})
            set_target_properties(${GAMEMODULE_NAME}-native-dll PROPERTIES
                PREFIX ""
                COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM;BUILD_VM_IN_PROCESS"
                COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-dll)
        endif()

        if (BUILD_GAME_NATIVE_EXE)
            add_executable(${GAMEMODULE_NAME}-native-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
            target_link_libraries(${GAMEMODULE_NAME}-native-exe ${GAMEMODULE_LIBS} ${LIBS_BASE})
            set_target_properties(${GAMEMODULE_NAME}-native-exe PROPERTIES
                COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM"
                COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
                FOLDER ${GAMEMODULE_NAME}
            )
            ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-native-exe)
        endif()

        if (NOT FORK AND BUILD_GAME_NACL)
            if (CMAKE_GENERATOR MATCHES "Visual Studio")
                set(VM_GENERATOR "NMake Makefiles")
            else()
                set(VM_GENERATOR ${CMAKE_GENERATOR})
            endif()
            set(FORK 1 PARENT_SCOPE)
            include(ExternalProject)
            set(vm nacl-vms)
            ExternalProject_Add(${vm}
                SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
                BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${vm}
                CMAKE_GENERATOR ${VM_GENERATOR}
                CMAKE_ARGS
                    -DFORK=2
                    -DCMAKE_TOOLCHAIN_FILE=${Daemon_SOURCE_DIR}/cmake/toolchain-pnacl.cmake
                    -DCMAKE_BUILD_TYPE=$<CONFIG>
                    -DDEPS_DIR=${DEPS_DIR}
                    -DBUILD_CGAME=${BUILD_CGAME}
                    -DBUILD_SGAME=${BUILD_SGAME}
                    -DBUILD_GAME_NACL_NEXE=${BUILD_GAME_NACL_NEXE}
                    -DBUILD_GAME_NACL=1
                    -DBUILD_GAME_NATIVE_DLL=0
                    -DBUILD_GAME_NATIVE_EXE=0
                    -DBUILD_CLIENT=0
                    -DBUILD_TTY_CLIENT=0
                    -DBUILD_SERVER=0
                INSTALL_COMMAND ""
            )
            ExternalProject_Add_Step(${vm} forcebuild
                COMMAND ${CMAKE_COMMAND} -E remove
                    ${CMAKE_CURRENT_BINARY_DIR}/${vm}-prefix/src/${vm}-stamp/${vm}-configure
                COMMENT "Forcing build step for '${vm}'"
                DEPENDEES build
                ALWAYS 1
            )

        endif()
    else()
        if (FORK EQUAL 2)
            # Put the .nexe and .pexe files in the same directory as the engine
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)
        endif()

        add_executable(${GAMEMODULE_NAME}-nacl ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST_${GAMEMODULE_NAME}} ${SHAREDLIST} ${COMMONLIST})
        target_link_libraries(${GAMEMODULE_NAME}-nacl ${GAMEMODULE_LIBS} ${LIBS_BASE})
        set_target_properties(${GAMEMODULE_NAME}-nacl PROPERTIES
            OUTPUT_NAME ${GAMEMODULE_NAME}.pexe
            COMPILE_DEFINITIONS "VM_NAME=${GAMEMODULE_NAME};${GAMEMODULE_DEFINITIONS};BUILD_VM"
            COMPILE_OPTIONS "${GAMEMODULE_FLAGS}"
            FOLDER ${GAMEMODULE_NAME}
        )
        ADD_PRECOMPILED_HEADER(${GAMEMODULE_NAME}-nacl)

        if (BUILD_GAME_NACL_NEXE)
            # Generate NaCl executables for x86 and x86_64
            pnacl_translate(${GAMEMODULE_NAME}-nacl i686 "-x86")
            pnacl_translate(${GAMEMODULE_NAME}-nacl x86-64 "-x86_64")
        endif()
    endif()
endfunction()
