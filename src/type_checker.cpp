#include "type_checker.hpp"

#include <utility>
#include <vector>

namespace minic {

class TypeChecker {
 public:
  TypeAnalysis check(const Program& program) {
    result_.resolution_ = resolve_names(program);
    for (const auto& function : program.functions) {
      Signature signature{detail::syntax_type(function.return_type), {}};
      for (const auto& parameter : function.parameters) {
        signature.parameters.push_back(detail::value_type(parameter.type));
      }
      signatures_.emplace(&function, std::move(signature));
    }
    for (const auto& function : program.functions) {
      const auto return_type = signatures_.at(&function).result;
      const bool returns = block(function.body, return_type);
      if (return_type != Type::Void && !returns) {
        throw TypeError(function.location, "Function \"" + function.name + "\" does not guarantee a return value.");
      }
    }
    return std::move(result_);
  }

 private:
  struct Signature {
    Type result;
    std::vector<Type> parameters;
  };

  const Symbol& symbol(const Expr& expression) const {
    const auto* binding = result_.resolution_.binding(expression);
    if (!binding) {
      throw std::logic_error("Expected a resolved name.");
    }
    return result_.resolution_.table().symbol(binding->symbol);
  }

  Type expression(const Expr& expression) {
    const auto type = std::visit([&](const auto& node) { return visit(node, expression); }, expression.node);
    result_.types_.emplace(&expression, type);
    return type;
  }

  Type visit(const IntegerExpr&, const Expr&) { return Type::Int; }
  Type visit(const BooleanExpr&, const Expr&) { return Type::Bool; }

  Type visit(const NameExpr&, const Expr& source) {
    const auto& declaration = symbol(source).declaration;
    if (const auto* parameter = std::get_if<const Parameter*>(&declaration)) {
      return detail::syntax_type((*parameter)->type);
    }
    return detail::syntax_type(std::get<const VarDecl*>(declaration)->type);
  }

  Type visit(const UnaryExpr& node, const Expr& source) {
    const auto operand = expression(*node.operand);
    return detail::unary_type(node.op, operand, source.location);
  }

  Type visit(const BinaryExpr& node, const Expr& source) {
    const auto left = expression(*node.left);
    const auto right = expression(*node.right);
    return detail::binary_type(node.op, left, right, source.location);
  }

  Type visit(const AssignmentExpr& node, const Expr& source) {
    const auto target = expression(*node.target);
    const auto value = expression(*node.value);
    detail::require_type(value, target, source.location);
    return target;
  }

  Type visit(const CallExpr& node, const Expr& source) {
    const auto* declaration = std::get<const FunctionDecl*>(symbol(source).declaration);
    const auto& signature = signatures_.at(declaration);
    if (node.arguments.size() != signature.parameters.size()) {
      const auto count = signature.parameters.size();
      throw TypeError(source.location, "Function \"" + node.name + "\" expects " + std::to_string(count) +
                          (count == 1 ? " argument. Received " : " arguments. Received ") +
                          std::to_string(node.arguments.size()) + '.');
    }
    for (std::size_t i = 0; i < node.arguments.size(); ++i) {
      const auto argument = expression(*node.arguments[i]);
      detail::require_type(argument, signature.parameters[i], node.arguments[i]->location);
    }
    return signature.result;
  }

  bool statement(const Stmt& statement, Type return_type) {
    return std::visit([&](const auto& node) { return visit(node, return_type); }, statement.node);
  }

  bool block(const BlockStmt& node, Type return_type) {
    bool returns = false;
    for (const auto& child : node.statements) {
      const bool child_returns = statement(*child, return_type);
      returns = returns || child_returns;
    }
    return returns;
  }

  bool visit(const BlockStmt& node, Type return_type) { return block(node, return_type); }

  bool visit(const VarDecl& node, Type) {
    const auto declared = detail::value_type(node.type);
    if (node.initializer) {
      const auto initializer = expression(*node.initializer);
      detail::require_type(initializer, declared, node.initializer->location);
    }
    return false;
  }

  bool visit(const ExprStmt& node, Type) {
    expression(*node.expression);
    return false;
  }

  bool visit(const EmptyStmt&, Type) { return false; }

  bool visit(const ReturnStmt& node, Type return_type) {
    if (return_type == Type::Void) {
      if (node.value) {
        throw TypeError(node.location, "A void function cannot return a value.");
      }
    } else {
      if (!node.value) {
        throw TypeError(node.location, "A non-void function must return a value.");
      }
      const auto value = expression(*node.value);
      detail::require_type(value, return_type, node.location);
    }
    return true;
  }

  bool visit(const IfStmt& node, Type return_type) {
    const auto condition = expression(*node.condition);
    detail::require_type(condition, Type::Bool, node.condition->location);
    const bool then_returns = statement(*node.then_branch, return_type);
    const bool else_returns = node.else_branch ? statement(*node.else_branch, return_type) : false;
    return then_returns && else_returns;
  }

  bool visit(const WhileStmt& node, Type return_type) {
    const auto condition = expression(*node.condition);
    detail::require_type(condition, Type::Bool, node.condition->location);
    statement(*node.body, return_type);
    return false;
  }

  TypeAnalysis result_;
  std::unordered_map<const FunctionDecl*, Signature> signatures_;
};

std::optional<Type> TypeAnalysis::type_of(const Expr& expression) const {
  const auto found = types_.find(&expression);
  return found == types_.end() ? std::nullopt : std::optional<Type>{found->second};
}

TypeAnalysis check_types(const Program& program) { return TypeChecker().check(program); }

}  // namespace minic
