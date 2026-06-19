cmake_minimum_required(VERSION 3.16)

project(refine
    VERSION 1.0.0
    DESCRIPTION "List files in a directory grouped by extension"
    LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Default to an optimized build when the user hasn't chosen a type, so a bare
# `cmake -S . -B build` doesn't silently produce an unoptimized binary.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(refine refine.c)

if(MSVC)
    target_compile_options(refine PRIVATE /W4)
else()
    target_compile_options(refine PRIVATE -Wall -Wextra)
endif()

include(GNUInstallDirs)
install(TARGETS refine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
