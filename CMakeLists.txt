cmake_minimum_required(VERSION 3.15..4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(CSC8502-Coursework VERSION 0.1.0 LANGUAGES CXX)

add_subdirectory(src)
add_subdirectory(engine)

