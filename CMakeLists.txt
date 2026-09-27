cmake_minimum_required(VERSION 3.12)
project(liblu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(LIBLU_BUILD_TESTS "Build example / smoke test binary" ON)
option(LIBLU_USE_GPERFTOOLS "Link gperftools profiler into the test binary" OFF)

find_package(Threads REQUIRED)

add_library(lu STATIC
    src/lu.cpp
    src/lutypes.cpp
    src/lusocket.cpp
)

target_include_directories(lu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(lu PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
)

target_link_libraries(lu PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(lu PUBLIC ws2_32)
endif()

if(LIBLU_BUILD_TESTS)
    add_subdirectory(test)
    enable_testing()
    add_test(
        NAME lu_smoke
        COMMAND $<TARGET_FILE:lu_test> smoke
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()
