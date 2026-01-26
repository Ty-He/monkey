cmake_minimum_required(VERSION 3.15)
project(monkey)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(repl)

add_subdirectory(test)

add_executable(monkey main.cc)

target_link_libraries(monkey PRIVATE repl)



