#include "bytecode_compiler.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "vm.hpp"

#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <string_view>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

minic::BytecodeProgram compile(std::string_view source) {
  return minic::compile_bytecode(minic::parse(minic::lex(source)));
}

minic::Value evaluate(std::string_view source, const std::string& entry = "main",
                      minic::ExecutionLimits limits = {}) {
  return minic::execute_bytecode(compile(source), entry, limits);
}

void integer(std::string_view source, std::int64_t expected) {
  require(std::get<std::int64_t>(evaluate(source)) == expected, "Unexpected integer result.");
}

void boolean(std::string_view source, bool expected) {
  require(std::get<bool>(evaluate(source)) == expected, "Unexpected boolean result.");
}

void error(std::string_view source, minic::SourceLocation location, const std::string& message,
           minic::ExecutionLimits limits = {}) {
  try {
    evaluate(source, "main", limits);
  } catch (const minic::RuntimeError& caught) {
    require(caught.location.line == location.line && caught.location.column == location.column,
            "Unexpected runtime error position.");
    require(std::string(caught.what()) == message, "Unexpected runtime diagnostic: " + std::string(caught.what()));
    return;
  }
  throw std::runtime_error("Expected a runtime error.");
}

void arithmetic() {
  integer("int main() { return (007 + 2) * -3 - 4 / 2 % 2; }", -27);
  integer("int main() { return -7 / 3; }", -2);
  integer("int main() { return -7 % 3; }", -1);
  integer("int main() { return 9223372036854775807; }", std::numeric_limits<std::int64_t>::max());
  integer("int main() { return -(0009223372036854775808); }", std::numeric_limits<std::int64_t>::min());
}

void booleans() {
  boolean("bool main() { return !(3 < 2) && 3 >= 3 && 1 <= 2 && 4 > 2 && 1 != 2 && true == !false; }", true);
  boolean("bool main() { return false == true; }", false);
}

void assignments() {
  integer("int main() { int a; int b; int c = a = b = 3; return a + b + c; }", 9);
  boolean("bool main() { bool a; bool b; return a = b = true; }", true);
}

void scopes() {
  integer("int main() { int x = 1; { bool x = true; x = false; } { int y = x; x = y + 3; } return x; }", 4);
  integer("int main() { int x = 1; { x = x + 1; int x = 9; x = x + 1; } return x; }", 2);
}

void initialization() {
  error("int main() {\n  int x;\n  return x;\n}", {3, 10}, "The variable \"x\" has no value.");
  error("int main() {\n  int x = x;\n  return x;\n}", {2, 11}, "The variable \"x\" has no value.");
  integer("int main() { int x; x = 7; return x; }", 7);
}

void evaluation_order() {
  integer("int main() { int x = 0; int y = (x = 1) + (x = 2); return x * 10 + y; }", 23);
  error("int main() {\n  return (1 / 0) + 9223372036854775808; }", {2, 13}, "The divisor is zero.");
}

void short_circuit() {
  boolean("bool main() { return false && (1 / 0 == 0); }", false);
  boolean("bool main() { return true || (1 / 0 == 0); }", true);
  boolean("bool main() { bool x; return false && x; }", false);
  boolean("bool main() { bool x = false; true || (x = true); false && (x = true); return x; }", false);
  boolean("bool main() { bool x = false; false || (x = true); return x; }", true);
}

void conditionals() {
  integer("int main() { if (true) return 1; else return 1 / 0; }", 1);
  integer("int main() { if (false) return 1 / 0; else return 2; }", 2);
  integer("int main() { int x = 0; if (true) if (false) x = 1; else x = 2; return x; }", 2);
}

