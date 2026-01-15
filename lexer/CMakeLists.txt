aux_source_directory(./src lexer_src)
add_library(lexer STATIC ${lexer_src})
target_include_directories(lexer PUBLIC ./include)
