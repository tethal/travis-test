cmake_minimum_required(VERSION 3.2)
project(travis-test VERSION 1.0.0 LANGUAGES C CXX)

option(COVERAGE "Compile with coverage info" ON)

if(COVERAGE)
    SET(GCC_COVERAGE_FLAGS -fprofile-arcs -ftest-coverage)
    SET(GCC_COVERAGE_LINK gcov)
endif(COVERAGE)

include(CMakeParseArguments)

#Setup GoogleTest and GoogleMock
find_package(Threads REQUIRED)
include(ExternalProject)
ExternalProject_Add(gtest URL http://googletest.googlecode.com/files/gtest-1.6.0.zip PREFIX ${CMAKE_BINARY_DIR}/ext INSTALL_COMMAND "")
ExternalProject_Add(gmock URL http://googlemock.googlecode.com/files/gmock-1.6.0.zip PREFIX ${CMAKE_BINARY_DIR}/ext INSTALL_COMMAND "")
link_directories(${CMAKE_BINARY_DIR}/ext/src/gtest-build ${CMAKE_BINARY_DIR}/ext/src/gmock-build)
add_library(googletest INTERFACE)
target_link_libraries(googletest INTERFACE gtest gmock ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(googletest INTERFACE ${CMAKE_BINARY_DIR}/ext/src/gtest/include ${CMAKE_BINARY_DIR}/ext/src/gmock/include)

function(qore_add_common name)
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} ${ARG_LIBS} ${GCC_COVERAGE_LINK})
    target_compile_options(${name} PRIVATE ${GCC_COVERAGE_FLAGS})
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 11)
endfunction(qore_add_common)

function(qore_add_static_library name)
    cmake_parse_arguments(ARG "" "" "LIBS" ${ARGN})
    add_library(${name} STATIC ${ARG_UNPARSED_ARGUMENTS})
    target_link_libraries(${name} ${ARG_LIBS})
    qore_add_common(${name})
endfunction(qore_add_static_library)

function(qore_add_executable name)
    cmake_parse_arguments(ARG "" "" "LIBS" ${ARGN})
    add_executable(${name} ${ARG_UNPARSED_ARGUMENTS})
    target_link_libraries(${name} ${ARG_LIBS})
    qore_add_common(${name})
endfunction(qore_add_executable)

function(qore_add_test baseName)
    set(name "test-${baseName}")
    cmake_parse_arguments(ARG "" "" "LIBS" ${ARGN})
    add_executable(${name} ${ARG_UNPARSED_ARGUMENTS})
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/mock)
    target_link_libraries(${name} ${baseName} ${ARG_LIBS} googletest)
    qore_add_common(${name})
    add_test(${baseName} ${name})
    add_dependencies(check ${name})
endfunction(qore_add_test)

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

if(COVERAGE)
    add_custom_command(TARGET check POST_BUILD
        COMMAND lcov -c -d . -o coverage.info
        COMMAND lcov -r coverage.info */ext/* -o coverage.info
        COMMAND lcov -r coverage.info mock/* -o coverage.info
        COMMAND lcov -r coverage.info c++* -o coverage.info
        COMMAND lcov -r coverage.info */test/Test* -o coverage.info
        COMMAND genhtml coverage.info -o coverage --highlight --demangle-cpp
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif(COVERAGE)

add_subdirectory(http2)
