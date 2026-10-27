#include "ast_printer.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "type_checker.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using minic::Type;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void accept(std::string_view source) {
  const auto program = minic::parse(minic::lex(source));
  minic::check_types(program);
}

void expect_error(std::string_view source, minic::SourceLocation location, const std::string& message) {
  const auto program = minic::parse(minic::lex(source));
  try {
    minic::check_types(program);
  } catch (const minic::TypeError& error) {
    require(error.location.line == location.line && error.location.column == location.column,
            "Unexpected error position: " + std::to_string(error.location.line) + ":" +
                std::to_string(error.location.column));
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a type error.");
}

void empty() {
  const auto program = minic::parse(minic::lex(""));
  const auto result = minic::check_types(program);
  require(result.resolution().table().symbols().empty(), "Expected no symbols for an empty program.");
  accept("void utility() {}");
}

void expression_types() {
  const auto program = minic::parse(minic::lex(
      "int f(int p) { int x; x = p + -007; return x; } "
      "bool g(bool b) { return !b || (1 < 2 && true == false); }"));
  const auto before = minic::print_ast(program);
  const auto result = minic::check_types(program);
  const auto& assignment = *std::get<minic::ExprStmt>(program.functions[0].body.statements[1]->node).expression;
  const auto& assigned = std::get<minic::AssignmentExpr>(assignment.node);
  const auto& addition = std::get<minic::BinaryExpr>(assigned.value->node);
  const auto& negation = std::get<minic::UnaryExpr>(addition.right->node);
  require(result.type_of(assignment) == Type::Int && result.type_of(*assigned.target) == Type::Int &&
              result.type_of(*assigned.value) == Type::Int && result.type_of(*addition.left) == Type::Int &&
              result.type_of(*addition.right) == Type::Int && result.type_of(*negation.operand) == Type::Int,
          "Expected types for assignment targets, operands, and results.");
  const auto& boolean = *std::get<minic::ReturnStmt>(program.functions[1].body.statements[0]->node).value;
  const auto& logical_or = std::get<minic::BinaryExpr>(boolean.node);
  const auto& logical_and = std::get<minic::BinaryExpr>(logical_or.right->node);
  require(result.type_of(boolean) == Type::Bool && result.type_of(*logical_or.left) == Type::Bool &&
              result.type_of(*logical_and.left) == Type::Bool && result.type_of(*logical_and.right) == Type::Bool,
          "Expected boolean types for logical, comparison, and equality results.");
  require(minic::print_ast(program) == before, "Expected type analysis to preserve the AST and literal text.");
  const minic::Expr outside{{1, 1}, minic::IntegerExpr{"9"}};
  require(!result.type_of(outside), "Expected no type for an expression outside the analyzed tree.");
  auto copied = result;
  auto moved = std::move(copied);
  require(moved.type_of(assignment) == Type::Int && moved.resolution().binding(*assigned.target),
          "Expected types and owned bindings after result copies and moves.");
}

void declarations() {
  accept("void f() { int x; bool b; int y = 1; bool c = true; }");
  expect_error("void f() {\n  void x;\n}", {2, 3}, "Variables and parameters cannot have type void.");
  expect_error("int f(\n  void x) { return 0; }", {2, 3}, "Variables and parameters cannot have type void.");
  expect_error("void f() {\n  bool x = 1;\n}", {2, 12}, "Expected type bool. Received int.");
  expect_error("void f() {\n  int x = true;\n}", {2, 11}, "Expected type int. Received bool.");
}

void assignments() {
  accept("int f(int a) { int b; return a = b = 3; }");
  accept("bool f(bool a) { bool b; return a = b = false; }");
  expect_error("void f(int x) {\n  x = true;\n}", {2, 5}, "Expected type int. Received bool.");
  expect_error("void f(bool x) {\n  x = 1;\n}", {2, 5}, "Expected type bool. Received int.");
}

void operators() {
  accept("int f(int a, int b) { return -a + b * 2 / 3 % 4 - 1; }");
  accept("bool f(int a, int b, bool p) { return !(a < b) || p && a == b; }");
  expect_error("int f() {\n  return true + 1;\n}", {2, 15}, "Expected type int. Received bool.");
  expect_error("bool f() {\n  return !1;\n}", {2, 10}, "Expected type bool. Received int.");
  expect_error("bool f() {\n  return true == 1;\n}", {2, 15}, "Expected type bool. Received int.");
}

void shadowing() {
  accept("int f(int x) { { bool x = true; x; } return x; }");
  accept("bool f(int x) { { bool x = true; return x; } }");
  expect_error("int f(int x) { { bool x = true;\n  return x; } }", {2, 3},
               "Expected type int. Received bool.");
}

void calls() {
  const auto program = minic::parse(minic::lex("int f() { return g(1); } int g(int x) { return x; }"));
  const auto result = minic::check_types(program);
  const auto& call = *std::get<minic::ReturnStmt>(program.functions[0].body.statements[0]->node).value;
  require(result.type_of(call) == Type::Int, "Expected a forward call to have its declared return type.");
  const auto* binding = result.resolution().binding(call);
  require(binding && result.resolution().table().symbol(binding->symbol).name == "g",
          "Expected the owned resolution to select the forward function.");
  accept("bool even(int n) { if (n == 0) return true; return odd(n - 1); } "
         "bool odd(int n) { if (n == 0) return false; return even(n - 1); }");
  accept("int f(int n) { if (n == 0) return 0; return f(n - 1); }");
}

void arity() {
  expect_error("int f(int x) { return x; }\nvoid g() {\n  f();\n}", {3, 3},
               "Function \"f\" expects 1 argument. Received 0.");
  expect_error("void f() {}\nvoid g() {\n  f(1);\n}", {3, 3},
               "Function \"f\" expects 0 arguments. Received 1.");
  expect_error("void f(int x) {}\nvoid g() {\n  f(true, 1);\n}", {3, 3},
               "Function \"f\" expects 1 argument. Received 2.");
}

void argument_types() {
  accept("void f(int a, bool b) {} void g() { f(1 + 2, true); }");
  expect_error("void f(int a, bool b) {}\nvoid g() {\n  f(true, false);\n}", {3, 5},
               "Expected type int. Received bool.");
  expect_error("void f(int a, bool b) {}\nvoid g() {\n  f(1, 2);\n}", {3, 8},
               "Expected type bool. Received int.");
}

void void_uses() {
  const auto program = minic::parse(minic::lex("void f() { return; } void g() { f(); 1; true; }"));
  const auto result = minic::check_types(program);
  const auto& call = *std::get<minic::ExprStmt>(program.functions[1].body.statements[0]->node).expression;
  require(result.type_of(call) == Type::Void, "Expected a discarded void call result.");
  expect_error("void f() {}\nvoid g() {\n  int x = f();\n}", {3, 11}, "Expected type int. Received void.");
  expect_error("void f() {} void take(int x) {}\nvoid g() {\n  take(f());\n}", {3, 8},
               "Expected type int. Received void.");
  expect_error("void f() {}\nvoid g() {\n  f() == f();\n}", {3, 7}, "Equality requires int or bool operands.");
}

void conditions() {
  accept("void f() { bool b; while (b = true) ; if (b) ; else ; }");
  expect_error("void f() {\n  if (1) ;\n}", {2, 7}, "Expected type bool. Received int.");
  expect_error("void f() {\n  while (1) ;\n}", {2, 10}, "Expected type bool. Received int.");
  expect_error("void g() {} void f() {\n  if (g()) ;\n}", {2, 7}, "Expected type bool. Received void.");
}

void returns() {
  accept("void f() {} void g() { return; } int h() { return 1; } bool i() { return false; }");
  expect_error("void f() {\n  return 1;\n}", {2, 3}, "A void function cannot return a value.");
  expect_error("void f() {\n  return f();\n}", {2, 3}, "A void function cannot return a value.");
  expect_error("int f() {\n  return;\n}", {2, 3}, "A non-void function must return a value.");
  expect_error("int f() {\n  return true;\n}", {2, 3}, "Expected type int. Received bool.");
  expect_error("bool f() {\n  return 1;\n}", {2, 3}, "Expected type bool. Received int.");
}

void return_paths() {
  accept("int f(bool b) { if (b) return 1; else return 2; }");
  accept("int f(bool a, bool b) { if (a) return 1; else if (b) return 2; else return 3; }");
  accept("int f(bool b) { if (b) return 1; return 2; }");
  accept("int f() { { { return 1; } } }");
  accept("int f(bool b) { while (b) { return 1; } return 0; }");
  for (const std::string_view source : {
           "int f() {}", "int f(bool b) { if (b) return 1; }",
           "int f(bool b) { if (b) return 1; else ; }",
           "int f() { while (true) return 1; }", "int f() { if (true) return 1; }"}) {
    expect_error(source, {1, 5}, "Function \"f\" does not guarantee a return value.");
  }
}

void unreachable() {
  expect_error("void f() { return;\n  int x = true;\n}", {2, 11}, "Expected type int. Received bool.");
  expect_error("int f() { return 1;\n  return true;\n}", {2, 3}, "Expected type int. Received bool.");
  expect_error("int f() { if (true) return 1; else {\n  return false; } }", {2, 3},
               "Expected type int. Received bool.");
}

void stage_priority() {
  const auto program = minic::parse(minic::lex("void f() { int x = true; }\nvoid g() { missing; }"));
  try {
    minic::check_types(program);
    throw std::runtime_error("Expected name resolution before type analysis.");
  } catch (const minic::ResolveError& error) {
    require(error.location.line == 2 && error.location.column == 12 &&
                std::string(error.what()) == "Undeclared name \"missing\".",
            "Unexpected name error priority.");
  }
  expect_error("void f() { int x = true; }\nvoid g(void p) {}", {2, 8},
               "Variables and parameters cannot have type void.");
}

void names_and_numbers() {
  accept("int f() { int x = x; return 0001234567890123456789012345678901234567890; }");
  accept("int f() { int x; return x; }");
  accept("int f() { return 1 / 0; }");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"expression_types", expression_types}, {"declarations", declarations},
      {"assignments", assignments}, {"operators", operators}, {"shadowing", shadowing},
      {"calls", calls}, {"arity", arity}, {"argument_types", argument_types}, {"void_uses", void_uses},
      {"conditions", conditions}, {"returns", returns}, {"return_paths", return_paths},
      {"unreachable", unreachable}, {"stage_priority", stage_priority}, {"names_and_numbers", names_and_numbers},
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
