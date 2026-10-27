#include "symbol_table.hpp"

namespace minic {

ResolveError::ResolveError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location(location) {}

SymbolTable::SymbolTable() : scopes_{{ScopeKind::Global, {1, 1}, std::nullopt, {}}} {}

ScopeId SymbolTable::add_scope(ScopeId parent, ScopeKind kind, SourceLocation location) {
  scopes_.at(parent.index);
  if (kind == ScopeKind::Global) {
    throw std::invalid_argument("Expected a function or block scope.");
  }
  const ScopeId id{scopes_.size()};
  scopes_.push_back({kind, location, parent, {}});
  return id;
}

SymbolId SymbolTable::declare(ScopeId scope, const FunctionDecl& declaration) {
  return declare(scope, SymbolKind::Function, declaration.name, declaration.location, &declaration);
}

SymbolId SymbolTable::declare(ScopeId scope, const Parameter& declaration) {
  return declare(scope, SymbolKind::Parameter, declaration.name, declaration.location, &declaration);
}

SymbolId SymbolTable::declare(ScopeId scope, const VarDecl& declaration) {
  return declare(scope, SymbolKind::Variable, declaration.name, declaration.location, &declaration);
}

SymbolId SymbolTable::declare(ScopeId scope, SymbolKind kind, const std::string& name,
                              SourceLocation location, Declaration declaration) {
  auto& names = scopes_.at(scope.index).names;
  if (names.find(name) != names.end()) {
    throw ResolveError(location, "Name \"" + name + "\" is already declared in this scope.");
  }
  const SymbolId id{symbols_.size()};
  symbols_.push_back({name, kind, location, scope, declaration});
  names.emplace(name, id);
  return id;
}

std::optional<SymbolId> SymbolTable::lookup(ScopeId scope, const std::string& name) const {
  std::optional<ScopeId> current = scope;
  while (current) {
    const auto& entry = scopes_.at(current->index);
    const auto found = entry.names.find(name);
    if (found != entry.names.end()) {
      return found->second;
    }
    current = entry.parent;
  }
  return std::nullopt;
}

const Symbol& SymbolTable::symbol(SymbolId id) const { return symbols_.at(id.index); }
const Scope& SymbolTable::scope(ScopeId id) const { return scopes_.at(id.index); }

}  // namespace minic
