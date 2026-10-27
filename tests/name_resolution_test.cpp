#include "ast_printer.hpp"
#include "lexer.hpp"
#include "name_resolution.hpp"
#include "parser.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void expect_error(std::string_view source, minic::SourceLocation location, const std::string& message) {
  const auto program = minic::parse(minic::lex(source));
  try {
    minic::resolve_names(program);
  } catch (const minic::ResolveError& error) {
    require(error.location.line == location.line && error.location.column == location.column,
            "Unexpected error position: " + std::to_string(error.location.line) + ":" +
                std::to_string(error.location.column));
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a resolution error.");
}

const minic::Symbol& bound_symbol(const minic::NameResolution& result, std::size_t index) {
  return result.table().symbol(result.bindings().at(index).symbol);
}

void empty() {
  const auto program = minic::parse(minic::lex(""));
  const auto result = minic::resolve_names(program);
  require(result.table().symbols().empty() && result.bindings().empty(), "Expected an empty result.");
  require(result.table().scopes().size() == 1, "Expected the global scope.");
}

void bindings() {
  const auto program = minic::parse(minic::lex("int f(int p) { int x; x = p + 1; return x; }"));
  const auto before = minic::print_ast(program);
  auto result = minic::resolve_names(program);
  require(result.bindings().size() == 3, "Expected a write and two reads.");
  const auto& statements = program.functions[0].body.statements;
  const auto& assignment = std::get<minic::AssignmentExpr>(
      std::get<minic::ExprStmt>(statements[1]->node).expression->node);
  const auto& sum = std::get<minic::BinaryExpr>(assignment.value->node);
  require(result.binding(*assignment.target)->kind == minic::UseKind::Write, "Expected a target binding.");
  require(result.binding(*sum.left)->kind == minic::UseKind::Read, "Expected an operand binding.");
  require(!result.binding(*sum.right), "Expected no binding for a literal.");
  require(bound_symbol(result, 0).name == "x" && bound_symbol(result, 1).name == "p",
          "Unexpected use order.");
  require(result.bindings()[0].symbol == result.bindings()[2].symbol,
          "Expected the same variable for the write and return.");
  require(result.table().scopes().size() == 2, "Expected one shared parameter and body scope.");
  require(minic::print_ast(program) == before, "Expected name resolution to preserve the AST.");
  auto moved = std::move(result);
  require(moved.binding(*assignment.target)->symbol == moved.bindings()[0].symbol,
          "Expected binding queries to survive a result move.");
}

void undeclared() {
  expect_error("int f() {\n  return missing;\n}", {2, 10}, "Undeclared name \"missing\".");
  expect_error("int f() {\n  missing = 1;\n}", {2, 3}, "Undeclared name \"missing\".");
  expect_error("int f() {\n  missing();\n}", {2, 3}, "Undeclared name \"missing\".");
  expect_error("int f() {\n  x = y;\n}", {2, 3}, "Undeclared name \"x\".");
  expect_error("int f() {\n  missing(other);\n}", {2, 3}, "Undeclared name \"missing\".");
}

void redeclarations() {
  expect_error("int f() {}\nint f() {}", {2, 5}, "Name \"f\" is already declared in this scope.");
  expect_error("int f(int x,\n bool x) {}", {2, 7}, "Name \"x\" is already declared in this scope.");
  expect_error("int f(int x) {\n bool x; }", {2, 7}, "Name \"x\" is already declared in this scope.");
  expect_error("int f() { int x;\n int x; }", {2, 6}, "Name \"x\" is already declared in this scope.");
  expect_error("int f() { missing; }\nint f() {}", {2, 5},
               "Name \"f\" is already declared in this scope.");
}

void shadowing() {
  const auto program = minic::parse(minic::lex(
      "int f(int x) { { x; int x = x; x; } { int x; x; } return x; }"));
  const auto result = minic::resolve_names(program);
  const auto& uses = result.bindings();
  require(uses.size() == 5 && result.table().scopes().size() == 4, "Unexpected nested scopes.");
  require(bound_symbol(result, 0).kind == minic::SymbolKind::Parameter, "Expected lookup before the local declaration.");
  require(uses[1].symbol == uses[2].symbol, "Expected the initializer and read to use the inner variable.");
  require(!(uses[0].symbol == uses[1].symbol) && !(uses[1].symbol == uses[3].symbol),
          "Expected distinct symbols for shadowed declarations.");
  require(uses[0].symbol == uses[4].symbol, "Expected the parameter after both nested blocks.");
  require(result.table().scope(uses[1].scope).parent == minic::ScopeId{1}, "Unexpected block parent.");
  const auto case_program = minic::parse(minic::lex("int f(int X) { int x = X; return x; }"));
  const auto cases = minic::resolve_names(case_program);
  require(bound_symbol(cases, 0).name == "X" && bound_symbol(cases, 1).name == "x",
          "Expected case-sensitive names.");
}

