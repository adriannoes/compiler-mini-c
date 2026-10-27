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
  KwBool,
  KwVoid,
  KwIf,
  KwElse,
  KwWhile,
  KwReturn,
  KwTrue,
  KwFalse,
  Identifier,
  Integer,
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  LeftBracket,
  RightBracket,
  Semicolon,
  Comma,
  Assign,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  EqualEqual,
  BangEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Bang,
  AndAnd,
  OrOr,
  EndOfFile,
};

struct Token {
  TokenKind kind;
  std::string lexeme;
  SourceLocation location;
};

const char* token_kind_name(TokenKind kind);

}  // namespace minic
