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
  src/name_resolution.cpp
  src/parser.cpp
  src/resolution_printer.cpp
  src/symbol_table.cpp
  src/token.cpp
  src/type_rules.cpp
  src/type_checker.cpp
)
target_include_directories(minic_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(minic_runtime src/runtime.cpp src/interpreter.cpp)
target_link_libraries(minic_runtime PUBLIC minic_frontend)

add_executable(minic src/main.cpp)
target_link_libraries(minic PRIVATE minic_runtime)

include(CTest)

if(BUILD_TESTING)
  add_executable(minic_interpreter_tests tests/interpreter_test.cpp)
  target_link_libraries(minic_interpreter_tests PRIVATE minic_runtime)
  foreach(case IN ITEMS arithmetic booleans assignments scopes initialization evaluation_order
                        short_circuit returns void_returns entry_selection numeric_limits
                        numeric_errors unsupported skipped_nodes static_priority ownership)
    add_test(NAME interpreter.${case} COMMAND minic_interpreter_tests ${case})
  endforeach()

  add_executable(minic_runtime_tests tests/runtime_test.cpp)
  target_link_libraries(minic_runtime_tests PRIVATE minic_runtime)
  foreach(case IN ITEMS values literals arithmetic overflow division logic)
    add_test(NAME runtime.${case} COMMAND minic_runtime_tests ${case})
  endforeach()

  add_executable(minic_type_checker_tests tests/type_checker_test.cpp)
  target_link_libraries(minic_type_checker_tests PRIVATE minic_frontend)
  foreach(case IN ITEMS empty expression_types declarations assignments operators shadowing
                        calls arity argument_types void_uses conditions returns return_paths
                        unreachable stage_priority names_and_numbers)
    add_test(NAME types.${case} COMMAND minic_type_checker_tests ${case})
  endforeach()

  add_executable(minic_type_rules_tests tests/type_rules_test.cpp)
  target_link_libraries(minic_type_rules_tests PRIVATE minic_frontend)
  foreach(case IN ITEMS syntax arithmetic comparisons logic equality)
    add_test(NAME type_rules.${case} COMMAND minic_type_rules_tests ${case})
  endforeach()

  add_executable(minic_resolution_tests tests/name_resolution_test.cpp)
  target_link_libraries(minic_resolution_tests PRIVATE minic_frontend)
  foreach(case IN ITEMS empty bindings undeclared redeclarations shadowing isolation
                        visibility functions categories traversal names_only)
    add_test(NAME resolution.${case} COMMAND minic_resolution_tests ${case})
  endforeach()

  add_executable(minic_symbol_table_tests tests/symbol_table_test.cpp)
  target_link_libraries(minic_symbol_table_tests PRIVATE minic_frontend)
  foreach(case IN ITEMS declarations scopes redeclarations stable_ids)
    add_test(NAME symbols.${case} COMMAND minic_symbol_table_tests ${case})
  endforeach()

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
                        ast_control_flow ast_language ast_eof_error tokens_invalid_syntax
                        resolve_hello resolve_scopes resolve_recursion resolve_empty
                        resolve_undeclared resolve_redeclared resolve_category
                        resolve_syntax_error resolve_lexical_error resolve_missing_file
                        resolve_missing_argument resolve_extra_argument resolve_names_only
                        check_hello check_scopes check_recursion check_calls check_control_flow
                        check_lexer check_types check_type_error check_arity check_return_path
                        check_void_parameter check_syntax_error check_lexical_error
                        check_resolution_error check_missing_file check_missing_argument
                        check_extra_argument check_empty
                        run_hello run_interpreter run_short_circuit run_void run_runtime_error
                        run_division_zero run_overflow run_literal_range run_minimum
                        run_missing_entry run_entry_parameters run_unsupported_call
                        run_unsupported_if run_unsupported_while run_type_error
                        run_resolution_error run_syntax_error run_lexical_error
                        run_missing_file run_missing_argument run_extra_argument)
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
