#include "bytecode_compiler.hpp"
#include "bytecode_optimizer.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "vm.hpp"

#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

struct Programs {
  minic::Program ast;
  minic::BytecodeProgram plain;
  minic::BytecodeProgram optimized;

  explicit Programs(std::string_view source)
      : ast(minic::parse(minic::lex(source))), plain(minic::compile_bytecode(ast)),
        optimized(minic::optimize_bytecode(plain)) {}
};

struct Outcome {
  std::optional<minic::Value> value;
  minic::SourceLocation location;
  std::string message;
};

template <class Run>
Outcome capture(Run run) {
  try {
    return {run(), {}, {}};
  } catch (const minic::RuntimeError& caught) {
    return {std::nullopt, caught.location, caught.what()};
  }
}

void same(const Outcome& expected, const Outcome& actual) {
  require(expected.value == actual.value && expected.message == actual.message &&
              expected.location.line == actual.location.line && expected.location.column == actual.location.column,
          "Execution engines disagree: " + expected.message + " / " + actual.message);
}

Outcome compare(const Programs& programs, minic::ExecutionLimits limits = {}) {
  const auto expected = capture([&] { return minic::interpret(programs.ast, "main", limits); });
  same(expected, capture([&] { return minic::execute_bytecode(programs.plain, "main", limits); }));
  same(expected, capture([&] { return minic::execute_bytecode(programs.optimized, "main", limits); }));
  return expected;
}

void value(std::string_view source, minic::Value expected, minic::ExecutionLimits limits = {}) {
  const auto actual = compare(Programs(source), limits);
  require(actual.value == expected, "Unexpected array result: " + actual.message);
}

void error(std::string_view source, minic::SourceLocation location, const std::string& message,
           minic::ExecutionLimits limits = {}) {
  const auto actual = compare(Programs(source), limits);
  require(!actual.value && actual.message == message && actual.location.line == location.line &&
              actual.location.column == location.column,
          "Unexpected array error at " + std::to_string(actual.location.line) + ':' +
              std::to_string(actual.location.column) + ": " + actual.message);
}

void integers() {
  value("int main() { int a[4]; int i = 0; int total = 0; while (i < 4) { "
        "a[i] = i + 1; total = total + a[i]; i = i + 1; } return total; }", std::int64_t{10});
  value("int main() { int a[1024]; a[0] = 3; a[1023] = 7; return a[0] + a[1023]; }", std::int64_t{10});
  value("int main() { int a[0001]; a[0] = -9223372036854775808; return a[0]; }",
        std::numeric_limits<std::int64_t>::min());
}

void booleans() {
  value("bool main() { bool a[2]; a[0] = false; a[1] = true; return !a[0] && a[1]; }", true);
  value("int main() { bool flags[1]; flags[0] = false; if (flags[0]) return 1; else return 2; }", std::int64_t{2});
}

void index_order() {
  value("int main() { int a[2]; int i = 0; a[i] = (i = 1); a[1] = 9; return a[0] * 10 + i; }", std::int64_t{11});
  value("int id(int n) { return n; } int main() { int a[2]; int i = 0; "
        "a[id(i = 1)] = id(i = 0); return a[1] + i; }", std::int64_t{0});
  value("int main() { int a[2]; int i = 0; a[i] = (a[i = 1] = 5); return a[0] * 10 + a[1] + i; }",
        std::int64_t{56});
}

void assignment_values() {
  value("int main() { int a[2]; int x = a[0] = a[1] = 7; return x + a[0] + a[1]; }", std::int64_t{21});
  value("bool main() { bool a[1]; return (a[0] = false) == a[0]; }", true);
  value("int main() { int a[2]; a[0] = 1; a[a[0]] = 8; return (a[1]) = a[1] + 1; }", std::int64_t{9});
}

void bounds() {
  for (const std::string index : {"-1", "2", "9223372036854775807", "-9223372036854775808"}) {
    error("int main() {\n  int a[2];\n  return a[" + index + "];\n}", {3, 10},
          "The array index " + index + " is outside \"a\" with length 2.");
  }
  error("int main() {\n  int a[1];\n  a[1] = 1 / 0;\n  return 0;\n}", {3, 3},
        "The array index 1 is outside \"a\" with length 1.");
  error("int f() { return 1; } int main() {\n  int a[1];\n  a[-1] = f();\n  return 0;\n}", {3, 3},
        "The array index -1 is outside \"a\" with length 1.", {100, 1});
  error("int main() {\n  int a[1];\n  return a[9223372036854775808];\n}", {3, 12},
        "The integer literal is outside the signed 64-bit range.");
}

void missing_values() {
  error("int main() {\n  int a[2];\n  return a[0];\n}", {3, 10}, "The array element \"a[0]\" has no value.");
  error("int main() {\n  int a[2]; a[0] = 1;\n  return a[1];\n}", {3, 10}, "The array element \"a[1]\" has no value.");
  value("int f() { return 7; } int main() { int a[1]; a[0] = f(); return a[0]; }", std::int64_t{7});
}

void reset_and_scope() {
  value("int main() { int a[1]; a[0] = 3; { bool a[1]; a[0] = false; } "
        "{ int a = 9; a = a + 1; } return a[0]; }", std::int64_t{3});
  error("int main() { int i = 0; while (i < 2) { int a[2]; if (i == 0) a[1] = 7; i = i + 1;\n"
        "  a[1]; } return 0; }", {2, 3}, "The array element \"a[1]\" has no value.");
  value("int main() { int i = 0; int total = 0; while (i < 3) { int a[1]; "
        "a[0] = i; total = total + a[0]; i = i + 1; } return total; }", std::int64_t{3});
}

