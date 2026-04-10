# ---------------------------------------------------------------------------
# Native test executable  (CMAKE-06, CMAKE-07, CMAKE-08)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE lumalink_json_test_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/test_native/*.cpp"
)

add_executable(lumalink_json_tests ${lumalink_json_test_sources})

target_link_libraries(lumalink_json_tests
    PRIVATE
        LumaLinkJson::LumaLinkJson
        unity
)

target_include_directories(lumalink_json_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_native
        ${CMAKE_CURRENT_SOURCE_DIR}/test_native/support
)

# Match the UNIT_TEST / NATIVE_TESTS guards used throughout the test sources.
target_compile_definitions(lumalink_json_tests
    PRIVATE
        UNIT_TEST
        NATIVE_TESTS
)

# Per-compiler warning and exception-handling flags, applied privately so they
# do not propagate to consumers of the interface library.
target_compile_options(lumalink_json_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /EHs-
        >
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall
            -Wextra
            -Wno-unused-parameter
            -fno-exceptions
        >
)

add_test(
    NAME    lumalink_json_tests
    COMMAND lumalink_json_tests
)

# ---------------------------------------------------------------------------
# Compile-fail tests  (CMAKE-09 / CMAKE-10)
#
# Each .cpp under test/compile_fail/ is expected to be rejected by the
# compiler.  A cmake -P script drives each case and exits non-zero if the
# file compiles successfully (unexpected).
#
# arduinojson_SOURCE_DIR is set by FetchContent_MakeAvailable(arduinojson)
# in the root CMakeLists.txt and is available here as a normal variable.
# Using the source directory directly avoids generator-expression evaluation
# problems when building the -I argument list.
# ---------------------------------------------------------------------------
file(GLOB compile_fail_cases
    "${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/*.cpp"
)

foreach(case_file IN LISTS compile_fail_cases)
    get_filename_component(case_name "${case_file}" NAME_WE)

    # Build a pipe-separated include path; pipe is safe because no include
    # directory in this project will contain that character.
    string(JOIN "|" _inc_dirs
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/libs/pfr"
        "${CMAKE_CURRENT_SOURCE_DIR}/compile_fail"
        "${arduinojson_SOURCE_DIR}/src"
    )

    add_test(
        NAME "compile_fail/${case_name}"
        COMMAND ${CMAKE_COMMAND}
            -DCOMPILER=${CMAKE_CXX_COMPILER}
            "-DINCLUDE_DIRS=${_inc_dirs}"
            -DCASE_FILE=${case_file}
            -P "${CMAKE_SOURCE_DIR}/cmake/run_compile_fail_test.cmake"
    )
endforeach()
