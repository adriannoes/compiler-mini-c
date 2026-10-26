#pragma once

#include <cstddef>
#include <string>

namespace minic {

struct SourceLocation {
  std::size_t line = 1;
  std::size_t column = 1;
};

enum class TokenKind {
  KwInt,
  KwReturn,
  Identifier,
  Integer,
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  Semicolon,
  EndOfFile,
};

struct Token {
  TokenKind kind;
  std::string lexeme;
  SourceLocation location;
};

const char* token_kind_name(TokenKind kind);

}  // namespace minic