void isolation() {
  expect_error("int f() { { int x; }\n x; }", {2, 2}, "Undeclared name \"x\".");
  expect_error("int f() { { int x; } {\n x; } }", {2, 2}, "Undeclared name \"x\".");
  expect_error("int f(int x) {}\nint g() { x; }", {2, 11}, "Undeclared name \"x\".");
  expect_error("int f() { int x; }\nint g() { x; }", {2, 11}, "Undeclared name \"x\".");
  expect_error("int f() { if (true) { int x; } else {\n x; } }", {2, 2}, "Undeclared name \"x\".");
  expect_error("int f() { while (true) { int x; }\n x; }", {2, 2}, "Undeclared name \"x\".");
}

void visibility() {
  expect_error("int f() {\n x = 1; int x; }", {2, 2}, "Undeclared name \"x\".");
  expect_error("int f() { int x =\n later; int later; }", {2, 2}, "Undeclared name \"later\".");
  const auto program = minic::parse(minic::lex("int f() { int x = x; return x; }"));
  const auto result = minic::resolve_names(program);
  require(result.bindings().size() == 2 && result.bindings()[0].symbol == result.bindings()[1].symbol,
          "Expected a self-reference without initialization analysis.");
  const auto& variable = std::get<minic::VarDecl>(program.functions[0].body.statements[0]->node);
  require(std::get<const minic::VarDecl*>(bound_symbol(result, 0).declaration) == &variable,
          "Expected the initializer to bind to its own declaration.");
}

void functions() {
  const auto program = minic::parse(minic::lex(
      "int first() { return second(); } int second() { first(); return second(); }"));
  const auto result = minic::resolve_names(program);
  require(result.bindings().size() == 3, "Expected forward, mutual, and self-recursive calls.");
  require(bound_symbol(result, 0).name == "second" && bound_symbol(result, 1).name == "first" &&
              result.bindings()[0].symbol == result.bindings()[2].symbol,
          "Unexpected recursive function bindings.");
  const auto order_program = minic::parse(minic::lex(
      "int f() {} int g() { f(); int f; f; }"));
  const auto order = minic::resolve_names(order_program);
  require(bound_symbol(order, 0).kind == minic::SymbolKind::Function &&
              bound_symbol(order, 1).kind == minic::SymbolKind::Variable,
          "Expected local shadowing only after the declaration.");
}

void categories() {
  expect_error("int f() {\n f; }", {2, 2}, "Name \"f\" does not refer to a variable or parameter.");
  expect_error("int f() {\n f = 1; }", {2, 2}, "Name \"f\" does not refer to a variable or parameter.");
  expect_error("int f() { int f;\n f(); }", {2, 2}, "Name \"f\" does not refer to a function.");
  expect_error("int f(int f) {\n f(); }", {2, 2}, "Name \"f\" does not refer to a function.");
}

void traversal() {
  const auto program = minic::parse(minic::lex(
      "int g() {} int f(int x) { ; if (x) x = -x; else { x = g(x, g()); } "
      "while (x) { int y = x; x = y; } return; }"));
  const auto result = minic::resolve_names(program);
  const auto& uses = result.bindings();
  require(uses.size() == 11, "Expected traversal of every expression and control-flow child.");
  require(result.table().scopes().size() == 5, "Expected scopes only for functions and explicit blocks.");
  require(uses[0].kind == minic::UseKind::Read && uses[1].kind == minic::UseKind::Write &&
              uses[4].kind == minic::UseKind::Call && uses[5].kind == minic::UseKind::Read &&
              uses[6].kind == minic::UseKind::Call,
          "Unexpected condition, assignment, or argument traversal order.");
  expect_error("int f() { if (true) ; else\n missing; }", {2, 2}, "Undeclared name \"missing\".");
  expect_error("int f() { while (\n missing) ; }", {2, 2}, "Undeclared name \"missing\".");
}

void names_only() {
  const auto program = minic::parse(minic::lex(
      "void f(void p) { void x; bool y = 1; x; f(true, 42); "
      "if (42) while (true + 1) ; return 123456789012345678901234567890; }"));
  const auto result = minic::resolve_names(program);
  require(result.table().symbols().size() == 4 && result.bindings().size() == 2,
          "Expected name resolution without type, arity, numeric, or initialization analysis.");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"bindings", bindings}, {"undeclared", undeclared},
      {"redeclarations", redeclarations}, {"shadowing", shadowing}, {"isolation", isolation},
      {"visibility", visibility}, {"functions", functions}, {"categories", categories},
      {"traversal", traversal}, {"names_only", names_only},
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
