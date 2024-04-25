cmake_minimum_required(VERSION 3.10)

project(tinyplanes LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)
add_compile_options(
  -Wall
  -Wextra
  -Wshadow
  -Wunreachable-code
  -Warray-bounds
  -O3
  -g
  -fanalyzer
  -fopenmp
)
add_link_options(
  -fopenmp
)


set(CMAKE_MODULE_PATH client;shared;server;libs;)
include(libs)
include(shared)
include(client)
include(server)
