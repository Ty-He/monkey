cmake_minimum_required(VERSION 3.15)
project(monkey)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

aux_source_directory(${PROJECT_SOURCE_DIR}/src ccfiles)

add_subdirectory(test)
add_executable(monkey main.cc ${ccfiles})

target_include_directories(monkey PRIVATE ${PROJECT_SOURCE_DIR}/include)