void loops() {
  integer("int main() { int x = 0; while (x < 4) x = x + 1; return x; }", 4);
  integer("int main() { while (false) 1 / 0; return 3; }", 3);
  integer("int main() { int i = 0; int total = 0; while (i < 3) { int x; x = i; "
          "{ int i = 10; total = total + x + i; } i = i + 1; } return total; }", 33);
  error("int main() { int i = 0; while (i < 2) { int x; if (i == 0) x = 1; i = i + 1;\n  x; } return 0; }",
        {2, 3}, "The variable \"x\" has no value.");
}

void returns() {
  integer("int main() { { return 7; } int x = 1 / 0; return x; }", 7);
  integer("int main() { while (true) { while (true) { if (true) return 7; } } return 0; }", 7);
  integer("int main() { return 1; return 9223372036854775808; }", 1);
  require(std::holds_alternative<std::monostate>(evaluate("void main() { { return; } 1 / 0; }")),
          "Expected an explicit void return.");
  require(std::holds_alternative<std::monostate>(evaluate("void main() { ; { int x = 1; x = 2; } }")),
          "Expected void fallthrough.");
}

void entry_selection() {
  require(std::get<bool>(evaluate("int main() { return 3; } bool answer() { return true; }", "answer")),
          "Expected the selected entry.");
  error("void utility() {}", {1, 1}, "The entry function \"main\" does not exist.");
  error("", {1, 1}, "The entry function \"main\" does not exist.");
  error("int main(int n) { return n; }", {1, 5}, "The entry function \"main\" must not have parameters.");
}

void numeric_errors() {
  error("int main() {\n  return 9223372036854775808; }", {2, 10},
        "The integer literal is outside the signed 64-bit range.");
  error("int main() {\n  return -9223372036854775809; }", {2, 10},
        "The integer literal is outside the signed 64-bit range.");
  error("int main() { int x = 9223372036854775807;\n  return x + 1; }", {2, 12},
        "Integer arithmetic exceeds the signed 64-bit range.");
  error("int main() { int x = -9223372036854775808;\n  return -x; }", {2, 10},
        "Integer arithmetic exceeds the signed 64-bit range.");
  for (const std::string op : {"/", "%"}) {
    error("int main() {\n  return 1 " + op + " 0; }", {2, 12}, "The divisor is zero.");
    error("int main() { int x = -9223372036854775808;\n  return x " + op + " -1; }", {2, 12},
          "Integer arithmetic exceeds the signed 64-bit range.");
  }
}

void step_limits() {
  require(std::get<std::int64_t>(evaluate("int main() { return 42; }", "main", {3, 1})) == 42,
          "Expected three source steps, independently of the instruction count.");
  error("int main() { return 42; }", {1, 21}, "The execution step limit was reached.", {2, 1});
  error("void main() {\n  while (true) ;\n}", {2, 10}, "The execution step limit was reached.", {6, 128});
  require(std::get<std::int64_t>(evaluate("int main() { return -9223372036854775808; }", "main", {4, 128})) ==
              std::numeric_limits<std::int64_t>::min(), "Expected two steps for a negated literal.");
  for (const minic::ExecutionLimits limits : {minic::ExecutionLimits{0, 128}, minic::ExecutionLimits{100, 0}}) {
    try {
      minic::execute_bytecode({}, "main", limits);
      throw std::runtime_error("Expected positive execution limits.");
    } catch (const std::invalid_argument& caught) {
      require(std::string(caught.what()) == "Execution limits must be greater than zero.", "Unexpected limit diagnostic.");
    }
  }
}

void ownership() {
  const auto program = compile("int main() { int x = 0; return x = x + 1; }");
  const auto before = minic::print_bytecode(program);
  require(std::get<std::int64_t>(minic::execute_bytecode(program)) == 1 &&
              std::get<std::int64_t>(minic::execute_bytecode(program)) == 1,
          "Expected independent state on repeated execution.");
  require(minic::print_bytecode(program) == before, "Execution changed the bytecode.");
  const auto result = evaluate("int main() { return 42; }");
  require(std::get<std::int64_t>(result) == 42, "Expected the result to outlive temporary bytecode.");
}

