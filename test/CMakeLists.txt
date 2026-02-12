add_executable(test test04.cc)

# message("message: " ${CMAKE_CURRENT_SOURCE_DIR} " " ${PROJECT_SOURCE_DIR})
target_link_libraries(test PRIVATE lexer)
target_link_libraries(test PRIVATE parser)
target_link_libraries(test PRIVATE evaluator)
