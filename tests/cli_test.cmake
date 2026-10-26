file(MAKE_DIRECTORY "${WORK_DIR}")
set(input "${WORK_DIR}/${CASE}.mc")
set(expected_exit 0)
set(expected_error "")

if(CASE STREQUAL "hello")
  set(input "${SOURCE_DIR}/examples/hello.mc")
  file(READ "${SOURCE_DIR}/tests/hello.tokens" expected_output)
elseif(CASE STREQUAL "help")
  set(arguments --help)
elseif(CASE STREQUAL "empty")
  file(WRITE "${input}" "")
  set(expected_output "1:1 EOF \"\"\n")
elseif(CASE STREQUAL "keywords")
  file(WRITE "${input}" "bool void if else while true false")
  set(expected_output [=[1:1 BOOL "bool"
1:6 VOID "void"
1:11 IF "if"
1:14 ELSE "else"
1:19 WHILE "while"
1:25 TRUE "true"
1:30 FALSE "false"
1:35 EOF ""
]=])
elseif(CASE STREQUAL "missing_file")
  file(REMOVE "${input}")
  set(expected_exit 1)
  set(expected_output "")
  set(expected_error "${input}: error: Cannot open file.\n")
elseif(CASE STREQUAL "invalid_character")
  file(WRITE "${input}" "int main() {\n  @\n}\n")
  set(expected_exit 1)
  set(expected_output "")
  set(expected_error "${input}:2:3: error: Unexpected character \"@\".\n")
elseif(CASE STREQUAL "unterminated_comment")
  file(WRITE "${input}" "\n  /* unfinished\n")
  set(expected_exit 1)
  set(expected_output "")
  set(expected_error "${input}:2:3: error: Unterminated block comment.\n")
elseif(CASE STREQUAL "missing_argument")
  set(arguments --tokens)
  set(expected_exit 1)
  set(expected_output "")
  set(expected_error "minic: error: Expected --tokens <file>. Use --help for usage.\n")
else()
  message(FATAL_ERROR "Unknown CLI test: ${CASE}")
endif()

if(NOT DEFINED arguments)
  set(arguments --tokens "${input}")
endif()

execute_process(
  COMMAND "${MINIC}" ${arguments}
  RESULT_VARIABLE exit_code
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)

if(NOT "${exit_code}" STREQUAL "${expected_exit}")
  message(FATAL_ERROR "Expected exit ${expected_exit}, received ${exit_code}.\n${output}${error}")
endif()

if(NOT "${error}" STREQUAL "${expected_error}")
  message(FATAL_ERROR "Expected stderr:\n${expected_error}\nReceived stderr:\n${error}")
endif()

if(CASE STREQUAL "help")
  string(FIND "${output}" "minic --tokens <file>" usage_position)
  if(usage_position EQUAL -1)
    message(FATAL_ERROR "Help does not describe the token command.")
  endif()
elseif(NOT "${output}" STREQUAL "${expected_output}")
  message(FATAL_ERROR "Expected stdout:\n${expected_output}\nReceived stdout:\n${output}")
endif()
