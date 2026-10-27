cmake_minimum_required(VERSION 3.20)

get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(NOT DEFINED WORK_DIR)
  set(WORK_DIR "${SOURCE_DIR}/build-llvm-lab")
endif()
get_filename_component(WORK_DIR "${WORK_DIR}" ABSOLUTE)
find_program(CLANG_EXECUTABLE NAMES clang REQUIRED)
file(MAKE_DIRECTORY "${WORK_DIR}")

function(run_step label)
  execute_process(
    COMMAND ${ARGN}
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 30
  )
  if(NOT "${status}" STREQUAL "0")
    message(FATAL_ERROR "The command failed (${status}): ${label}\n${output}${error}")
  endif()
  if(NOT "${error}" STREQUAL "")
    message(FATAL_ERROR "The command wrote to standard error: ${label}\n${error}")
  endif()
  set(STEP_OUTPUT "${output}" PARENT_SCOPE)
endfunction()

run_step("Read the Clang version" "${CLANG_EXECUTABLE}" --version)
if(NOT STEP_OUTPUT MATCHES "[Cc]lang")
  message(FATAL_ERROR "CLANG_EXECUTABLE must select Clang.")
endif()
file(WRITE "${WORK_DIR}/clang-version.txt" "${STEP_OUTPUT}")
file(WRITE "${WORK_DIR}/results.txt" "")

foreach(level IN ITEMS 0 1)
  set(ir "${WORK_DIR}/sum.O${level}.ll")
  set(native "${WORK_DIR}/native.O${level}")
  set(from_ir "${WORK_DIR}/from-ir.O${level}")
  run_step("Generate IR at -O${level}"
    "${CLANG_EXECUTABLE}" -std=c17 -Wall -Wextra -Wpedantic
    -S -emit-llvm -fno-discard-value-names "-O${level}"
    examples/llvm/sum.c -o "${ir}")
  run_step("Compile C at -O${level}"
    "${CLANG_EXECUTABLE}" -std=c17 -Wall -Wextra -Wpedantic "-O${level}"
    examples/llvm/sum.c examples/llvm/driver.c -o "${native}")
  run_step("Execute the program built from C at -O${level}" "${native}")
  if(NOT STEP_OUTPUT STREQUAL "10\n")
    message(FATAL_ERROR "Expected output 10 from C at -O${level}. Received: ${STEP_OUTPUT}")
  endif()
  run_step("Compile IR at -O${level}"
    "${CLANG_EXECUTABLE}" -std=c17 -Wall -Wextra -Wpedantic "-O${level}"
    "${ir}" examples/llvm/driver.c -o "${from_ir}")
  run_step("Execute the program built from IR at -O${level}" "${from_ir}")
  if(NOT STEP_OUTPUT STREQUAL "10\n")
    message(FATAL_ERROR "Expected output 10 from IR at -O${level}. Received: ${STEP_OUTPUT}")
  endif()
  file(APPEND "${WORK_DIR}/results.txt" "-O${level}: C and IR print 10 and pass all 5 cases.\n")
  message(STATUS "-O${level}: C and IR print 10 and pass all 5 cases.")
endforeach()

message(STATUS "The LLVM laboratory passed all 4 native execution checks.")
