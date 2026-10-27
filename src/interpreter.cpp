#include "interpreter.hpp"

#include "type_checker.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace minic {
namespace {

using Environment = std::unordered_map<std::size_t, std::optional<Value>>;

struct CallFrame {
  std::vector<Environment> environments;
};

class FrameScope {
 public:
  explicit FrameScope(std::vector<std::unique_ptr<CallFrame>>& frames) : frames_(frames) {
    frames_.push_back(std::make_unique<CallFrame>());
  }
  ~FrameScope() { frames_.pop_back(); }
  FrameScope(const FrameScope&) = delete;
  FrameScope& operator=(const FrameScope&) = delete;

 private:
  std::vector<std::unique_ptr<CallFrame>>& frames_;
};

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
  Interpreter(const Program& program, ExecutionLimits limits) : analysis_(check_types(program)), limits_(limits) {
    const auto& symbols = analysis_.resolution().table().symbols();
    for (std::size_t i = 0; i < symbols.size(); ++i) {
      if (const auto* declaration = std::get_if<const VarDecl*>(&symbols[i].declaration)) {
        declarations_.emplace(*declaration, i);
      } else if (const auto* parameter = std::get_if<const Parameter*>(&symbols[i].declaration)) {
        parameters_.emplace(*parameter, i);
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
    return invoke(*function, {}, function->location);
  }

 private:
  Value invoke(const FunctionDecl& function, const std::vector<Value>& arguments, SourceLocation call_location) {
    if (frames_.size() == limits_.max_call_depth) {
      throw RuntimeError(call_location, "The call depth limit was reached.");
    }
    const FrameScope frame(frames_);
    const EnvironmentScope environment(frames_.back()->environments);
    for (std::size_t i = 0; i < function.parameters.size(); ++i) {
      const auto id = parameters_.at(&function.parameters[i]);
      frames_.back()->environments.back().emplace(id, arguments.at(i));
    }
    step(function.body.location);
    const auto returned = block_contents(function.body);
    if (returned) {
      return *returned;
    }
    if (function.return_type.keyword != TokenKind::KwVoid) {
      throw std::logic_error("A typed function ended without a return value.");
    }
    return Value{};
  }

  void step(SourceLocation location) {
    if (steps_ == limits_.max_steps) {
      throw RuntimeError(location, "The execution step limit was reached.");
    }
    ++steps_;
  }

  const Binding& binding(const Expr& expression) const {
    const auto* found = analysis_.resolution().binding(expression);
    if (!found) {
      throw std::logic_error("The expression has no resolved binding.");
    }
    return *found;
  }

  std::optional<Value>& slot(const Expr& expression) {
    const auto id = binding(expression).symbol.index;
    auto& environments = frames_.back()->environments;
    for (auto environment = environments.rbegin(); environment != environments.rend(); ++environment) {
      const auto found = environment->find(id);
      if (found != environment->end()) {
        return found->second;
      }
    }
    throw std::logic_error("The resolved variable has no active runtime slot.");
  }

  Value expression(const Expr& expression) {
    step(expression.location);
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
        step(node.operand->location);
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

  Value evaluate(const CallExpr& node, const Expr& source) {
    const auto& symbol = analysis_.resolution().table().symbol(binding(source).symbol);
    const auto* function = std::get<const FunctionDecl*>(symbol.declaration);
    std::vector<Value> arguments;
    arguments.reserve(node.arguments.size());
    for (const auto& argument : node.arguments) {
      arguments.push_back(expression(*argument));
    }
    return invoke(*function, arguments, source.location);
  }

  std::optional<Value> statement(const Stmt& statement) {
    return std::visit([&](const auto& node) {
      step(node.location);
      return execute(node);
    }, statement.node);
  }

  std::optional<Value> block(const BlockStmt& block) {
    const EnvironmentScope scope(frames_.back()->environments);
    return block_contents(block);
  }

  std::optional<Value> block_contents(const BlockStmt& block) {
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
    frames_.back()->environments.back().emplace(id, std::nullopt);
    if (node.initializer) {
      auto value = expression(*node.initializer);
      frames_.back()->environments.back().at(id) = std::move(value);
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
    if (std::get<bool>(expression(*node.condition))) {
      return statement(*node.then_branch);
    }
    return node.else_branch ? statement(*node.else_branch) : std::nullopt;
  }

  std::optional<Value> execute(const WhileStmt& node) {
    while (std::get<bool>(expression(*node.condition))) {
      auto returned = statement(*node.body);
      if (returned) {
        return returned;
      }
    }
    return std::nullopt;
  }

  TypeAnalysis analysis_;
  std::unordered_map<const VarDecl*, std::size_t> declarations_;
  std::unordered_map<const Parameter*, std::size_t> parameters_;
  std::vector<std::unique_ptr<CallFrame>> frames_;
  ExecutionLimits limits_;
  std::size_t steps_ = 0;
};

}  // namespace

Value interpret(const Program& program, const std::string& entry) {
  return interpret(program, entry, ExecutionLimits{});
}

Value interpret(const Program& program, const std::string& entry, ExecutionLimits limits) {
  if (limits.max_steps == 0 || limits.max_call_depth == 0) {
    throw std::invalid_argument("Execution limits must be greater than zero.");
  }
  return Interpreter(program, limits).run(entry);
}

}  // namespace minic
