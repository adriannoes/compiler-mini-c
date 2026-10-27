#include "bytecode_compiler.hpp"
#include "bytecode_optimizer.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "vm.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

using minic::OpCode;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

minic::BytecodeProgram compile(std::string_view source) {
  return minic::compile_bytecode(minic::parse(minic::lex(source)));
}

struct Outcome {
  std::optional<minic::Value> value;
  minic::SourceLocation location;
  std::string message;
};

template <class Run>
Outcome capture(Run run) {
  try {
    return {run(), {}, {}};
  } catch (const minic::RuntimeError& error) {
    return {std::nullopt, error.location, error.what()};
  }
}

void same(const Outcome& actual, const Outcome& expected, minic::ExecutionLimits limits) {
  require(actual.value == expected.value && actual.message == expected.message &&
              actual.location.line == expected.location.line && actual.location.column == expected.location.column,
          "Execution differs at step budget " + std::to_string(limits.max_steps) +
              " and call depth " + std::to_string(limits.max_call_depth) + ": " +
              expected.message + " / " + actual.message);
}

void compare(const minic::Program& source, const minic::BytecodeProgram& original,
             const minic::BytecodeProgram& optimized, minic::ExecutionLimits limits = {}) {
  const auto expected = capture([&] { return minic::interpret(source, "main", limits); });
  same(capture([&] { return minic::execute_bytecode(original, "main", limits); }), expected, limits);
  same(capture([&] { return minic::execute_bytecode(optimized, "main", limits); }), expected, limits);
}

void compare(std::string_view text, std::size_t max_steps = 0) {
  const auto source = minic::parse(minic::lex(text));
  const auto original = minic::compile_bytecode(source);
  const auto optimized = minic::optimize_bytecode(original);
  compare(source, original, optimized);
  for (std::size_t steps = 1; steps <= max_steps; ++steps) {
    compare(source, original, optimized, {steps, 128});
  }
}

std::size_t count(const minic::BytecodeFunction& function, OpCode opcode) {
  return static_cast<std::size_t>(std::count_if(function.code.begin(), function.code.end(), [&](const auto& instruction) {
    return instruction.opcode == opcode;
  }));
}

void empty() {
  require(minic::print_bytecode(minic::optimize_bytecode({})) == "bytecode\n", "Expected empty optimization.");
  const auto program = minic::optimize_bytecode(compile("int helper(int n) { return n + (2 * 3); }"));
  require(program.functions[0].name == "helper" && program.functions[0].parameter_count == 1,
          "Optimization must not require an entry or change a function signature.");
}

void integer_folding() {
  const auto original = compile("int main() { return (2 + 3) * 4; }");
  const auto optimized = minic::optimize_bytecode(original);
  require(optimized.functions[0].code.size() < original.functions[0].code.size(), "Expected fewer instructions.");
  require(count(optimized.functions[0], OpCode::Add) == 0 && count(optimized.functions[0], OpCode::Multiply) == 0,
          "Expected nested arithmetic to fold.");
  require(std::get<std::int64_t>(minic::execute_bytecode(optimized)) == 20, "Expected the folded value 20.");
  compare("int main() { return -7 / 3 + -7 % 3 - 2; }", 40);
  compare("int main() { return -9223372036854775807 - 1; }", 20);
}

void boolean_folding() {
  const auto optimized = minic::optimize_bytecode(compile("bool main() { return !(1 + 2 < 4) == false; }"));
  require(count(optimized.functions[0], OpCode::Less) == 0 && count(optimized.functions[0], OpCode::Not) == 0 &&
              count(optimized.functions[0], OpCode::Equal) == 0, "Expected constant comparisons and logic to fold.");
  require(std::get<bool>(minic::execute_bytecode(optimized)), "Unexpected folded boolean result.");
  compare("bool main() { return (1 <= 2) != (3 > 4) == (4 >= 4); }", 40);
}

void numeric_guards() {
  for (const std::string text : {
           "int main() { return 1 / 0; }", "int main() { return 1 % 0; }",
           "int main() { return 9223372036854775807 + 1; }",
           "int main() { return -9223372036854775808 - 1; }",
           "int main() { return 9223372036854775807 * 2; }",
           "int main() { return -(-9223372036854775808); }",
           "int main() { return -9223372036854775808 / -1; }",
           "int main() { return -9223372036854775808 % -1; }",
           "int main() { return 9223372036854775808; }",
           "int main() { return -9223372036854775809; }"}) {
    compare(text, 20);
  }
}

