#include "token.hpp"

#include <stdexcept>

namespace minic {

const char* token_kind_name(TokenKind kind) {
  switch (kind) {
    case TokenKind::KwInt: return "INT";
    case TokenKind::KwBool: return "BOOL";
    case TokenKind::KwVoid: return "VOID";
    case TokenKind::KwIf: return "IF";
    case TokenKind::KwElse: return "ELSE";
    case TokenKind::KwWhile: return "WHILE";
    case TokenKind::KwReturn: return "RETURN";
    case TokenKind::KwTrue: return "TRUE";
    case TokenKind::KwFalse: return "FALSE";
    case TokenKind::Identifier: return "IDENTIFIER";
    case TokenKind::Integer: return "INTEGER";
    case TokenKind::LeftParen: return "LEFT_PAREN";
    case TokenKind::RightParen: return "RIGHT_PAREN";
    case TokenKind::LeftBrace: return "LEFT_BRACE";
    case TokenKind::RightBrace: return "RIGHT_BRACE";
    case TokenKind::Semicolon: return "SEMICOLON";
    case TokenKind::Comma: return "COMMA";
    case TokenKind::Assign: return "ASSIGN";
    case TokenKind::Plus: return "PLUS";
    case TokenKind::Minus: return "MINUS";
    case TokenKind::Star: return "STAR";
    case TokenKind::Slash: return "SLASH";
    case TokenKind::Percent: return "PERCENT";
    case TokenKind::EqualEqual: return "EQUAL_EQUAL";
    case TokenKind::BangEqual: return "BANG_EQUAL";
    case TokenKind::Less: return "LESS";
    case TokenKind::LessEqual: return "LESS_EQUAL";
    case TokenKind::Greater: return "GREATER";
    case TokenKind::GreaterEqual: return "GREATER_EQUAL";
    case TokenKind::Bang: return "BANG";
    case TokenKind::AndAnd: return "AND_AND";
    case TokenKind::OrOr: return "OR_OR";
    case TokenKind::EndOfFile: return "EOF";
  }
  throw std::logic_error("Unknown token kind.");
}

}  // namespace minic
