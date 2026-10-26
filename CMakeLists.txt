cmake_minimum_required(VERSION 3.20)
project(compiler-mini-c LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(minic_frontend
  src/ast_printer.cpp
  src/lexer.cpp
  src/parser.cpp
  src/token.cpp
)
target_include_directories(minic_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(minic src/main.cpp)
target_link_libraries(minic PRIVATE minic_frontend)

include(CTest)

if(BUILD_TESTING)
  add_executable(minic_parser_tests tests/parser_test.cpp)
  target_link_libraries(minic_parser_tests PRIVATE minic_frontend)

  foreach(case IN ITEMS functions empty precedence operators associativity parentheses
                        literals positions syntax_errors declarations parameters calls
                        syntax_only declaration_call_errors conditionals dangling_else
                        loops control_flow_errors)
    add_test(NAME parser.${case} COMMAND minic_parser_tests ${case})
  endforeach()

  add_executable(minic_lexer_tests tests/lexer_test.cpp)
  target_link_libraries(minic_lexer_tests PRIVATE minic_frontend)

  foreach(case IN ITEMS hello empty keywords identifiers integers positions_and_comments
                        operators operator_boundaries division_and_comments
                        invalid_character unterminated_comment)
    add_test(NAME lexer.${case} COMMAND minic_lexer_tests ${case})
  endforeach()

  foreach(case IN ITEMS hello help empty keywords missing_file invalid_character
                        unterminated_comment missing_argument language invalid_logical
                        ast_hello ast_expressions ast_empty ast_syntax_error ast_lexical_error
                        ast_missing_file ast_missing_argument ast_extra_argument ast_calls
                        ast_control_flow ast_language ast_eof_error tokens_invalid_syntax)
    add_test(NAME cli.${case}
      COMMAND ${CMAKE_COMMAND}
        "-DMINIC=$<TARGET_FILE:minic>"
        "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-data"
        "-DCASE=${case}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_test.cmake"
    )
  endforeach()
endif()
