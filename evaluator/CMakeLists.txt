add_library(evaluator INTERFACE)
target_include_directories(evaluator INTERFACE ./include)
target_link_libraries(evaluator INTERFACE parser)
