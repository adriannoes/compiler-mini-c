#pragma once

#include "bytecode.hpp"

namespace minic {

BytecodeProgram optimize_bytecode(const BytecodeProgram& program);

}  // namespace minic
