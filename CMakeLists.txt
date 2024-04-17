cmake_minimum_required(VERSION 3.10)

project(tinyplanes LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED YES)


set(CMAKE_MODULE_PATH client;shared;server;libs;)
include(libs)
include(shared)
include(client)
include(server)