void calls() {
  integer("int add(int a, int b) { return a + b; } int main() { return 1 + add(2, add(3, 4)); }", 10);
  integer("int main() { int x = f(); int f = 7; return x + f; } int f() { return 1; }", 8);
  integer("void noop() {} int main() { noop(); return 2; }", 2);
}

void parameters() {
  integer("int change(int a, int b) { a = 9; return b; } int main() { int x = 3; return change(x, x) + x; }", 6);
  boolean("bool flip(bool b) { b = !b; return b; } bool main() { bool b = false; return flip(b) && !b; }", true);
  integer("int f(int n) { { int n = 10; n = 20; } return n; } int main() { return f(3); }", 3);
  integer("int pair(int a, int b) { return a * 10 + b; } "
          "int main() { int x = 0; int r = pair(x = x + 1, x = x + 1); return r * 10 + x; }", 122);
}

void recursion() {
  integer("int sum(int n) { int local = n * 10; if (n == 0) return local; "
          "int child = sum(n - 1); return local + n + child; } int main() { return sum(3); }", 66);
  integer("int factorial(int n) { if (n < 2) return 1; return n * factorial(n - 1); } "
          "int main() { return factorial(6); }", 720);
  boolean("bool even(int n) { if (n == 0) return true; return odd(n - 1); } "
          "bool odd(int n) { if (n == 0) return false; return even(n - 1); } "
          "bool main() { return even(10) && odd(9); }", true);
}

void call_returns() {
  integer("int f() { while (true) { if (true) return 4; } return 0; } "
          "int main() { int x = f(); x = x + 1; return x; }", 5);
  integer("void f(int x) { while (true) { x = 99; return; } } int main() { int x = 1; f(x); return x; }", 1);
  require(std::holds_alternative<std::monostate>(evaluate("void f() { return; } void main() { f(); }")),
          "Expected an explicit void call result.");
  integer("void f() { int x = 2; } int main() { int x = 1; f(); return x; }", 1);
}

void call_errors() {
  error("int divide(int n) {\n  return 1 / n;\n} int main() { return divide(0); }", {2, 12}, "The divisor is zero.");
  error("int f(int n) { int x; if (n == 0) {\n  return x; } x = n; return f(n - 1); } "
        "int main() { return f(1); }", {2, 10}, "The variable \"x\" has no value.");
  const auto program = compile("int bad() { return 1 / 0; } int good() { return 42; }");
  try {
    minic::execute_bytecode(program, "bad");
    throw std::runtime_error("Expected a callee runtime error.");
  } catch (const minic::RuntimeError&) {}
  require(std::get<std::int64_t>(minic::execute_bytecode(program, "good")) == 42,
          "Expected independent state after a failed execution.");
}

void call_depth() {
  const std::string source = "int f() {\n  return f();\n} int main() { return f(); }";
  error(source, {2, 10}, "The call depth limit was reached.", {1000, 3});
  error(source, {2, 10}, "The call depth limit was reached.");
  require(std::get<std::int64_t>(evaluate("int id(int n) { return n; } int main() { return id(id(id(3))); }",
                                         "main", {100, 2})) == 3,
          "Expected argument calls to return before the enclosing call starts.");
  error("int take(int x) { return x; } int main() { int x;\n  return take(x); }", {2, 15},
        "The variable \"x\" has no value.", {100, 1});
}

void shared_budget() {
  error("void f() {} void main() { f(); f(); }", {1, 10}, "The execution step limit was reached.", {6, 2});
  require(std::get<std::int64_t>(evaluate("void f() {} int main() { f(); f(); f(); return 42; }", "main", {100, 2})) == 42,
          "Expected returns to free call-depth capacity.");
}

void deep_calls() {
  const auto value = evaluate("int down(int n) { if (n == 0) return 0; return 1 + down(n - 1); } "
                              "int main() { return down(5000); }", "main", {200000, 6000});
  require(std::get<std::int64_t>(value) == 5000, "Expected deep VM calls without native recursion.");
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
  } catch (const minic::RuntimeError& caught) {
    return {std::nullopt, caught.location, caught.what()};
  }
}

