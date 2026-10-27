#include "ast_printer.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "type_checker.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

minic::Value evaluate(std::string_view source, const std::string& entry = "main") {
  return minic::interpret(minic::parse(minic::lex(source)), entry);
}

void expect_integer(std::string_view source, std::int64_t expected) {
  require(std::get<std::int64_t>(evaluate(source)) == expected, "Unexpected integer result.");
}

void expect_boolean(std::string_view source, bool expected) {
  require(std::get<bool>(evaluate(source)) == expected, "Unexpected boolean result.");
}

void expect_error(std::string_view source, minic::SourceLocation location, const std::string& message,
                  const std::string& entry = "main") {
  try {
    evaluate(source, entry);
  } catch (const minic::RuntimeError& error) {
    require(error.location.line == location.line && error.location.column == location.column,
            "Unexpected error position: " + std::to_string(error.location.line) + ":" +
                std::to_string(error.location.column));
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a runtime error.");
}

void arithmetic() {
  expect_integer("int main() { return (007 + 2) * -3 - 4 / 2 % 2; }", -27);
  expect_integer("int main() { return -7 / 3; }", -2);
  expect_integer("int main() { return -7 % 3; }", -1);
}

void booleans() {
  expect_boolean("bool main() { return !(3 < 2) && 3 >= 3 && 1 <= 2 && 4 > 2 && 1 != 2 && true == !false; }", true);
  expect_boolean("bool main() { return false == true; }", false);
}

void assignments() {
  expect_integer("int main() { int a; int b; int c = a = b = 3; return a + b + c; }", 9);
  expect_boolean("bool main() { bool a; bool b; return a = b = true; }", true);
}

void scopes() {
  expect_integer("int main() { int x = 1; { bool x = true; x = false; } "
                 "{ int y = x; x = y + 3; } return x; }", 4);
  expect_integer("int main() { int x = 1; { x = x + 1; int x = 9; x = x + 1; } return x; }", 2);
}

void initialization() {
  expect_error("int main() {\n  int x;\n  return x;\n}", {3, 10}, "The variable \"x\" has no value.");
  expect_error("int main() {\n  int x = x;\n  return x;\n}", {2, 11}, "The variable \"x\" has no value.");
  expect_error("int main() { int x = 1; {\n  int x = x; } return x; }", {2, 11},
               "The variable \"x\" has no value.");
  expect_integer("int main() { int x; x = 7; return x; }", 7);
}

void evaluation_order() {
  expect_integer("int main() { int x = 0; int y = (x = 1) + (x = 2); return x * 10 + y; }", 23);
  expect_error("int main() {\n  return (1 / 0) + 9223372036854775808; }", {2, 13}, "The divisor is zero.");
}

void short_circuit() {
  expect_boolean("bool main() { return false && (1 / 0 == 0); }", false);
  expect_boolean("bool main() { return true || (1 / 0 == 0); }", true);
  expect_boolean("bool main() { bool x; return false && x; }", false);
  expect_boolean("bool main() { bool x = false; true || (x = true); false && (x = true); return x; }", false);
  expect_boolean("bool main() { bool x = false; false || (x = true); return x; }", true);
  expect_boolean("bool main() { bool x = false; true && (x = true); return x; }", true);
  expect_error("bool main() { return true &&\n  (1 / 0 == 0); }", {2, 6}, "The divisor is zero.");
  expect_error("bool main() { return false ||\n  (1 / 0 == 0); }", {2, 6}, "The divisor is zero.");
}

void returns() {
  expect_integer("int main() { { return 7; } int x = 1 / 0; return x; }", 7);
  expect_integer("int main() { return 1; return 9223372036854775808; }", 1);
  expect_boolean("bool main() { { return false; } return true; }", false);
}

void void_returns() {
  require(std::holds_alternative<std::monostate>(evaluate("void main() { { return; } 1 / 0; }")),
          "Expected an explicit void return to propagate through blocks.");
  require(std::holds_alternative<std::monostate>(evaluate("void main() { ; { int x = 1; x = 2; } }")),
          "Expected void fallthrough.");
}

void entry_selection() {
  require(std::get<bool>(evaluate("int main() { return 3; } bool answer() { return true; }", "answer")),
          "Expected a selected entry without parameters.");
  expect_error("void utility() {}", {1, 1}, "The entry function \"main\" does not exist.");
  expect_error("", {1, 1}, "The entry function \"main\" does not exist.");
  expect_error("int main(int n) { return n; }", {1, 5}, "The entry function \"main\" must not have parameters.");
  expect_error("void utility() {}", {1, 1}, "The entry function \"missing\" does not exist.", "missing");
}

void numeric_limits() {
  expect_integer("int main() { return 9223372036854775807; }", std::numeric_limits<std::int64_t>::max());
  expect_integer("int main() { return -9223372036854775808; }", std::numeric_limits<std::int64_t>::min());
  expect_integer("int main() { return -(0009223372036854775808); }", std::numeric_limits<std::int64_t>::min());
}

void numeric_errors() {
  expect_error("int main() {\n  return 9223372036854775808; }", {2, 10},
               "The integer literal is outside the signed 64-bit range.");
  expect_error("int main() {\n  return -9223372036854775809; }", {2, 10},
               "The integer literal is outside the signed 64-bit range.");
  expect_error("int main() { int x = 9223372036854775807;\n  return x + 1; }", {2, 12},
               "Integer arithmetic exceeds the signed 64-bit range.");
  expect_error("int main() { int x = -9223372036854775808;\n  return -x; }", {2, 10},
               "Integer arithmetic exceeds the signed 64-bit range.");
  for (const std::string op : {"/", "%"}) {
    expect_error("int main() {\n  return 1 " + op + " 0; }", {2, 12}, "The divisor is zero.");
    expect_error("int main() { int x = -9223372036854775808;\n  return x " + op + " -1; }", {2, 12},
                 "Integer arithmetic exceeds the signed 64-bit range.");
  }
}

void unsupported() {
  expect_error("int f() { return 1; } int main() {\n  return f(); }", {2, 10},
               "The interpreter does not support function calls.");
}

void conditionals() {
  expect_integer("int main() { if (true) return 1; else return 1 / 0; }", 1);
  expect_integer("int main() { if (false) return 1 / 0; else return 2; }", 2);
  expect_integer("int main() { int x = 0; if (false) x = 1; return x; }", 0);
  expect_integer("int main() { int x = 0; if (true) if (false) x = 1; else x = 2; return x; }", 2);
  expect_integer("int main() { bool b = false; if (b = !b) { int x = 4; return x; } return 0; }", 4);
}

void loops() {
  expect_integer("int main() { int x = 0; while (x < 4) x = x + 1; return x; }", 4);
  expect_integer("int main() { while (false) 1 / 0; return 3; }", 3);
  expect_integer("int main() { int x = 0; while ((x = x + 1) < 4) ; return x; }", 4);
  expect_integer("int main() { int total = 0; int i = 0; while (i < 2) { int j = 0; "
                 "while (j < 3) { total = total + 1; j = j + 1; } i = i + 1; } return total; }", 6);
}

void loop_environments() {
  expect_integer("int main() { int i = 0; int total = 0; while (i < 3) { int x; x = i; "
                 "{ int i = 10; total = total + x + i; } i = i + 1; } return total; }", 33);
  expect_error("int main() { int i = 0; while (i < 2) { int x; if (i == 0) x = 1; "
               "i = i + 1;\n  x; } return 0; }", {2, 3}, "The variable \"x\" has no value.");
}

void control_returns() {
  expect_integer("int main() { while (true) { while (true) { if (true) return 7; } } return 0; }", 7);
  require(std::holds_alternative<std::monostate>(evaluate("void main() { while (true) { if (true) return; } 1 / 0; }")),
          "Expected a void return to leave conditions and loops.");
}

void step_limits() {
  const auto simple = minic::parse(minic::lex("int main() { return 42; }"));
  require(std::get<std::int64_t>(minic::interpret(simple, "main", {3, 128})) == 42,
          "Expected one step for the body, return, and integer expression.");
  try {
    minic::interpret(simple, "main", {2, 128});
    throw std::runtime_error("Expected a step limit error.");
  } catch (const minic::RuntimeError& error) {
    require(error.location.line == 1 && error.location.column == 21 &&
                std::string(error.what()) == "The execution step limit was reached.",
            "Expected the step error at the next expression.");
  }
  const auto loop = minic::parse(minic::lex("void main() {\n  while (true) ;\n}"));
  try {
    minic::interpret(loop, "main", {6, 128});
    throw std::runtime_error("Expected the loop to consume its step budget.");
  } catch (const minic::RuntimeError& error) {
    require(error.location.line == 2 && error.location.column == 10 &&
                std::string(error.what()) == "The execution step limit was reached.",
            "Expected the loop condition to consume steps on each iteration.");
  }
  const auto minimum = minic::parse(minic::lex("int main() { return -9223372036854775808; }"));
  require(std::get<std::int64_t>(minic::interpret(minimum, "main", {4, 128})) == std::numeric_limits<std::int64_t>::min(),
          "Expected a step for both the unary operator and its literal.");
  for (const minic::ExecutionLimits limits : {minic::ExecutionLimits{0, 128}, minic::ExecutionLimits{100, 0}}) {
    try {
      minic::interpret(simple, "main", limits);
      throw std::runtime_error("Expected positive execution limits.");
    } catch (const std::invalid_argument& error) {
      require(std::string(error.what()) == "Execution limits must be greater than zero.", "Unexpected limit diagnostic.");
    }
  }
}

void skipped_nodes() {
  expect_integer("void helper() { while (true) ; } int main() { return 3; helper(); }", 3);
  expect_boolean("bool helper() { return helper(); } bool main() { return false && helper(); }", false);
  expect_boolean("bool helper() { return helper(); } bool main() { return true || helper(); }", true);
}

void static_priority() {
  try {
    evaluate("int main() { return 1 / 0; } void helper() { missing; }");
    throw std::runtime_error("Expected a name error before execution.");
  } catch (const minic::ResolveError& error) {
    require(std::string(error.what()) == "Undeclared name \"missing\".", "Unexpected name error.");
  }
  try {
    evaluate("int main() { return 1 / 0; } int helper() { return true; }");
    throw std::runtime_error("Expected a type error before execution.");
  } catch (const minic::TypeError& error) {
    require(std::string(error.what()) == "Expected type int. Received bool.", "Unexpected type error.");
  }
}

void ownership() {
  const auto program = minic::parse(minic::lex("int main() { int x = 0; return x = x + 1; }"));
  const auto before = minic::print_ast(program);
  require(std::get<std::int64_t>(minic::interpret(program)) == 1 &&
              std::get<std::int64_t>(minic::interpret(program)) == 1,
          "Expected independent state for repeated executions.");
  require(minic::print_ast(program) == before, "Expected execution to preserve the AST.");
  const auto owned = evaluate("int main() { return 42; }");
  require(std::get<std::int64_t>(owned) == 42, "Expected the value to outlive its temporary AST.");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"arithmetic", arithmetic}, {"booleans", booleans}, {"assignments", assignments},
      {"scopes", scopes}, {"initialization", initialization}, {"evaluation_order", evaluation_order},
      {"short_circuit", short_circuit}, {"returns", returns}, {"void_returns", void_returns},
      {"entry_selection", entry_selection}, {"numeric_limits", numeric_limits}, {"numeric_errors", numeric_errors},
      {"unsupported", unsupported}, {"skipped_nodes", skipped_nodes}, {"static_priority", static_priority},
      {"ownership", ownership},
      {"conditionals", conditionals}, {"loops", loops}, {"loop_environments", loop_environments},
      {"control_returns", control_returns}, {"step_limits", step_limits},
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
