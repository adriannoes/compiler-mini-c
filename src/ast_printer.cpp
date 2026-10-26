#include "ast_printer.hpp"

#include <iomanip>
#include <sstream>

namespace minic {
namespace {

class AstPrinter {
 public:
  std::string print(const Program& program) {
    output_ << "Program\n";
    for (const auto& function : program.functions) {
      indent(1);
      output_ << "Function " << std::quoted(function.name);
      position(function.location);
      indent(2);
      output_ << "Type " << token_kind_name(function.return_type.keyword);
      position(function.return_type.location);
      print(function.body, 2);
    }
    return output_.str();
  }

 private:
  void indent(std::size_t depth) {
    output_ << std::string(depth * 2, ' ');
  }

  void position(SourceLocation location) {
    output_ << " @" << location.line << ':' << location.column << '\n';
  }

  void print(const Expr& expression, std::size_t depth) {
    std::visit([&](const auto& node) { print(node, expression.location, depth); }, expression.node);
  }

  void print(const IntegerExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Integer " << std::quoted(node.text);
    position(location);
  }

  void print(const BooleanExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Boolean " << (node.value ? "true" : "false");
    position(location);
  }

  void print(const NameExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Name " << std::quoted(node.name);
    position(location);
  }

  void print(const UnaryExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Unary " << token_kind_name(node.op);
    position(location);
    print(*node.operand, depth + 1);
  }

  void print(const BinaryExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Binary " << token_kind_name(node.op);
    position(location);
    print(*node.left, depth + 1);
    print(*node.right, depth + 1);
  }

  void print(const AssignmentExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Assign";
    position(location);
    print(*node.target, depth + 1);
    print(*node.value, depth + 1);
  }

  void print(const Stmt& statement, std::size_t depth) {
    std::visit([&](const auto& node) { print(node, depth); }, statement.node);
  }

  void print(const BlockStmt& node, std::size_t depth) {
    indent(depth);
    output_ << "Block";
    position(node.location);
    for (const auto& statement : node.statements) {
      print(*statement, depth + 1);
    }
  }

  void print(const ReturnStmt& node, std::size_t depth) {
    indent(depth);
    output_ << "Return";
    position(node.location);
    if (node.value) {
      print(*node.value, depth + 1);
    }
  }

  std::ostringstream output_;
};

}  // namespace

std::string print_ast(const Program& program) {
  return AstPrinter().print(program);
}

}  // namespace minic