void compare(const minic::Program& program, const minic::BytecodeProgram& bytecode,
             minic::ExecutionLimits limits = {}, const std::string& entry = "main") {
  const auto expected = capture([&] { return minic::interpret(program, entry, limits); });
  const auto actual = capture([&] { return minic::execute_bytecode(bytecode, entry, limits); });
  require(actual.value == expected.value && actual.message == expected.message &&
              actual.location.line == expected.location.line && actual.location.column == expected.location.column,
          "VM and interpreter disagree at step budget " + std::to_string(limits.max_steps) +
              " and call depth " + std::to_string(limits.max_call_depth) + ": " +
              expected.message + " / " + actual.message);
}

void compare(std::string_view source) {
  const auto program = minic::parse(minic::lex(source));
  compare(program, minic::compile_bytecode(program));
}

void differential_values() {
  for (const std::string source : {
           "int main() { int x; int y = x = 7; return x * 10 + y; }",
           "int main() { int x = 1; { x = x + 1; int x = 9; } return x; }",
           "bool main() { bool x = false; true || (x = true); return !x; }",
           "int main() { int i = 0; while ((i = i + 1) < 4) ; return i; }",
           "void main() { while (true) { return; } 1 / 0; }",
           "int main() { return -(0009223372036854775808); }",
           "int id(int n) { return n; } int main() { return 100 + id(id(7)) * id(3); }",
           "int pair(int a, int b) { return a * 10 + b; } int main() { int x = 0; return pair(x = 1, x = x + 1); }",
           "void f() {} void main() { f(); f(); }",
           "int fib(int n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); } int main() { return fib(10); }"}) {
    compare(source);
  }
}

void differential_errors() {
  for (const std::string source : {
           "", "void helper() {}", "int main(int n) { return n; }",
           "int main() { int x = x; return x; }",
           "int main() { return 9223372036854775808; }",
           "int main() { return -9223372036854775809; }",
           "int main() { return 9223372036854775807 + 1; }",
           "int main() { return -9223372036854775808 - 1; }",
           "int main() { return 9223372036854775807 * 2; }",
           "int main() { return -(-9223372036854775808); }",
           "int main() { return -9223372036854775808 / -1; }",
           "int main() { return -9223372036854775808 % -1; }",
           "int main() { return 1 % 0; }",
           "bool f() { bool x; return x; } bool main() { return f(); }",
           "int divide(int n) { return 1 / n; } int main() { return 7 + divide(0); }",
           "int main() { int i = 0; while (i < 2) { int x; if (i == 0) x = 1; i = i + 1; x; } return 0; }"}) {
    compare(source);
  }
}

void differential_limits() {
  for (const std::string source : {
           "int main() { return 42; }",
           "int main() { int x; return x = -1; }",
           "bool main() { return true || (1 / 0 == 0); }",
           "bool main() { return false && (1 / 0 == 0); }",
           "int main() { if (true) { return 2; } else return 3; }",
           "int main() { int i = 0; while (i < 3) { i = i + 1; } return i; }",
           "void main() { while (true) ; }",
           "void f() {} void main() { f(); f(); }",
           "int f(int n) { if (n == 0) return 0; return 1 + f(n - 1); } int main() { return f(3); }",
           "int id(int n) { return n; } int main() { return id(id(id(3))); }"}) {
    const auto program = minic::parse(minic::lex(source));
    const auto bytecode = minic::compile_bytecode(program);
    for (std::size_t steps = 1; steps <= 80; ++steps) {
      for (const std::size_t depth : {1, 2, 4}) {
        compare(program, bytecode, {steps, depth});
      }
    }
  }
}

