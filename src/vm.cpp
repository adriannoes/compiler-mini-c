#include "vm.hpp"

#include <optional>
#include <utility>

namespace minic {
namespace {

[[noreturn]] void invalid(const char* message) {
  throw std::invalid_argument(std::string("Invalid bytecode: ") + message);
}

Type value_type(const Value& value) {
  if (std::holds_alternative<std::int64_t>(value)) {
    return Type::Int;
  }
  if (std::holds_alternative<bool>(value)) {
    return Type::Bool;
  }
  invalid("A stack operand has no value.");
}

void require_type(const Value& value, Type expected) {
  if (value_type(value) != expected) {
    invalid("The operand type does not match the instruction.");
  }
}

TokenKind binary_token(OpCode opcode) {
  switch (opcode) {
    case OpCode::Add: return TokenKind::Plus;
    case OpCode::Subtract: return TokenKind::Minus;
    case OpCode::Multiply: return TokenKind::Star;
    case OpCode::Divide: return TokenKind::Slash;
    case OpCode::Remainder: return TokenKind::Percent;
    case OpCode::Equal: return TokenKind::EqualEqual;
    case OpCode::NotEqual: return TokenKind::BangEqual;
    case OpCode::Less: return TokenKind::Less;
    case OpCode::LessEqual: return TokenKind::LessEqual;
    case OpCode::Greater: return TokenKind::Greater;
    case OpCode::GreaterEqual: return TokenKind::GreaterEqual;
    default: invalid("The instruction is not a binary operation.");
  }
}

struct Frame {
  std::size_t function;
  std::size_t pc = 0;
  std::size_t base = 0;
  std::vector<std::optional<Value>> locals;
};

class VirtualMachine {
 public:
  VirtualMachine(const BytecodeProgram& program, ExecutionLimits limits) : program_(program), limits_(limits) {}

  Value run(const std::string& entry) {
    std::size_t index = 0;
    while (index < program_.functions.size() && program_.functions[index].name != entry) {
      ++index;
    }
    if (index == program_.functions.size()) {
      throw RuntimeError({1, 1}, "The entry function \"" + entry + "\" does not exist.");
    }
    const auto& function = program_.functions[index];
    if (function.parameter_count != 0) {
      throw RuntimeError(function.location, "The entry function \"" + entry + "\" must not have parameters.");
    }
    frames_.push_back({index, 0, 0, std::vector<std::optional<Value>>(function.locals.size())});
    while (true) {
      const auto instruction = next();
      switch (instruction.opcode) {
        case OpCode::Step:
          if (steps_ == limits_.max_steps) {
            throw RuntimeError(instruction.location, "The execution step limit was reached.");
          }
          ++steps_;
          break;
        case OpCode::LoadConstant: load_constant(instruction); break;
        case OpCode::LoadLocal: {
          const auto& value = slot(instruction.operand);
          if (!value) {
            throw RuntimeError(instruction.location, "The variable \"" + local(instruction.operand).name + "\" has no value.");
          }
          values_.push_back(*value);
          break;
        }
        case OpCode::StoreLocal:
          require_type(top(), local(instruction.operand).type);
          slot(instruction.operand) = top();
          break;
        case OpCode::ResetLocal: slot(instruction.operand).reset(); break;
        case OpCode::Pop: pop(); break;
        case OpCode::Negate:
        case OpCode::Not: {
          const auto operand = pop();
          require_type(operand, instruction.opcode == OpCode::Negate ? Type::Int : Type::Bool);
          values_.push_back(detail::runtime_unary(instruction.opcode == OpCode::Negate ? TokenKind::Minus : TokenKind::Bang,
                                                   operand, instruction.location));
          break;
        }
        case OpCode::Add:
        case OpCode::Subtract:
        case OpCode::Multiply:
        case OpCode::Divide:
        case OpCode::Remainder:
        case OpCode::Equal:
        case OpCode::NotEqual:
        case OpCode::Less:
        case OpCode::LessEqual:
        case OpCode::Greater:
        case OpCode::GreaterEqual: binary(instruction); break;
        case OpCode::Jump: jump(instruction.operand); break;
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue: {
          const auto condition = pop();
          require_type(condition, Type::Bool);
          if (std::get<bool>(condition) == (instruction.opcode == OpCode::JumpIfTrue)) {
            jump(instruction.operand);
          }
          break;
        }
        case OpCode::Call:
          throw RuntimeError(instruction.location, "Function calls are not available in this VM increment.");
        case OpCode::Return:
        case OpCode::ReturnVoid: {
          const auto result = finish(instruction.opcode == OpCode::Return);
          if (result) {
            return *result;
          }
          break;
        }
        case OpCode::Unreachable:
          throw std::logic_error("A typed function ended without a return value.");
        default: invalid("The opcode does not exist.");
      }
    }
  }

