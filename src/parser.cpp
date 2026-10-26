#include "parser.hpp"

#include <initializer_list>
#include <utility>

namespace minic {
namespace {

template <typename Node>
ExprPtr make_expr(SourceLocation location, Node node) {
  return std::make_unique<Expr>(Expr{location, std::move(node)});
}

class Parser {
 public:
  explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

  Program program() {
    Program result;
    while (peek().kind != TokenKind::EndOfFile) {
      result.functions.push_back(function());
    }
    return result;
  }

 private:
  const Token& peek() const {
    return tokens_[index_];
  }

  const Token& previous() const {
    return tokens_[index_ - 1];
  }

  bool match(std::initializer_list<TokenKind> kinds) {
    for (const auto kind : kinds) {
      if (peek().kind == kind) {
        ++index_;
        return true;
      }
    }
    return false;
  }

  [[noreturn]] void fail(const std::string& message) const {
    throw ParseError(peek().location,
                     message + " Received " + token_kind_name(peek().kind) + '.');
  }

  const Token& expect(TokenKind kind, const std::string& message) {
    if (!match({kind})) {
      fail(message);
    }
    return previous();
  }

  TypeSyntax type() {
    if (!match({TokenKind::KwInt, TokenKind::KwBool, TokenKind::KwVoid})) {
      fail("Expected a type.");
    }
    return {previous().kind, previous().location};
  }

  FunctionDecl function() {
    const auto return_type = type();
    const auto& name = expect(TokenKind::Identifier, "Expected a function name.");
    expect(TokenKind::LeftParen, "Expected \"(\" after function name.");
    expect(TokenKind::RightParen, "Expected \")\" after parameters.");
    return {name.location, return_type, name.lexeme, block()};
  }

  BlockStmt block() {
    const auto location = expect(TokenKind::LeftBrace, "Expected \"{\" before block.").location;
    BlockStmt result{location, {}};
    while (peek().kind != TokenKind::RightBrace && peek().kind != TokenKind::EndOfFile) {
      result.statements.push_back(statement());
    }
    expect(TokenKind::RightBrace, "Expected \"}\" after block.");
    return result;
  }

  StmtPtr statement() {
    if (peek().kind == TokenKind::LeftBrace) {
      return std::make_unique<Stmt>(Stmt{block()});
    }
    if (match({TokenKind::KwReturn})) {
      const auto location = previous().location;
      ExprPtr value;
      if (peek().kind != TokenKind::Semicolon) {
        value = expression();
      }
      expect(TokenKind::Semicolon, "Expected \";\" after return value.");
      return std::make_unique<Stmt>(Stmt{ReturnStmt{location, std::move(value)}});
    }
    fail("Expected a return statement or a block.");
  }

  ExprPtr expression() {
    return assignment();
  }

  ExprPtr assignment() {
    auto target = logical_or();
    if (match({TokenKind::Assign})) {
      const auto location = previous().location;
      if (!std::holds_alternative<NameExpr>(target->node)) {
        throw ParseError(location, "Expected a name before \"=\".");
      }
      auto value = assignment();
      return make_expr(location, AssignmentExpr{std::move(target), std::move(value)});
    }
    return target;
  }

  ExprPtr binary(ExprPtr (Parser::*operand)(), std::initializer_list<TokenKind> operators) {
    auto left = (this->*operand)();
    while (match(operators)) {
      const auto& op = previous();
      auto right = (this->*operand)();
      left = make_expr(op.location, BinaryExpr{std::move(left), op.kind, std::move(right)});
    }
    return left;
  }

  ExprPtr logical_or() {
    return binary(&Parser::logical_and, {TokenKind::OrOr});
  }

  ExprPtr logical_and() {
    return binary(&Parser::equality, {TokenKind::AndAnd});
  }

  ExprPtr equality() {
    return binary(&Parser::comparison, {TokenKind::EqualEqual, TokenKind::BangEqual});
  }

  ExprPtr comparison() {
    return binary(&Parser::additive, {TokenKind::Less, TokenKind::LessEqual,
                                     TokenKind::Greater, TokenKind::GreaterEqual});
  }

  ExprPtr additive() {
    return binary(&Parser::multiplicative, {TokenKind::Plus, TokenKind::Minus});
  }

  ExprPtr multiplicative() {
    return binary(&Parser::unary, {TokenKind::Star, TokenKind::Slash, TokenKind::Percent});
  }

  ExprPtr unary() {
    if (match({TokenKind::Bang, TokenKind::Minus})) {
      const auto& op = previous();
      auto operand = unary();
      return make_expr(op.location, UnaryExpr{op.kind, std::move(operand)});
    }
    return primary();
  }

  ExprPtr primary() {
    if (match({TokenKind::Integer})) {
      return make_expr(previous().location, IntegerExpr{previous().lexeme});
    }
    if (match({TokenKind::KwTrue, TokenKind::KwFalse})) {
      return make_expr(previous().location, BooleanExpr{previous().kind == TokenKind::KwTrue});
    }
    if (match({TokenKind::Identifier})) {
      return make_expr(previous().location, NameExpr{previous().lexeme});
    }
    if (match({TokenKind::LeftParen})) {
      auto result = expression();
      expect(TokenKind::RightParen, "Expected \")\" after expression.");
      return result;
    }
    fail("Expected an expression.");
  }

  const std::vector<Token>& tokens_;
  std::size_t index_ = 0;
};

}  // namespace

ParseError::ParseError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location(location) {}

Program parse(const std::vector<Token>& tokens) {
  if (tokens.empty() || tokens.back().kind != TokenKind::EndOfFile) {
    throw std::invalid_argument("Expected tokens with a final EOF token.");
  }
  return Parser(tokens).program();
}

}  // namespace minic
