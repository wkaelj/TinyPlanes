cmake_minimum_required(VERSION 3.10)

project(tinyplanes LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_compile_options(
  -Wall
  -Wextra
  -Wshadow
  -Wunreachable-code
  -Warray-bounds
  -O3
  -g
  -pedantic
)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
add_compile_options(
  -Xanalyzer
)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")

add_compile_options(
  -fsanitize=address
  -fopenmp
)
add_link_options(
  -fsanitize=address
  -fopenmp
)
endif()

set(CMAKE_MODULE_PATH client;shared;server;libs;)
include(libs/libs.cmake)
include(shared/shared.cmake)
include(client/client.cmake)
include(server/server.cmake)
