cmake_minimum_required(VERSION 3.25)

project(LumaLinkJson
    VERSION 0.1.0
    DESCRIPTION "LumaLink JSON serialization library"
    LANGUAGES CXX
)

# Detect whether this is the top-level project so options default sensibly
# when the library is consumed via add_subdirectory.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(_lumalink_json_top_level ON)
else()
    set(_lumalink_json_top_level OFF)
endif()

option(LUMALINK_JSON_BUILD_TESTS "Build the native test suite"  ${_lumalink_json_top_level})
option(LUMALINK_JSON_INSTALL     "Generate install rules"        ${_lumalink_json_top_level})

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
include(FetchContent)

# ArduinoJson – used by library headers; always fetched at build time.
# The arduinojson_SOURCE_DIR variable is set by FetchContent and forwarded to
# test/ so compile-fail tests can locate the headers without generator
# expression evaluation.
FetchContent_Declare(
    arduinojson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
    GIT_TAG        v7.4.2
)
set(ARDUINOJSON_ENABLE_ARDUINO OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(arduinojson)

# Unity – test framework; only fetched when the test suite is enabled.
if(LUMALINK_JSON_BUILD_TESTS)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.1
    )
    FetchContent_MakeAvailable(unity)
endif()

# ---------------------------------------------------------------------------
# Interface library target
# ---------------------------------------------------------------------------
add_library(lumalink_json INTERFACE)
add_library(LumaLinkJson::LumaLinkJson ALIAS lumalink_json)

target_include_directories(lumalink_json
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libs/pfr>
        $<INSTALL_INTERFACE:include>
)

# Require C++23; CMAKE_CXX_EXTENSIONS stays ON (the default) so GCC uses
# gnu++23, matching the -std=gnu++23 flag in the PlatformIO configuration.
target_compile_features(lumalink_json INTERFACE cxx_std_23)

target_link_libraries(lumalink_json INTERFACE ArduinoJson)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(LUMALINK_JSON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
if(LUMALINK_JSON_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Export the interface target.
    install(TARGETS lumalink_json
        EXPORT LumaLinkJsonTargets
    )

    # Install all public headers preserving the src/ sub-layout so consumers
    # can use the same #include paths as the build tree.
    install(DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )

    # Export file referenced by LumaLinkJsonConfig.cmake.
    install(EXPORT LumaLinkJsonTargets
        FILE       LumaLinkJsonTargets.cmake
        NAMESPACE  LumaLinkJson::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LumaLinkJson
    )

    # Config and version files.
    configure_package_config_file(
        cmake/LumaLinkJsonConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LumaLinkJsonConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LumaLinkJson
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LumaLinkJsonVersion.cmake
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LumaLinkJsonConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LumaLinkJsonVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LumaLinkJson
    )
endif()