void calls() {
  value("int change(int n) { n = 9; return n; } int main() { int a[1]; "
        "a[0] = 3; return change(a[0]) + a[0]; }", std::int64_t{12});
  value("int add(int x, int y) { return x + y; } int main() { int a[2]; "
        "a[0] = 4; a[1] = add(a[0], add(a[0], a[0])); return 1 + add(a[0], a[1]); }", std::int64_t{17});
}

void recursion() {
  value("int sum(int n) { int a[2]; a[0] = n; if (n == 0) return a[0]; "
        "a[1] = sum(n - 1); return a[0] + a[1]; } int main() { return sum(10); }", std::int64_t{55});
  value("int f(int n) { int a[1]; a[0] = n; if (n > 0) a[0] = a[0] + f(n - 1); "
        "else a[0] = 1; return a[0]; } int main() { return f(5); }", std::int64_t{16});
}

void skipped_access() {
  value("bool main() { bool a[1]; false && a[0]; true || a[9]; "
        "if (false) a[-1] = true; while (false) a[2]; return true; }", true);
  value("int main() { int a[1]; if (true) return 7; else return a[3]; }", std::int64_t{7});
}

void step_limits() {
  const std::string source = "int main() {\n  int a[1];\n  a[0] = 7;\n  return a[0];\n}";
  value(source, std::int64_t{7}, {10, 1});
  error(source, {4, 12}, "The execution step limit was reached.", {9, 1});
  value("void main() { int a[1024]; }", std::monostate{}, {2, 1});
}

void optimizer_barriers() {
  error("int main() {\n  int a[1];\n  a[0];\n  return 7;\n}", {3, 3}, "The array element \"a[0]\" has no value.");
  error("int main() {\n  int a[1];\n  a[2] = 1 / 0;\n  return 7;\n}", {3, 3},
        "The array index 2 is outside \"a\" with length 1.");
  value("int main() { int a[1]; a[0] = 1 + 2 * 3; a[0] = a[0] + 1; return a[0]; }", std::int64_t{8});
}

void differential_limits() {
  for (const std::string_view source : {
           "int main() { int a[1]; a[0] = 7; return a[0]; }",
           "int main() { int a[1]; return a[0]; }",
           "int main() { int a[1]; a[-1] = 1 / 0; return 0; }",
           "int main() { int a[2]; int i = 0; a[i] = (i = 1); return a[0]; }",
           "int f(int n) { int a[1]; a[0] = n; if (n == 0) return a[0]; "
           "a[0] = a[0] + f(n - 1); return a[0]; } int main() { return f(3); }",
           "void f() {} int main() { int a[1]; f(); a[0] = 1; return a[0]; }",
           "int main() { int i = 0; while (i < 2) { int a[1]; if (i == 0) a[0] = 2; i = i + 1; a[0]; } return 0; }",
           "bool main() { bool a[1]; false && a[0]; a[0] = true; return a[0] || a[-1]; }"}) {
    const Programs programs(source);
    for (std::size_t steps = 1; steps <= 80; ++steps) {
      for (const std::size_t depth : {1U, 2U, 4U}) {
        compare(programs, {steps, depth});
      }
    }
  }
}

void invalid_bytecode() {
  const Programs programs("int main() { int a[1]; a[0] = 7; return a[0]; }");
  for (int mutation = 0; mutation < 4; ++mutation) {
    auto bad = programs.plain;
    if (mutation == 0) {
      bad.functions[0].locals[0].array_length = 1025;
    } else {
      bool changed = false;
      for (auto& instruction : bad.functions[0].code) {
        if (instruction.opcode == (mutation == 3 ? minic::OpCode::ResetLocal : minic::OpCode::CheckIndex)) {
          if (mutation == 1) {
            instruction.opcode = minic::OpCode::LoadLocal;
          } else {
            instruction.operand = 999;
          }
          changed = true;
          break;
        }
      }
      require(changed, "Expected the mutated instruction.");
    }
    try {
      minic::execute_bytecode(bad);
    } catch (const std::invalid_argument&) {
      continue;
    }
    throw std::runtime_error("Expected an invalid array instruction diagnostic.");
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    require(argc == 2, "Expected one test case.");
    const std::string_view name = argv[1];
    if (name == "integers") integers();
    else if (name == "booleans") booleans();
    else if (name == "index_order") index_order();
    else if (name == "assignment_values") assignment_values();
    else if (name == "bounds") bounds();
    else if (name == "missing_values") missing_values();
    else if (name == "reset_and_scope") reset_and_scope();
    else if (name == "calls") calls();
    else if (name == "recursion") recursion();
    else if (name == "skipped_access") skipped_access();
    else if (name == "step_limits") step_limits();
    else if (name == "optimizer_barriers") optimizer_barriers();
    else if (name == "differential_limits") differential_limits();
    else if (name == "invalid_bytecode") invalid_bytecode();
    else throw std::runtime_error("Unknown test case.");
  } catch (const std::exception& caught) {
    std::cerr << caught.what() << '\n';
    return 1;
  }
}
