#include "interpreter.hpp"

#include "type_checker.hpp"

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace minic {
namespace {

using Environment = std::unordered_map<std::size_t, std::optional<Value>>;

class EnvironmentScope {
 public:
  explicit EnvironmentScope(std::vector<Environment>& environments) : environments_(environments) {
    environments_.emplace_back();
  }
  ~EnvironmentScope() { environments_.pop_back(); }
  EnvironmentScope(const EnvironmentScope&) = delete;
  EnvironmentScope& operator=(const EnvironmentScope&) = delete;

 private:
  std::vector<Environment>& environments_;
};

class Interpreter {
 public:
  explicit Interpreter(const Program& program) : analysis_(check_types(program)) {
    const auto& symbols = analysis_.resolution().table().symbols();
    for (std::size_t i = 0; i < symbols.size(); ++i) {
      if (const auto* declaration = std::get_if<const VarDecl*>(&symbols[i].declaration)) {
        declarations_.emplace(*declaration, i);
      }
    }
  }

  Value run(const std::string& entry) {
    const auto& table = analysis_.resolution().table();
    const auto id = table.lookup(SymbolTable::global_scope(), entry);
    if (!id) {
      throw RuntimeError({1, 1}, "The entry function \"" + entry + "\" does not exist.");
    }
    const auto* function = std::get<const FunctionDecl*>(table.symbol(*id).declaration);
    if (!function->parameters.empty()) {
      throw RuntimeError(function->location, "The entry function \"" + entry + "\" must not have parameters.");
    }
    const auto returned = block(function->body);
    if (returned) {
      return *returned;
    }
    if (function->return_type.keyword != TokenKind::KwVoid) {
      throw std::logic_error("A typed entry function ended without a return value.");
    }
    return Value{};
  }

 private:
  std::optional<Value>& slot(const Expr& expression) {
    const auto* binding = analysis_.resolution().binding(expression);
    if (!binding) {
      throw std::logic_error("The expression has no resolved binding.");
    }
    for (auto environment = environments_.rbegin(); environment != environments_.rend(); ++environment) {
      const auto found = environment->find(binding->symbol.index);
      if (found != environment->end()) {
        return found->second;
      }
    }
    throw std::logic_error("The resolved variable has no active runtime slot.");
  }

  Value expression(const Expr& expression) {
    return std::visit([&](const auto& node) { return evaluate(node, expression); }, expression.node);
  }

  Value evaluate(const IntegerExpr& node, const Expr& source) {
    return detail::parse_integer(node.text, false, source.location);
  }

  Value evaluate(const BooleanExpr& node, const Expr&) { return node.value; }

  Value evaluate(const NameExpr& node, const Expr& source) {
    const auto& value = slot(source);
    if (!value) {
      throw RuntimeError(source.location, "The variable \"" + node.name + "\" has no value.");
    }
    return *value;
  }

  Value evaluate(const UnaryExpr& node, const Expr& source) {
    if (node.op == TokenKind::Minus) {
      if (const auto* literal = std::get_if<IntegerExpr>(&node.operand->node)) {
        return detail::parse_integer(literal->text, true, source.location);
      }
    }
    const auto operand = expression(*node.operand);
    return detail::runtime_unary(node.op, operand, source.location);
  }

  Value evaluate(const BinaryExpr& node, const Expr& source) {
    const auto left = expression(*node.left);
    if (node.op == TokenKind::AndAnd && !std::get<bool>(left)) {
      return false;
    }
    if (node.op == TokenKind::OrOr && std::get<bool>(left)) {
      return true;
    }
    const auto right = expression(*node.right);
    return detail::runtime_binary(node.op, left, right, source.location);
  }

  Value evaluate(const AssignmentExpr& node, const Expr&) {
    const auto value = expression(*node.value);
    slot(*node.target) = value;
    return value;
  }

  Value evaluate(const CallExpr&, const Expr& source) {
    throw RuntimeError(source.location, "The interpreter does not support function calls.");
  }

  std::optional<Value> statement(const Stmt& statement) {
    return std::visit([&](const auto& node) { return execute(node); }, statement.node);
  }

  std::optional<Value> block(const BlockStmt& block) {
    const EnvironmentScope scope(environments_);
    for (const auto& child : block.statements) {
      auto returned = statement(*child);
      if (returned) {
        return returned;
      }
    }
    return std::nullopt;
  }

  std::optional<Value> execute(const BlockStmt& node) { return block(node); }

  std::optional<Value> execute(const VarDecl& node) {
    const auto id = declarations_.at(&node);
    environments_.back().emplace(id, std::nullopt);
    if (node.initializer) {
      auto value = expression(*node.initializer);
      environments_.back().at(id) = std::move(value);
    }
    return std::nullopt;
  }

  std::optional<Value> execute(const ExprStmt& node) {
    expression(*node.expression);
    return std::nullopt;
  }

  std::optional<Value> execute(const EmptyStmt&) { return std::nullopt; }

  std::optional<Value> execute(const ReturnStmt& node) {
    if (node.value) {
      return expression(*node.value);
    }
    return Value{};
  }

  std::optional<Value> execute(const IfStmt& node) {
    throw RuntimeError(node.location, "The interpreter does not support if statements.");
  }

  std::optional<Value> execute(const WhileStmt& node) {
    throw RuntimeError(node.location, "The interpreter does not support while statements.");
  }

  TypeAnalysis analysis_;
  std::unordered_map<const VarDecl*, std::size_t> declarations_;
  std::vector<Environment> environments_;
};

}  // namespace

Value interpret(const Program& program, const std::string& entry) { return Interpreter(program).run(entry); }

}  // namespace minic
