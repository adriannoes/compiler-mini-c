#include "lexer.hpp"

#include <iomanip>
#include <sstream>

namespace minic {
namespace {

bool is_digit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool is_identifier_start(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

bool is_whitespace(char ch) {
  return ch == ' ' || (ch >= '\t' && ch <= '\r');
}

std::string unexpected_character(char ch) {
  std::ostringstream message;
  const auto byte = static_cast<unsigned char>(ch);
  if (byte >= 0x20 && byte <= 0x7e) {
    message << "Unexpected character " << std::quoted(std::string(1, ch)) << '.';
  } else {
    message << "Unexpected byte 0x" << std::uppercase << std::hex
            << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte)
            << '.';
  }
  return message.str();
}

class Scanner {
 public:
  explicit Scanner(std::string_view source) : source_(source) {}

  std::vector<Token> scan() {
    std::vector<Token> tokens;
    for (;;) {
      skip_whitespace_and_comments();
      if (at_end()) {
        tokens.push_back({TokenKind::EndOfFile, "", location_});
        return tokens;
      }
      tokens.push_back(scan_token());
    }
  }

 private:
  bool at_end() const {
    return offset_ == source_.size();
  }

  char peek(std::size_t ahead = 0) const {
    return ahead < source_.size() - offset_ ? source_[offset_ + ahead] : '\0';
  }

  char advance() {
    const char ch = source_[offset_++];
    if (ch == '\r') {
      if (peek() == '\n') {
        ++offset_;
      }
      ++location_.line;
      location_.column = 1;
    } else if (ch == '\n') {
      ++location_.line;
      location_.column = 1;
    } else {
      ++location_.column;
    }
    return ch;
  }

  bool match(char expected) {
    if (peek() != expected) {
      return false;
    }
    advance();
    return true;
  }

  void skip_whitespace_and_comments() {
    while (!at_end()) {
      if (is_whitespace(peek())) {
        advance();
      } else if (peek() == '/' && peek(1) == '/') {
        while (!at_end() && peek() != '\n' && peek() != '\r') {
          advance();
        }
      } else if (peek() == '/' && peek(1) == '*') {
        const SourceLocation start = location_;
        advance();
        advance();
        while (!at_end() && !(peek() == '*' && peek(1) == '/')) {
          advance();
        }
        if (at_end()) {
          throw LexError(start, "Unterminated block comment.");
        }
        advance();
        advance();
      } else {
        return;
      }
    }
  }

  Token scan_token() {
    const std::size_t start = offset_;
    const SourceLocation location = location_;
    const char ch = advance();
    TokenKind kind;

    if (is_identifier_start(ch)) {
      while (is_identifier_start(peek()) || is_digit(peek())) {
        advance();
      }
      const std::string_view text = source_.substr(start, offset_ - start);
      if (text == "int") {
        kind = TokenKind::KwInt;
      } else if (text == "bool") {
        kind = TokenKind::KwBool;
      } else if (text == "void") {
        kind = TokenKind::KwVoid;
      } else if (text == "if") {
        kind = TokenKind::KwIf;
      } else if (text == "else") {
        kind = TokenKind::KwElse;
      } else if (text == "while") {
        kind = TokenKind::KwWhile;
      } else if (text == "return") {
        kind = TokenKind::KwReturn;
      } else if (text == "true") {
        kind = TokenKind::KwTrue;
      } else if (text == "false") {
        kind = TokenKind::KwFalse;
      } else {
        kind = TokenKind::Identifier;
      }
    } else if (is_digit(ch)) {
      while (is_digit(peek())) {
        advance();
      }
      kind = TokenKind::Integer;
    } else {
      switch (ch) {
        case '(': kind = TokenKind::LeftParen; break;
        case ')': kind = TokenKind::RightParen; break;
        case '{': kind = TokenKind::LeftBrace; break;
        case '}': kind = TokenKind::RightBrace; break;
        case ';': kind = TokenKind::Semicolon; break;
        case ',': kind = TokenKind::Comma; break;
        case '+': kind = TokenKind::Plus; break;
        case '-': kind = TokenKind::Minus; break;
        case '*': kind = TokenKind::Star; break;
        case '/': kind = TokenKind::Slash; break;
        case '%': kind = TokenKind::Percent; break;
        case '=': kind = match('=') ? TokenKind::EqualEqual : TokenKind::Assign; break;
        case '!': kind = match('=') ? TokenKind::BangEqual : TokenKind::Bang; break;
        case '<': kind = match('=') ? TokenKind::LessEqual : TokenKind::Less; break;
        case '>': kind = match('=') ? TokenKind::GreaterEqual : TokenKind::Greater; break;
        case '&':
          if (!match('&')) {
            throw LexError(location, unexpected_character(ch));
          }
          kind = TokenKind::AndAnd;
          break;
        case '|':
          if (!match('|')) {
            throw LexError(location, unexpected_character(ch));
          }
          kind = TokenKind::OrOr;
          break;
        default: throw LexError(location, unexpected_character(ch));
      }
    }

    return {kind, std::string(source_.substr(start, offset_ - start)), location};
  }

  std::string_view source_;
  std::size_t offset_ = 0;
  SourceLocation location_;
};

}  // namespace

LexError::LexError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location(location) {}

std::vector<Token> lex(std::string_view source) {
  return Scanner(source).scan();
}

}  // namespace minic
