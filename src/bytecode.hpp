#pragma once

#include "type_rules.hpp"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace minic {

enum class OpCode {
  Step,
  LoadConstant,
  LoadLocal,
  StoreLocal,
  ResetLocal,
  CheckIndex,
  LoadElement,
  StoreElement,
  Pop,
  Negate,
  Not,
  Add,
  Subtract,
  Multiply,
  Divide,
  Remainder,
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Jump,
  JumpIfFalse,
  JumpIfTrue,
  Call,
  Return,
  ReturnVoid,
  Unreachable,
};

struct IntegerConstant {
  std::string digits;
  bool negative = false;
};

using BytecodeConstant = std::variant<IntegerConstant, bool>;

struct Instruction {
  OpCode opcode;
  std::size_t operand = 0;
  SourceLocation location;
};

struct BytecodeLocal {
  std::string name;
  Type type;
  SourceLocation location;
  std::size_t array_length = 0;
};

struct BytecodeFunction {
  std::string name;
  Type return_type;
  std::size_t parameter_count = 0;
  SourceLocation location;
  std::vector<BytecodeLocal> locals;
  std::vector<BytecodeConstant> constants;
  std::vector<Instruction> code;
};

struct BytecodeProgram {
  std::vector<BytecodeFunction> functions;
};

const char* opcode_name(OpCode opcode);
std::string print_bytecode(const BytecodeProgram& program);

}  // namespace minic
