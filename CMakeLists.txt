cmake_minimum_required(VERSION 3.20)
project(compiler-mini-c LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(minic_frontend
  src/lexer.cpp
  src/token.cpp
)
target_include_directories(minic_frontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(minic src/main.cpp)
target_link_libraries(minic PRIVATE minic_frontend)

include(CTest)

if(BUILD_TESTING)
  add_executable(minic_lexer_tests tests/lexer_test.cpp)
  target_link_libraries(minic_lexer_tests PRIVATE minic_frontend)

  foreach(case IN ITEMS hello empty identifiers integers positions_and_comments
                        invalid_character unterminated_comment)
    add_test(NAME lexer.${case} COMMAND minic_lexer_tests ${case})
  endforeach()

  foreach(case IN ITEMS hello help empty missing_file invalid_character
                        unterminated_comment missing_argument)
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
