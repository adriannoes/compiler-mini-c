#include "name_resolution.hpp"

#include <utility>

namespace minic {

class NameResolver {
 public:
  NameResolution resolve(const Program& program) {
    const auto global = SymbolTable::global_scope();
    for (const auto& function : program.functions) {
      result_.table_.declare(global, function);
    }
    for (const auto& function : program.functions) {
      const auto scope = result_.table_.add_scope(global, ScopeKind::Function, function.location);
      for (const auto& parameter : function.parameters) {
        result_.table_.declare(scope, parameter);
      }
      block_contents(function.body, scope);
    }
    return std::move(result_);
  }

 private:
  void bind(const Expr& expression, const std::string& name, ScopeId scope, UseKind kind) {
    const auto id = result_.table_.lookup(scope, name);
    if (!id) {
      throw ResolveError(expression.location, "Undeclared name \"" + name + "\".");
    }
    const auto symbol_kind = result_.table_.symbol(*id).kind;
    if (kind == UseKind::Call && symbol_kind != SymbolKind::Function) {
      throw ResolveError(expression.location, "Name \"" + name + "\" does not refer to a function.");
    }
    if (kind != UseKind::Call && symbol_kind == SymbolKind::Function) {
      throw ResolveError(expression.location, "Name \"" + name + "\" does not refer to a variable or parameter.");
    }
    result_.binding_indices_.emplace(&expression, result_.bindings_.size());
    result_.bindings_.push_back({&expression, *id, scope, kind});
  }

  void expression(const Expr& expression, ScopeId scope) {
    std::visit([&](const auto& node) { visit(node, expression, scope); }, expression.node);
  }

  void visit(const IntegerExpr&, const Expr&, ScopeId) {}
  void visit(const BooleanExpr&, const Expr&, ScopeId) {}

  void visit(const NameExpr& node, const Expr& source, ScopeId scope) {
    bind(source, node.name, scope, UseKind::Read);
  }

  void visit(const IndexExpr& node, const Expr& source, ScopeId scope) {
    bind(source, node.name, scope, UseKind::Read);
    expression(*node.index, scope);
  }

  void visit(const UnaryExpr& node, const Expr&, ScopeId scope) {
    expression(*node.operand, scope);
  }

  void visit(const BinaryExpr& node, const Expr&, ScopeId scope) {
    expression(*node.left, scope);
    expression(*node.right, scope);
  }

  void visit(const AssignmentExpr& node, const Expr&, ScopeId scope) {
    if (const auto* index = std::get_if<IndexExpr>(&node.target->node)) {
      bind(*node.target, index->name, scope, UseKind::Write);
      expression(*index->index, scope);
    } else {
      bind(*node.target, std::get<NameExpr>(node.target->node).name, scope, UseKind::Write);
    }
    expression(*node.value, scope);
  }

  void visit(const CallExpr& node, const Expr& source, ScopeId scope) {
    bind(source, node.name, scope, UseKind::Call);
    for (const auto& argument : node.arguments) {
      expression(*argument, scope);
    }
  }

  void statement(const Stmt& statement, ScopeId scope) {
    std::visit([&](const auto& node) { visit(node, scope); }, statement.node);
  }

  void block_contents(const BlockStmt& node, ScopeId scope) {
    for (const auto& child : node.statements) {
      statement(*child, scope);
    }
  }

  void visit(const BlockStmt& node, ScopeId parent) {
    const auto scope = result_.table_.add_scope(parent, ScopeKind::Block, node.location);
    block_contents(node, scope);
  }

  void visit(const VarDecl& node, ScopeId scope) {
    result_.table_.declare(scope, node);
    if (node.initializer) {
      expression(*node.initializer, scope);
    }
  }

  void visit(const ReturnStmt& node, ScopeId scope) {
    if (node.value) {
      expression(*node.value, scope);
    }
  }

  void visit(const ExprStmt& node, ScopeId scope) { expression(*node.expression, scope); }
  void visit(const EmptyStmt&, ScopeId) {}

  void visit(const IfStmt& node, ScopeId scope) {
    expression(*node.condition, scope);
    statement(*node.then_branch, scope);
    if (node.else_branch) {
      statement(*node.else_branch, scope);
    }
  }

  void visit(const WhileStmt& node, ScopeId scope) {
    expression(*node.condition, scope);
    statement(*node.body, scope);
  }

  NameResolution result_;
};

const Binding* NameResolution::binding(const Expr& expression) const {
  const auto found = binding_indices_.find(&expression);
  return found == binding_indices_.end() ? nullptr : &bindings_[found->second];
}

NameResolution resolve_names(const Program& program) { return NameResolver().resolve(program); }

}  // namespace minic
