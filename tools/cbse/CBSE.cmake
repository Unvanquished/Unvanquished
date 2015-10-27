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

find_package(PythonInterp REQUIRED)

function(maybe_add_dep target dep)
    if (TARGET ${target})
        add_dependencies(${target} ${dep})
    endif()
endfunction()

function(CBSE target definition output)
    # Check if we need to initialize submodule
    file(GLOB RESULT ${CMAKE_SOURCE_DIR}/src/utils/cbse)
    list(LENGTH ${RESULT} RES_LEN)
    if(RES_LEN EQUAL 0)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
            find_package(Git REQUIRED)
            if (GIT_FOUND)
                execute_process(
                    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
            endif()
        endif()
    endif()
    # Check if python has all the dependencies
    # TODO: Execute pip directly here and install them
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "import jinja2, yaml, collections, argparse, sys, os.path, re"
        RESULT_VARIABLE RET)
    if (NOT RET EQUAL 0)
        message(FATAL_ERROR "Missing dependences for CBSE generation. Use pip -r install src/utils/cbse/requirements.txt to install")
    endif()
    set(GENERATED_CBSE ${output}/backend/CBSEBackend.cpp
                       ${output}/backend/CBSEBackend.h
                       ${output}/backend/CBSEComponents.h)
    add_custom_command(
        OUTPUT ${GENERATED_CBSE}
        COMMENT "Generating CBSE entities for ${definition}"
        DEPENDS ${definition}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/utils/cbse
        COMMAND
                ${PYTHON_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/src/utils/cbse/CBSE.py
                -s -o
                "${output}"
                "${definition}"
    )
    string(REPLACE ${CMAKE_SOURCE_DIR}/ "" rel_path ${definition})
    string(REPLACE "/" "-" new_target ${rel_path})
    add_custom_target(${new_target} ALL
        DEPENDS ${GENERATED_CBSE}
    )
    set(${target}_GENERATED_CBSE ${GENERATED_CBSE} PARENT_SCOPE)
endfunction()