void step_markers() {
  const auto original = compile("int main() { return (2 + 3) * -4; }");
  const auto optimized = minic::optimize_bytecode(original);
  const auto positions = [](const auto& function) {
    std::vector<std::pair<std::size_t, std::size_t>> result;
    for (const auto& instruction : function.code) {
      if (instruction.opcode == OpCode::Step) {
        result.emplace_back(instruction.location.line, instruction.location.column);
      }
    }
    return result;
  };
  require(positions(original.functions[0]) == positions(optimized.functions[0]), "Constant folding changed source steps.");
  compare("int main() { return (2 + 3) * -4; }", 25);
}

void block_boundaries() {
  const auto original = compile("bool f(bool b) { return (b || false) == false; } "
                                "bool main() { return f(false) && !f(true); }");
  const auto optimized = minic::optimize_bytecode(original);
  require(count(optimized.functions[0], OpCode::Equal) == 1, "A branch merge must end local constant tracking.");
  require(std::get<bool>(minic::execute_bytecode(optimized)), "Unexpected merged branch value.");
  compare("int main() { int x = 0; if (true) x = (1 + 2); else x = (3 + 4); return x; }", 50);
}

void side_effects() {
  compare("int main() { int x = 0; int y = (x = 1) + (x = 2) + (1 + 2); return y + x; }", 80);
  compare("int f(int n) { return n; } int main() { return 1 + f(2 + 3); }", 40);
  compare("int main() { int x; return (1 + 2) + x; }", 30);
  compare("int main() { return (1 / 0) + (2 + 3); }", 30);
}

void branch_targets() {
  const std::string text = "int main() { int x = 0; while (x < 3) { x = x + (1 + 2); } "
                            "if (x == 3) return 4 + 5; else return 6 * 7; }";
  const auto optimized = minic::optimize_bytecode(compile(text));
  for (const auto& function : optimized.functions) {
    for (const auto& instruction : function.code) {
      if (instruction.opcode == OpCode::Jump || instruction.opcode == OpCode::JumpIfFalse ||
          instruction.opcode == OpCode::JumpIfTrue) {
        require(instruction.operand < function.code.size(), "An optimized jump left its function.");
      }
    }
  }
  compare(text, 120);
  compare("bool main() { return (true && (1 + 2 == 3)) || false; }", 50);
}

void ownership() {
  const auto original = compile("int main() { return (2 + 3) * 4; }");
  const auto before = minic::print_bytecode(original);
  const auto first = minic::optimize_bytecode(original);
  require(minic::print_bytecode(original) == before, "Optimization changed the input bytecode.");
  require(minic::print_bytecode(first) == minic::print_bytecode(minic::optimize_bytecode(original)),
          "Expected deterministic optimization.");
  require(minic::print_bytecode(first) == minic::print_bytecode(minic::optimize_bytecode(first)),
          "Expected stable repeated optimization.");
  const auto owned = minic::optimize_bytecode(compile("int main() { return 2 + 3; }"));
  require(std::get<std::int64_t>(minic::execute_bytecode(owned)) == 5, "Expected the result to own its bytecode data.");
}

void differential_limits() {
  for (const std::string text : {
           "int main() { return (2 + 3) * 4; }",
           "bool main() { return true || (1 / 0 == 0); }",
           "bool main() { return false && (1 / 0 == 0); }",
           "int main() { if (1 + 2 == 3) return 5 * 6; else return 1 / 0; }",
           "int main() { int i = 0; while (i < 3) { i = i + (2 - 1); } return i; }",
           "void main() { while (true) ; }",
           "int f(int n) { if (n == 0) return 2 + 3; return 1 + f(n - 1); } int main() { return f(3); }",
           "int id(int n) { return n; } int main() { return id(id(2 + 3)); }"}) {
    const auto source = minic::parse(minic::lex(text));
    const auto original = minic::compile_bytecode(source);
    const auto optimized = minic::optimize_bytecode(original);
    for (std::size_t steps = 1; steps <= 80; ++steps) {
      for (const std::size_t depth : {1, 2, 4}) {
        compare(source, original, optimized, {steps, depth});
      }
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"integer_folding", integer_folding}, {"boolean_folding", boolean_folding},
      {"numeric_guards", numeric_guards}, {"step_markers", step_markers}, {"block_boundaries", block_boundaries},
      {"side_effects", side_effects}, {"branch_targets", branch_targets}, {"ownership", ownership},
      {"differential_limits", differential_limits},
  };
  if (argc == 2) {
    for (const auto& test : cases) {
      if (std::string_view(argv[1]) == test.name) {
        try {
          test.run();
          std::cout << "PASS " << test.name << '\n';
          return 0;
        } catch (const std::exception& error) {
          std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
          return 1;
        }
      }
    }
  }
  std::cerr << "Expected one known test name.\n";
  return 1;
}
