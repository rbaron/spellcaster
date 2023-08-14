cmake_minimum_required(VERSION 3.20.0)

# Pull in the dts/ and boards/ from sclib.
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../sclib)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../sclib)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(caster-test)

target_sources(app PRIVATE src/main.c)

add_subdirectory(../../sclib sclib)
target_include_directories(app PRIVATE ../../sclib/include)
target_link_libraries(app PUBLIC sclib)
