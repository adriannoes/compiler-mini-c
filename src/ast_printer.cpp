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
      print(function.return_type, 2);
      for (const auto& parameter : function.parameters) {
        indent(2);
        output_ << "Parameter " << std::quoted(parameter.name);
        position(parameter.location);
        print(parameter.type, 3);
      }
      print(function.body, 2);
    }
    return output_.str();
  }

 private:
  void print(const TypeSyntax& type, std::size_t depth) {
    indent(depth);
    output_ << "Type " << token_kind_name(type.keyword);
    position(type.location);
  }

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

  void print(const CallExpr& node, SourceLocation location, std::size_t depth) {
    indent(depth);
    output_ << "Call " << std::quoted(node.name);
    position(location);
    for (const auto& argument : node.arguments) {
      print(*argument, depth + 1);
    }
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

  void print(const VarDecl& node, std::size_t depth) {
    indent(depth);
    output_ << "Variable " << std::quoted(node.name);
    position(node.location);
    print(node.type, depth + 1);
    if (node.initializer) {
      print(*node.initializer, depth + 1);
    }
  }

  void print(const ExprStmt& node, std::size_t depth) {
    indent(depth);
    output_ << "Expression";
    position(node.location);
    print(*node.expression, depth + 1);
  }

  void print(const EmptyStmt& node, std::size_t depth) {
    indent(depth);
    output_ << "Empty";
    position(node.location);
  }

  std::ostringstream output_;
};

}  // namespace

std::string print_ast(const Program& program) {
  return AstPrinter().print(program);
}

}  // namespace minic
