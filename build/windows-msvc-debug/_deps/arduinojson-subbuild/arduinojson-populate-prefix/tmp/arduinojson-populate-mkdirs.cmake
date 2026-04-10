# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-src")
  file(MAKE_DIRECTORY "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-src")
endif()
file(MAKE_DIRECTORY
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-build"
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix"
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/tmp"
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/src/arduinojson-populate-stamp"
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/src"
  "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/src/arduinojson-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/src/arduinojson-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/ode/lumalink-json/build/windows-msvc-debug/_deps/arduinojson-subbuild/arduinojson-populate-prefix/src/arduinojson-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
