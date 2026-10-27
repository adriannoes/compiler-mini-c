#pragma once

#include "bytecode.hpp"
#include "runtime.hpp"

namespace minic {

Value execute_bytecode(const BytecodeProgram& program, const std::string& entry = "main");
Value execute_bytecode(const BytecodeProgram& program, const std::string& entry, ExecutionLimits limits);

}  // namespace minic