void fixtures(const std::string& directory) {
  for (const std::string name : {"hello", "lexer", "calls", "control_flow", "scopes", "recursion", "types",
                                 "interpreter", "short_circuit", "runtime_error", "factorial", "fibonacci"}) {
    std::ifstream input(directory + "/" + name + ".mc", std::ios::binary);
    require(input.is_open(), "Cannot open the example " + name + ".");
    std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (name == "control_flow") {
      source += "\nint main() { return choose(true, false); }\n";
    } else if (name == "recursion") {
      source += "\nbool main() { return even(10) && odd(9); }\n";
    }
    compare(source);
  }
}

void invalid_bytecode() {
  for (const auto opcode : {minic::OpCode::Pop, minic::OpCode::LoadConstant, minic::OpCode::LoadLocal,
                            minic::OpCode::Jump, static_cast<minic::OpCode>(999)}) {
    auto program = compile("int main() { return 42; }");
    program.functions[0].code[0] = {opcode, 999, {1, 1}};
    try {
      minic::execute_bytecode(program);
      throw std::runtime_error("Expected an invalid instruction error.");
    } catch (const std::invalid_argument& caught) {
      require(std::string(caught.what()).find("Invalid bytecode: ") == 0, "Unexpected invalid bytecode diagnostic.");
    }
  }
  auto program = compile("int main() { return 42; }");
  program.functions[0].constants[0] = true;
  try {
    minic::execute_bytecode(program);
    throw std::runtime_error("Expected an invalid return type error.");
  } catch (const std::invalid_argument&) {}
  for (const auto corrupt : {0, 1, 2}) {
    auto called = compile("int f(int n) { return n; } int main() { return 9 + f(1); }");
    if (corrupt == 0) {
      for (auto& instruction : called.functions[1].code) {
        if (instruction.opcode == minic::OpCode::Call) {
          instruction.operand = 999;
        }
      }
    } else if (corrupt == 1) {
      called.functions[0].parameter_count = 2;
    } else {
      called.functions[0].code[0] = {minic::OpCode::Pop, 0, {1, 1}};
    }
    try {
      minic::execute_bytecode(called);
      throw std::runtime_error("Expected a guarded invalid call.");
    } catch (const std::invalid_argument&) {}
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3 && std::string_view(argv[1]) == "fixtures") {
    try {
      fixtures(argv[2]);
      std::cout << "PASS fixtures\n";
      return 0;
    } catch (const std::exception& caught) {
      std::cerr << "FAIL fixtures: " << caught.what() << '\n';
      return 1;
    }
  }
  const struct { const char* name; void (*run)(); } cases[] = {
      {"arithmetic", arithmetic}, {"booleans", booleans}, {"assignments", assignments}, {"scopes", scopes},
      {"initialization", initialization}, {"evaluation_order", evaluation_order}, {"short_circuit", short_circuit},
      {"conditionals", conditionals}, {"loops", loops}, {"returns", returns}, {"entry_selection", entry_selection},
      {"numeric_errors", numeric_errors}, {"step_limits", step_limits}, {"ownership", ownership},
      {"invalid_bytecode", invalid_bytecode},
      {"calls", calls}, {"parameters", parameters}, {"recursion", recursion}, {"call_returns", call_returns},
      {"call_errors", call_errors}, {"call_depth", call_depth}, {"shared_budget", shared_budget},
      {"deep_calls", deep_calls}, {"differential_values", differential_values},
      {"differential_errors", differential_errors}, {"differential_limits", differential_limits},
  };
  if (argc == 2) {
    for (const auto& test : cases) {
      if (std::string_view(argv[1]) == test.name) {
        try {
          test.run();
          std::cout << "PASS " << test.name << '\n';
          return 0;
        } catch (const std::exception& caught) {
          std::cerr << "FAIL " << test.name << ": " << caught.what() << '\n';
          return 1;
        }
      }
    }
  }
  std::cerr << "Expected one known test name.\n";
  return 1;
}
