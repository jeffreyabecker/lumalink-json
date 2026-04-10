# cmake/run_compile_fail_test.cmake
#
# Drives a single compile-fail test case.
#
# Expected arguments (set via -D on the cmake command line):
#   COMPILER     – path to the C++ compiler executable
#   INCLUDE_DIRS – pipe-separated list of include directories
#   CASE_FILE    – absolute path to the .cpp file that must fail to compile
#
# Exit behaviour:
#   exits 0      if the compiler rejects the file (expected)
#   FATAL_ERROR  if the compiler accepts the file (unexpected)

cmake_minimum_required(VERSION 3.25)

if(NOT COMPILER)
    message(FATAL_ERROR "COMPILER must be set")
endif()

if(NOT CASE_FILE)
    message(FATAL_ERROR "CASE_FILE must be set")
endif()

# Expand the pipe-separated include list into CMake list form.
string(REPLACE "|" ";" _inc_list "${INCLUDE_DIRS}")

set(_inc_args)
foreach(_dir IN LISTS _inc_list)
    list(APPEND _inc_args "-I${_dir}")
endforeach()

# Detect compiler family by executable name.
get_filename_component(_compiler_name "${COMPILER}" NAME)

if(_compiler_name MATCHES "^cl(\\.exe)?$")
    # MSVC: /c compiles without linking; /nologo suppresses banner noise.
    # Output goes to NUL (Windows null device) to avoid leaving artefacts.
    set(_compile_args /nologo /std:c++latest /EHs- ${_inc_args} /c ${CASE_FILE} /Fo${CMAKE_CURRENT_LIST_DIR}/_cf_discard.obj)
else()
    # GCC / Clang: -fsyntax-only checks without emitting any output file.
    set(_compile_args -std=gnu++23 -fno-exceptions ${_inc_args} -fsyntax-only ${CASE_FILE})
endif()

execute_process(
    COMMAND ${COMPILER} ${_compile_args}
    RESULT_VARIABLE _result
    OUTPUT_QUIET
    ERROR_QUIET
)

if(_result EQUAL 0)
    message(FATAL_ERROR
        "compile-fail case compiled successfully but was expected to fail:\n"
        "  ${CASE_FILE}")
endif()

message(STATUS "compile-fail case correctly rejected: ${CASE_FILE}")
