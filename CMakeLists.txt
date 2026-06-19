cmake_minimum_required(VERSION 3.16)

project(refine
    VERSION 1.0.0
    DESCRIPTION "List files in a directory grouped by extension"
    LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(refine refine.c)

if(MSVC)
    target_compile_options(refine PRIVATE /W4)
else()
    target_compile_options(refine PRIVATE -Wall -Wextra)
endif()

include(GNUInstallDirs)
install(TARGETS refine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
