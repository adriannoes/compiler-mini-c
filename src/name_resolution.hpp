#pragma once

#include "symbol_table.hpp"

#include <unordered_map>
#include <vector>

namespace minic {

enum class UseKind { Read, Write, Call };

struct Binding {
  const Expr* expression;
  SymbolId symbol;
  ScopeId scope;
  UseKind kind;
};

class NameResolution {
 public:
  const SymbolTable& table() const { return table_; }
  const std::vector<Binding>& bindings() const { return bindings_; }
  const Binding* binding(const Expr& expression) const;

 private:
  friend class NameResolver;

  SymbolTable table_;
  std::vector<Binding> bindings_;
  std::unordered_map<const Expr*, std::size_t> binding_indices_;
};

NameResolution resolve_names(const Program& program);
NameResolution resolve_names(Program&& program) = delete;
NameResolution resolve_names(const Program&& program) = delete;

}  // namespace minic
