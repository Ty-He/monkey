add_executable(test test01.cc ${ccfiles})

message("message: " ${CMAKE_SOURCE_DIR} ${PROJECT_SOURCE_DIR})
target_include_directories(test PRIVATE ${PROJECT_SOURCE_DIR}/include)
