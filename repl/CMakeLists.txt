add_library(repl INTERFACE)
target_include_directories(repl INTERFACE ./include)
# target_link_libraries(repl INTERFACE lexer)
target_link_libraries(repl INTERFACE evaluator)
