#include "type_rules.hpp"

namespace minic {

const char* type_name(Type type) {
  switch (type) {
    case Type::Int: return "int";
    case Type::Bool: return "bool";
    case Type::Void: return "void";
  }
  throw std::invalid_argument("Unknown Mini-C type.");
}

TypeError::TypeError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location(location) {}

namespace detail {

Type syntax_type(const TypeSyntax& syntax) {
  switch (syntax.keyword) {
    case TokenKind::KwInt: return Type::Int;
    case TokenKind::KwBool: return Type::Bool;
    case TokenKind::KwVoid: return Type::Void;
    default: throw std::invalid_argument("Expected a type keyword.");
  }
}

Type value_type(const TypeSyntax& syntax) {
  const auto type = syntax_type(syntax);
  if (type == Type::Void) {
    throw TypeError(syntax.location, "Variables and parameters cannot have type void.");
  }
  return type;
}

void require_type(Type actual, Type expected, SourceLocation location) {
  if (actual != expected) {
    throw TypeError(location, std::string("Expected type ") + type_name(expected) +
                                 ". Received " + type_name(actual) + '.');
  }
}

Type unary_type(TokenKind op, Type operand, SourceLocation location) {
  switch (op) {
    case TokenKind::Minus:
      require_type(operand, Type::Int, location);
      return Type::Int;
    case TokenKind::Bang:
      require_type(operand, Type::Bool, location);
      return Type::Bool;
    default: throw std::invalid_argument("Expected a unary operator.");
  }
}

Type binary_type(TokenKind op, Type left, Type right, SourceLocation location) {
  switch (op) {
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Star:
    case TokenKind::Slash:
    case TokenKind::Percent:
      require_type(left, Type::Int, location);
      require_type(right, Type::Int, location);
      return Type::Int;
    case TokenKind::Less:
    case TokenKind::LessEqual:
    case TokenKind::Greater:
    case TokenKind::GreaterEqual:
      require_type(left, Type::Int, location);
      require_type(right, Type::Int, location);
      return Type::Bool;
    case TokenKind::AndAnd:
    case TokenKind::OrOr:
      require_type(left, Type::Bool, location);
      require_type(right, Type::Bool, location);
      return Type::Bool;
    case TokenKind::EqualEqual:
    case TokenKind::BangEqual:
      if (left == Type::Void || right == Type::Void) {
        throw TypeError(location, "Equality requires int or bool operands.");
      }
      require_type(right, left, location);
      return Type::Bool;
    default: throw std::invalid_argument("Expected a binary operator.");
  }
}

}  // namespace detail
}  // namespace minic
