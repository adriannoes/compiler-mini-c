#pragma once

#include "ast.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace minic {

struct SymbolId {
  std::size_t index;
};

struct ScopeId {
  std::size_t index;
};

inline bool operator==(SymbolId left, SymbolId right) { return left.index == right.index; }
inline bool operator==(ScopeId left, ScopeId right) { return left.index == right.index; }

enum class SymbolKind { Function, Parameter, Variable };
enum class ScopeKind { Global, Function, Block };

using Declaration = std::variant<const FunctionDecl*, const Parameter*, const VarDecl*>;

struct Symbol {
  std::string name;
  SymbolKind kind;
  SourceLocation location;
  ScopeId scope;
  Declaration declaration;
};

struct Scope {
  ScopeKind kind;
  SourceLocation location;
  std::optional<ScopeId> parent;
  std::unordered_map<std::string, SymbolId> names;
};

struct ResolveError : std::runtime_error {
  ResolveError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

class SymbolTable {
 public:
  SymbolTable();

  static constexpr ScopeId global_scope() { return {0}; }
  ScopeId add_scope(ScopeId parent, ScopeKind kind, SourceLocation location);
  SymbolId declare(ScopeId scope, const FunctionDecl& declaration);
  SymbolId declare(ScopeId scope, const Parameter& declaration);
  SymbolId declare(ScopeId scope, const VarDecl& declaration);
  std::optional<SymbolId> lookup(ScopeId scope, const std::string& name) const;

  const Symbol& symbol(SymbolId id) const;
  const Scope& scope(ScopeId id) const;
  const std::vector<Symbol>& symbols() const { return symbols_; }
  const std::vector<Scope>& scopes() const { return scopes_; }

 private:
  SymbolId declare(ScopeId scope, SymbolKind kind, const std::string& name,
                   SourceLocation location, Declaration declaration);

  std::vector<Symbol> symbols_;
  std::vector<Scope> scopes_;
};

}  // namespace minic
