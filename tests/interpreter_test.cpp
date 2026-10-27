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
  expect_error("int main() {\n  if (true) return 1;\n  return 0; }", {2, 3},
               "The interpreter does not support if statements.");
  expect_error("int main() {\n  while (true) ;\n  return 0; }", {2, 3},
               "The interpreter does not support while statements.");
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
