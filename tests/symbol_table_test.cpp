#include "lexer.hpp"
#include "parser.hpp"
#include "symbol_table.hpp"

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

void declarations() {
  const auto program = minic::parse(minic::lex("int f(int p) { int x; }"));
  const auto& function = program.functions[0];
  const auto& variable = std::get<minic::VarDecl>(function.body.statements[0]->node);
  minic::SymbolTable table;
  const auto root = table.global_scope();
  const auto function_id = table.declare(root, function);
  const auto body = table.add_scope(root, minic::ScopeKind::Function, function.body.location);
  const auto parameter_id = table.declare(body, function.parameters[0]);
  const auto variable_id = table.declare(body, variable);
  require(table.symbol(function_id).kind == minic::SymbolKind::Function, "Expected a function symbol.");
  require(table.symbol(parameter_id).kind == minic::SymbolKind::Parameter, "Expected a parameter symbol.");
  const auto& symbol = table.symbol(variable_id);
  require(symbol.name == "x" && symbol.kind == minic::SymbolKind::Variable && symbol.scope == body,
          "Unexpected variable symbol.");
  require(symbol.location.line == 1 && symbol.location.column == 20, "Unexpected declaration position.");
  require(std::get<const minic::VarDecl*>(symbol.declaration) == &variable, "Expected the AST declaration.");
  require(std::get<const minic::FunctionDecl*>(table.symbol(function_id).declaration) == &function,
          "Expected the function declaration.");
  require(std::get<const minic::Parameter*>(table.symbol(parameter_id).declaration) == &function.parameters[0],
          "Expected the parameter declaration.");
  require(!table.scope(root).parent && table.scope(body).parent == root, "Unexpected scope parents.");
}

void scopes() {
  const auto program = minic::parse(minic::lex("int x() {} int f(int x) { { int x; } }"));
  const auto& function = program.functions[1];
  const auto& block = std::get<minic::BlockStmt>(function.body.statements[0]->node);
  const auto& variable = std::get<minic::VarDecl>(block.statements[0]->node);
  minic::SymbolTable table;
  const auto root = table.global_scope();
  const auto global = table.declare(root, program.functions[0]);
  const auto body = table.add_scope(root, minic::ScopeKind::Function, function.body.location);
  const auto parameter = table.declare(body, function.parameters[0]);
  const auto inner = table.add_scope(body, minic::ScopeKind::Block, block.location);
  require(table.lookup(inner, "x") == parameter, "Expected lookup in the parent scope.");
  const auto local = table.declare(inner, variable);
  const auto sibling = table.add_scope(body, minic::ScopeKind::Block, block.location);
  const auto other = table.add_scope(root, minic::ScopeKind::Function, function.body.location);
  require(table.lookup(inner, "x") == local, "Expected the nearest declaration.");
  require(table.lookup(sibling, "x") == parameter, "Expected sibling isolation.");
  require(table.lookup(body, "x") == parameter, "Expected the parameter outside the block.");
  require(table.lookup(other, "x") == global, "Expected function isolation.");
  require(table.lookup(root, "x") == global, "Expected the global declaration.");
  require(!table.lookup(inner, "missing"), "Expected no symbol for an undeclared name.");
}

void redeclarations() {
  const auto program = minic::parse(minic::lex("int f(int x) { bool x; } int f() {}"));
  minic::SymbolTable table;
  const auto root = table.global_scope();
  table.declare(root, program.functions[0]);
  try {
    table.declare(root, program.functions[1]);
    throw std::runtime_error("Expected a duplicate function error.");
  } catch (const minic::ResolveError& error) {
    require(std::string(error.what()) == "Name \"f\" is already declared in this scope.",
            "Unexpected duplicate function diagnostic.");
  }
  const auto& function = program.functions[0];
  const auto body = table.add_scope(root, minic::ScopeKind::Function, function.body.location);
  const auto original = table.declare(body, function.parameters[0]);
  const auto& variable = std::get<minic::VarDecl>(function.body.statements[0]->node);
  try {
    table.declare(body, variable);
    throw std::runtime_error("Expected a parameter collision error.");
  } catch (const minic::ResolveError& error) {
    require(error.location.column == variable.location.column, "Expected the second declaration position.");
    require(std::string(error.what()) == "Name \"x\" is already declared in this scope.",
            "Unexpected collision diagnostic.");
  }
  require(table.lookup(body, "x") == original && table.symbols().size() == 2,
          "Expected a rejected declaration to preserve the original symbol.");
}

void stable_ids() {
  std::string source;
  for (int i = 0; i < 64; ++i) {
    source += "int f" + std::to_string(i) + "() {}";
  }
  const auto program = minic::parse(minic::lex(source));
  minic::SymbolTable table;
  const auto root = table.global_scope();
  const auto first = table.declare(root, program.functions[0]);
  for (std::size_t i = 1; i < program.functions.size(); ++i) {
    table.declare(root, program.functions[i]);
    table.add_scope(root, minic::ScopeKind::Function, program.functions[i].body.location);
  }
  auto moved = std::move(table);
  require(moved.lookup(root, "f0") == first && moved.symbol(first).name == "f0",
          "Expected stable identifiers after growth and a table move.");
  require(std::get<const minic::FunctionDecl*>(moved.symbol(first).declaration) == &program.functions[0],
          "Expected the borrowed AST declaration to remain unchanged.");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"declarations", declarations}, {"scopes", scopes},
      {"redeclarations", redeclarations}, {"stable_ids", stable_ids},
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
