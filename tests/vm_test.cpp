#include "bytecode_compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "vm.hpp"

#include <iostream>
#include <limits>
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
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"arithmetic", arithmetic}, {"booleans", booleans}, {"assignments", assignments}, {"scopes", scopes},
      {"initialization", initialization}, {"evaluation_order", evaluation_order}, {"short_circuit", short_circuit},
      {"conditionals", conditionals}, {"loops", loops}, {"returns", returns}, {"entry_selection", entry_selection},
      {"numeric_errors", numeric_errors}, {"step_limits", step_limits}, {"ownership", ownership},
      {"invalid_bytecode", invalid_bytecode},
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