 private:
  const BytecodeFunction& function() const { return program_.functions[frames_.back().function]; }

  Instruction next() {
    auto& frame = frames_.back();
    const auto& code = function().code;
    if (frame.pc >= code.size()) {
      invalid("Control flow left the instruction vector.");
    }
    return code[frame.pc++];
  }

  const BytecodeLocal& local(std::size_t index) const {
    if (index >= function().locals.size()) {
      invalid("The local index is outside the function.");
    }
    return function().locals[index];
  }

  std::optional<Value>& slot(std::size_t index) {
    local(index);
    return frames_.back().locals[index];
  }

  const Value& top() const {
    if (values_.size() <= frames_.back().base) {
      invalid("The instruction underflows the value stack.");
    }
    return values_.back();
  }

  Value pop() {
    auto value = top();
    values_.pop_back();
    return value;
  }

  void load_constant(const Instruction& instruction) {
    const auto& constants = function().constants;
    if (instruction.operand >= constants.size()) {
      invalid("The constant index is outside the function.");
    }
    const auto& value = constants[instruction.operand];
    if (const auto* integer = std::get_if<IntegerConstant>(&value)) {
      values_.push_back(detail::parse_integer(integer->digits, integer->negative, instruction.location));
    } else {
      values_.push_back(std::get<bool>(value));
    }
  }

  void binary(const Instruction& instruction) {
    const auto right = pop();
    const auto left = pop();
    require_type(left, instruction.opcode == OpCode::Equal || instruction.opcode == OpCode::NotEqual
                           ? value_type(right) : Type::Int);
    if (instruction.opcode != OpCode::Equal && instruction.opcode != OpCode::NotEqual) {
      require_type(right, Type::Int);
    }
    values_.push_back(detail::runtime_binary(binary_token(instruction.opcode), left, right, instruction.location));
  }

  void jump(std::size_t target) {
    if (target >= function().code.size()) {
      invalid("The jump target is outside the function.");
    }
    frames_.back().pc = target;
  }

  std::optional<Value> finish(bool has_value) {
    Value result;
    if (has_value) {
      result = pop();
      require_type(result, function().return_type);
    } else if (function().return_type != Type::Void) {
      invalid("A non-void function returned no value.");
    }
    if (values_.size() != frames_.back().base) {
      invalid("A return left temporary values on the stack.");
    }
    frames_.pop_back();
    if (frames_.empty()) {
      return result;
    }
    if (has_value) {
      values_.push_back(std::move(result));
    }
    return std::nullopt;
  }

  const BytecodeProgram& program_;
  ExecutionLimits limits_;
  std::vector<Frame> frames_;
  std::vector<Value> values_;
  std::size_t steps_ = 0;
};

}  // namespace

Value execute_bytecode(const BytecodeProgram& program, const std::string& entry) {
  return execute_bytecode(program, entry, ExecutionLimits{});
}

Value execute_bytecode(const BytecodeProgram& program, const std::string& entry, ExecutionLimits limits) {
  if (limits.max_steps == 0 || limits.max_call_depth == 0) {
    throw std::invalid_argument("Execution limits must be greater than zero.");
  }
  return VirtualMachine(program, limits).run(entry);
}

}  // namespace minic
