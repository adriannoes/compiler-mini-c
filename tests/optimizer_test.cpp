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

void constant_branches() {
  const std::string text = "int main() { if (1 + 2 == 3) return 42; else return 1 / 0; }";
  const auto optimized = minic::optimize_bytecode(compile(text));
  require(count(optimized.functions[0], OpCode::Divide) == 0, "Expected the unreachable arithmetic to disappear.");
  require(count(optimized.functions[0], OpCode::JumpIfFalse) == 0, "Expected a known condition to simplify.");
  compare(text, 60);
  compare("int main() { if (3 < 2) return 1 / 0; return 7; }", 40);
  compare("bool loop() { return loop(); } bool main() { return true || loop(); }", 40);
  compare("bool main() { return false && (1 / 0 == 0) || true; }", 40);
}

void unreachable() {
  const auto optimized = minic::optimize_bytecode(compile("int main() { return 1; 1 / 0; return 2; }"));
  require(count(optimized.functions[0], OpCode::Divide) == 0, "Expected code after a return to disappear.");
  require(count(optimized.functions[0], OpCode::Return) == 1, "Expected one reachable return.");
  compare("int main() { return 1; 1 / 0; return 2; }", 30);
  compare("void main() { return; 9223372036854775808; }", 20);
}

void discarded_values() {
  const std::string text = "int main() { (1 + 2); true; -7; return 42; }";
  const auto optimized = minic::optimize_bytecode(compile(text));
  require(count(optimized.functions[0], OpCode::LoadConstant) == 1 && count(optimized.functions[0], OpCode::Pop) == 0,
          "Expected unused successful constants to disappear.");
  compare(text, 60);
  compare("int main() { 9223372036854775808; return 42; }", 30);
  compare("int main() { 1 / 0; return 42; }", 30);
}

void jump_cleanup() {
  const std::string text = "int main() { if (true) { if (true) return 1; else return 2; } else return 3; }";
  const auto optimized = minic::optimize_bytecode(compile(text));
  require(count(optimized.functions[0], OpCode::Jump) == 0 && count(optimized.functions[0], OpCode::JumpIfFalse) == 0,
          "Expected redundant jumps to disappear.");
  compare(text, 60);
  compare("int main() { int x = 0; if (false) x = 1; else if (true) x = 2; return x; }", 60);
  const std::string logic = "bool main() { return (false && true) || (true && !false); }";
  const auto simplified = minic::optimize_bytecode(compile(logic));
  require(count(simplified.functions[0], OpCode::Jump) == 0 && count(simplified.functions[0], OpCode::JumpIfFalse) == 0 &&
              count(simplified.functions[0], OpCode::JumpIfTrue) == 0, "Expected constant logical paths to become linear.");
  compare(logic, 60);
}

void loop_limits() {
  compare("void main() { while (1 + 2 == 3) { 2 + 3; } }", 100);
  compare("int main() { while (false) { 1 / 0; } return 2 + 3; }", 50);
  compare("int main() { int i = 0; while (i < 2) { if (true) { i = i + 1; } else return 1 / 0; } return i; }", 100);
}

void preserved_reads() {
  const auto optimized = minic::optimize_bytecode(compile("int main() { int x; x; return 42; }"));
  require(count(optimized.functions[0], OpCode::LoadLocal) == 1, "An unused local read can still fail.");
  compare("int main() { int x; x; return 42; }", 30);
  compare("int main() { int x; return x * 0; }", 30);
  compare("int f() { return 1 / 0; } int main() { return 0 * f(); }", 30);
  compare("int main() { return 0 * (1 / 0); }", 30);
}

void call_arguments() {
  const auto optimized = minic::optimize_bytecode(compile("int f(int n) { return n; } int main() { f(2 + 3); return 1; }"));
  require(count(optimized.functions[1], OpCode::Call) == 1, "A discarded call result must retain the call.");
  compare("void f() { int x = 1 / 0; } int main() { f(); return 42; }", 40);
  compare("int pair(int a, int b) { return a * 10 + b; } "
          "int main() { int x = 0; return pair(x = x + 1, x = x + 1); }", 60);
}

void all_entries() {
  const auto optimized = minic::optimize_bytecode(compile("int first() { return 2 + 3; } int second() { return 6 * 7; }"));
  require(optimized.functions.size() == 2 && optimized.functions[0].name == "first" && optimized.functions[1].name == "second",
          "Expected all named entries to remain available.");
  require(std::get<std::int64_t>(minic::execute_bytecode(optimized, "first")) == 5 &&
              std::get<std::int64_t>(minic::execute_bytecode(optimized, "second")) == 42,
          "Expected independent optimization of every function.");
}

void idempotence() {
  for (const std::string text : {
           "int main() { if (true) return 2 + 3; else return 1 / 0; }",
           "bool main() { return false && (1 / 0 == 0) || true; }",
           "void main() { while (1 + 2 == 3) { 4 * 5; } }",
           "int main() { return 1; 9223372036854775808; }"}) {
    const auto original = compile(text);
    const auto first = minic::optimize_bytecode(original);
    require(minic::print_bytecode(first) == minic::print_bytecode(minic::optimize_bytecode(first)),
            "Expected optimization to reach a stable result.");
    require(first.functions[0].code.size() <= original.functions[0].code.size(), "Optimization increased the instruction count.");
  }
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"integer_folding", integer_folding}, {"boolean_folding", boolean_folding},
      {"numeric_guards", numeric_guards}, {"step_markers", step_markers}, {"block_boundaries", block_boundaries},
      {"side_effects", side_effects}, {"branch_targets", branch_targets}, {"ownership", ownership},
      {"differential_limits", differential_limits},
      {"constant_branches", constant_branches}, {"unreachable", unreachable}, {"discarded_values", discarded_values},
      {"jump_cleanup", jump_cleanup}, {"loop_limits", loop_limits}, {"preserved_reads", preserved_reads},
      {"call_arguments", call_arguments}, {"all_entries", all_entries}, {"idempotence", idempotence},
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
