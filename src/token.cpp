#include "token.hpp"

#include <stdexcept>

namespace minic {

const char* token_kind_name(TokenKind kind) {
  switch (kind) {
    case TokenKind::KwInt: return "INT";
    case TokenKind::KwReturn: return "RETURN";
    case TokenKind::Identifier: return "IDENTIFIER";
    case TokenKind::Integer: return "INTEGER";
    case TokenKind::LeftParen: return "LEFT_PAREN";
    case TokenKind::RightParen: return "RIGHT_PAREN";
    case TokenKind::LeftBrace: return "LEFT_BRACE";
    case TokenKind::RightBrace: return "RIGHT_BRACE";
    case TokenKind::Semicolon: return "SEMICOLON";
    case TokenKind::EndOfFile: return "EOF";
  }
  throw std::logic_error("Unknown token kind.");
}

}  // namespace minic
