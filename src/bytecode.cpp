#include "bytecode.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace minic {

const char* opcode_name(OpCode opcode) {
  switch (opcode) {
    case OpCode::Step: return "STEP";
    case OpCode::LoadConstant: return "LOAD_CONSTANT";
    case OpCode::LoadLocal: return "LOAD_LOCAL";
    case OpCode::StoreLocal: return "STORE_LOCAL";
    case OpCode::ResetLocal: return "RESET_LOCAL";
    case OpCode::CheckIndex: return "CHECK_INDEX";
    case OpCode::LoadElement: return "LOAD_ELEMENT";
    case OpCode::StoreElement: return "STORE_ELEMENT";
    case OpCode::Pop: return "POP";
    case OpCode::Negate: return "NEGATE";
    case OpCode::Not: return "NOT";
    case OpCode::Add: return "ADD";
    case OpCode::Subtract: return "SUBTRACT";
    case OpCode::Multiply: return "MULTIPLY";
    case OpCode::Divide: return "DIVIDE";
    case OpCode::Remainder: return "REMAINDER";
    case OpCode::Equal: return "EQUAL";
    case OpCode::NotEqual: return "NOT_EQUAL";
    case OpCode::Less: return "LESS";
    case OpCode::LessEqual: return "LESS_EQUAL";
    case OpCode::Greater: return "GREATER";
    case OpCode::GreaterEqual: return "GREATER_EQUAL";
    case OpCode::Jump: return "JUMP";
    case OpCode::JumpIfFalse: return "JUMP_IF_FALSE";
    case OpCode::JumpIfTrue: return "JUMP_IF_TRUE";
    case OpCode::Call: return "CALL";
    case OpCode::Return: return "RETURN";
    case OpCode::ReturnVoid: return "RETURN_VOID";
    case OpCode::Unreachable: return "UNREACHABLE";
  }
  throw std::invalid_argument("The opcode does not exist.");
}

namespace {

bool has_operand(OpCode opcode) {
  switch (opcode) {
    case OpCode::LoadConstant:
    case OpCode::LoadLocal:
    case OpCode::StoreLocal:
    case OpCode::ResetLocal:
    case OpCode::CheckIndex:
    case OpCode::LoadElement:
    case OpCode::StoreElement:
    case OpCode::Jump:
    case OpCode::JumpIfFalse:
    case OpCode::JumpIfTrue:
    case OpCode::Call: return true;
    default: return false;
  }
}

}  // namespace

std::string print_bytecode(const BytecodeProgram& program) {
  std::ostringstream output;
  output << "bytecode\n";
  for (std::size_t index = 0; index < program.functions.size(); ++index) {
    const auto& function = program.functions[index];
    output << "function " << index << ' ' << std::quoted(function.name) << ' ' << type_name(function.return_type)
           << " parameters=" << function.parameter_count << " @" << function.location.line << ':'
           << function.location.column << '\n';
    for (std::size_t local = 0; local < function.locals.size(); ++local) {
      const auto& slot = function.locals[local];
      output << "  local " << local << ' ' << std::quoted(slot.name) << ' ' << type_name(slot.type);
      if (slot.array_length != 0) {
        output << '[' << slot.array_length << ']';
      }
      output << " @" << slot.location.line << ':' << slot.location.column << '\n';
    }
    for (std::size_t constant = 0; constant < function.constants.size(); ++constant) {
      output << "  constant " << constant;
      if (const auto* integer = std::get_if<IntegerConstant>(&function.constants[constant])) {
        output << " int " << std::quoted((integer->negative ? "-" : "") + integer->digits);
      } else {
        output << " bool " << (std::get<bool>(function.constants[constant]) ? "true" : "false");
      }
      output << '\n';
    }
    for (std::size_t offset = 0; offset < function.code.size(); ++offset) {
      const auto& instruction = function.code[offset];
      output << "  " << offset << ' ' << opcode_name(instruction.opcode);
      if (has_operand(instruction.opcode)) {
        output << ' ' << instruction.operand;
      }
      output << " @" << instruction.location.line << ':' << instruction.location.column << '\n';
    }
    output << "end\n";
  }
  return output.str();
}

}  // namespace minic
