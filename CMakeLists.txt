#*******************************************************************************
#*   (c) 2018 Zondax GmbH
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
cmake_minimum_required(VERSION 3.0)
project(ledger-cosmos VERSION 0.0.0)
enable_testing()

cmake_policy(SET CMP0025 NEW)
set(CMAKE_CXX_STANDARD 11)

include(cmake/conan/CMakeLists.txt)
add_subdirectory(cmake/gtest)

string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address -fno-omit-frame-pointer")
string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=address -fno-omit-frame-pointer")

##############################################################
##############################################################
#  static libs
file(GLOB_RECURSE JSMN_SRC
        deps/jsmn/src/jsmn.c
        )

file(GLOB_RECURSE LIB_SRC
        app/src/json/json_parser.c
        app/src/tx_parser.c
        app/src/tx_display.c
        app/src/tx_validate.c
        app/src/parser.c
        app/src/parser_impl.c
        deps/ledger-zxlib/app/common/app_mode.c
        )

add_library(app_lib STATIC
        ${LIB_SRC}
        ${JSMN_SRC}
        )

target_include_directories(app_lib PUBLIC
        deps/ledger-zxlib/include
        deps/jsmn/src
        app/src
        app/src/common
        deps/ledger-zxlib/app/common
        )

target_link_libraries(app_lib PUBLIC bsd)

##############################################################
##############################################################
#  Tests
file(GLOB_RECURSE TESTS_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

add_executable(unittests ${TESTS_SRC})
target_include_directories(unittests PUBLIC
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include
        ${CONAN_INCLUDE_DIRS_FMT}
        ${CONAN_INCLUDE_DIRS_JSONCPP}
        deps/jsmn/src
        )

target_link_libraries(unittests PRIVATE
        gtest_main
        app_lib
        CONAN_PKG::fmt
        CONAN_PKG::jsoncpp)

add_test(unittests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unittests)
set_tests_properties(unittests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

file(GLOB_RECURSE FUZZING_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/fuzzing/fuzzingMain.cpp
        tests/util/common.cpp
        )

add_executable(fuzzing_stub ${FUZZING_SRC})
target_include_directories(fuzzing_stub PUBLIC
        app/src
        deps/jsmn/src
        )
target_link_libraries(fuzzing_stub app_lib)
