#include "bytecode_optimizer.hpp"

#include "runtime.hpp"

#include <optional>
#include <utility>

namespace minic {
namespace {

bool jump(OpCode opcode) {
  return opcode == OpCode::Jump || opcode == OpCode::JumpIfFalse || opcode == OpCode::JumpIfTrue;
}

bool terminal(OpCode opcode) {
  return opcode == OpCode::Return || opcode == OpCode::ReturnVoid || opcode == OpCode::Unreachable;
}

std::vector<bool> leaders(const BytecodeFunction& function) {
  std::vector<bool> result(function.code.size(), false);
  if (!result.empty()) {
    result[0] = true;
  }
  for (std::size_t i = 0; i < function.code.size(); ++i) {
    const auto& instruction = function.code[i];
    if (jump(instruction.opcode)) {
      result.at(instruction.operand) = true;
    }
    if ((jump(instruction.opcode) || terminal(instruction.opcode)) && i + 1 < result.size()) {
      result[i + 1] = true;
    }
  }
  return result;
}

using Edits = std::vector<std::optional<Instruction>>;

void compact(BytecodeFunction& function, const Edits& edits) {
  std::vector<std::size_t> offsets(edits.size());
  std::vector<Instruction> code;
  for (std::size_t i = 0; i < edits.size(); ++i) {
    offsets[i] = code.size();
    if (edits[i]) {
      code.push_back(*edits[i]);
    }
  }
  for (auto& instruction : code) {
    if (jump(instruction.opcode)) {
      instruction.operand = offsets.at(instruction.operand);
      if (instruction.operand >= code.size()) {
        throw std::logic_error("An optimized jump has no target instruction.");
      }
    }
  }
  function.code = std::move(code);
}

std::optional<Value> constant_value(const BytecodeFunction& function, const Instruction& instruction) {
  const auto& value = function.constants.at(instruction.operand);
  if (const auto* boolean = std::get_if<bool>(&value)) {
    return *boolean;
  }
  const auto& integer = std::get<IntegerConstant>(value);
  try {
    return detail::parse_integer(integer.digits, integer.negative, instruction.location);
  } catch (const RuntimeError&) {
    return std::nullopt;
  }
}

BytecodeConstant constant(const Value& value) {
  if (const auto* boolean = std::get_if<bool>(&value)) {
    return *boolean;
  }
  auto text = std::to_string(std::get<std::int64_t>(value));
  const bool negative = text.front() == '-';
  if (negative) {
    text.erase(0, 1);
  }
  return IntegerConstant{std::move(text), negative};
}

std::optional<TokenKind> binary_token(OpCode opcode) {
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
    default: return std::nullopt;
  }
}

struct KnownValue {
  Value value;
  std::size_t producer;
};

bool local_rewrites(BytecodeFunction& function) {
  const auto entries = leaders(function);
  Edits edits(function.code.begin(), function.code.end());
  std::vector<KnownValue> known;
  bool changed = false;
  for (std::size_t i = 0; i < function.code.size(); ++i) {
    if (entries[i]) {
      known.clear();
    }
    const auto instruction = function.code[i];
    if (instruction.opcode == OpCode::Step) {
      continue;
    }
    if (instruction.opcode == OpCode::LoadConstant) {
      const auto value = constant_value(function, instruction);
      if (value) {
        known.push_back({*value, i});
      } else {
        known.clear();
      }
      continue;
    }
    if (instruction.opcode == OpCode::Pop && !known.empty()) {
      edits[known.back().producer].reset();
      edits[i].reset();
      known.pop_back();
      changed = true;
      continue;
    }
    if ((instruction.opcode == OpCode::JumpIfFalse || instruction.opcode == OpCode::JumpIfTrue) &&
        !known.empty() && std::holds_alternative<bool>(known.back().value)) {
      const bool taken = std::get<bool>(known.back().value) == (instruction.opcode == OpCode::JumpIfTrue);
      edits[known.back().producer].reset();
      if (taken) {
        edits[i] = Instruction{OpCode::Jump, instruction.operand, instruction.location};
      } else {
        edits[i].reset();
      }
      known.clear();
      changed = true;
      continue;
    }
    if (instruction.opcode == OpCode::Jump && instruction.operand == i + 1) {
      edits[i].reset();
      known.clear();
      changed = true;
      continue;
    }
    const bool unary = instruction.opcode == OpCode::Negate || instruction.opcode == OpCode::Not;
    const auto binary = binary_token(instruction.opcode);
    const std::size_t count = unary ? 1 : 2;
    if ((!unary && !binary) || known.size() < count) {
      known.clear();
      continue;
    }
    const auto first = known.size() - count;
    std::optional<Value> result;
    try {
      if (unary) {
        result = detail::runtime_unary(instruction.opcode == OpCode::Negate ? TokenKind::Minus : TokenKind::Bang,
                                       known[first].value, instruction.location);
      } else {
        result = detail::runtime_binary(*binary, known[first].value, known[first + 1].value, instruction.location);
      }
    } catch (const RuntimeError&) {
      known.clear();
      continue;
    }
    for (std::size_t operand = first; operand < known.size(); ++operand) {
      edits[known[operand].producer].reset();
    }
    const auto index = function.constants.size();
    function.constants.push_back(constant(*result));
    edits[i] = Instruction{OpCode::LoadConstant, index, instruction.location};
    known.resize(first);
    known.push_back({*result, i});
    changed = true;
  }
  if (changed) {
    compact(function, edits);
  }
  return changed;
}

bool thread_jumps(BytecodeFunction& function) {
  bool changed = false;
  for (auto& instruction : function.code) {
    if (!jump(instruction.opcode)) {
      continue;
    }
    auto target = instruction.operand;
    for (std::size_t hops = 0; hops < function.code.size(); ++hops) {
      const auto& next = function.code.at(target);
      if (next.opcode != OpCode::Jump || next.operand == target) {
        break;
      }
      target = next.operand;
    }
    if (target != instruction.operand) {
      instruction.operand = target;
      changed = true;
    }
  }
  return changed;
}

bool remove_unreachable(BytecodeFunction& function) {
  if (function.code.empty()) {
    return false;
  }
  std::vector<bool> reachable(function.code.size(), false);
  std::vector<std::size_t> pending{0};
  while (!pending.empty()) {
    const auto offset = pending.back();
    pending.pop_back();
    if (reachable.at(offset)) {
      continue;
    }
    reachable[offset] = true;
    const auto& instruction = function.code[offset];
    if (jump(instruction.opcode)) {
      pending.push_back(instruction.operand);
    }
    if (instruction.opcode != OpCode::Jump && !terminal(instruction.opcode)) {
      if (offset + 1 == function.code.size()) {
        throw std::logic_error("Optimized control flow leaves the function.");
      }
      pending.push_back(offset + 1);
    }
  }
  Edits edits(function.code.begin(), function.code.end());
  bool changed = false;
  for (std::size_t i = 0; i < edits.size(); ++i) {
    if (!reachable[i]) {
      edits[i].reset();
      changed = true;
    }
  }
  if (changed) {
    compact(function, edits);
  }
  return changed;
}

}  // namespace

BytecodeProgram optimize_bytecode(const BytecodeProgram& program) {
  auto result = program;
  for (auto& function : result.functions) {
    bool changed;
    do {
      changed = local_rewrites(function);
      changed = thread_jumps(function) || changed;
      changed = remove_unreachable(function) || changed;
    } while (changed);
  }
  return result;
}

}  // namespace minic
