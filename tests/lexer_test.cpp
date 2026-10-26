#include "lexer.hpp"

#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using minic::SourceLocation;
using minic::Token;
using minic::TokenKind;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string describe(const Token& token) {
  std::ostringstream output;
  output << token.location.line << ':' << token.location.column << ' '
         << minic::token_kind_name(token.kind) << ' ' << std::quoted(token.lexeme);
  return output.str();
}

void expect_tokens(std::string_view source, std::initializer_list<Token> expected) {
  const auto actual = minic::lex(source);
  require(actual.size() == expected.size(),
          "Expected " + std::to_string(expected.size()) + " tokens, received " +
              std::to_string(actual.size()) + '.');

  std::size_t index = 0;
  for (const auto& token : expected) {
    const auto& result = actual[index];
    require(result.kind == token.kind && result.lexeme == token.lexeme &&
                result.location.line == token.location.line &&
                result.location.column == token.location.column,
            "Token " + std::to_string(index) + ": expected " + describe(token) +
                ", received " + describe(result) + '.');
    ++index;
  }
}

void expect_error(std::string_view source, SourceLocation location,
                  const std::string& message) {
  try {
    minic::lex(source);
  } catch (const minic::LexError& error) {
    require(error.location.line == location.line &&
                error.location.column == location.column,
            "The diagnostic has the wrong source location.");
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a lexical error.");
}

void hello() {
  expect_tokens("int main() { return 0; }", {
      {TokenKind::KwInt, "int", {1, 1}},
      {TokenKind::Identifier, "main", {1, 5}},
      {TokenKind::LeftParen, "(", {1, 9}},
      {TokenKind::RightParen, ")", {1, 10}},
      {TokenKind::LeftBrace, "{", {1, 12}},
      {TokenKind::KwReturn, "return", {1, 14}},
      {TokenKind::Integer, "0", {1, 21}},
      {TokenKind::Semicolon, ";", {1, 22}},
      {TokenKind::RightBrace, "}", {1, 24}},
      {TokenKind::EndOfFile, "", {1, 25}},
  });
}

void empty() {
  expect_tokens("", {{TokenKind::EndOfFile, "", {1, 1}}});
  expect_tokens(" \t\n", {{TokenKind::EndOfFile, "", {2, 1}}});
}

void keywords() {
  expect_tokens("bool void if else while true false", {
      {TokenKind::KwBool, "bool", {1, 1}},
      {TokenKind::KwVoid, "void", {1, 6}},
      {TokenKind::KwIf, "if", {1, 11}},
      {TokenKind::KwElse, "else", {1, 14}},
      {TokenKind::KwWhile, "while", {1, 19}},
      {TokenKind::KwTrue, "true", {1, 25}},
      {TokenKind::KwFalse, "false", {1, 30}},
      {TokenKind::EndOfFile, "", {1, 35}},
  });
}

void identifiers() {
  const struct {
    std::string_view text;
    TokenKind kind;
  } cases[] = {
      {"int", TokenKind::KwInt},
      {"return", TokenKind::KwReturn},
      {"integer", TokenKind::Identifier},
      {"int2", TokenKind::Identifier},
      {"_return", TokenKind::Identifier},
      {"return_value", TokenKind::Identifier},
      {"Int", TokenKind::Identifier},
      {"Return", TokenKind::Identifier},
      {"boolean", TokenKind::Identifier},
      {"bool2", TokenKind::Identifier},
      {"_bool", TokenKind::Identifier},
      {"Bool", TokenKind::Identifier},
      {"voided", TokenKind::Identifier},
      {"if2", TokenKind::Identifier},
      {"elseif", TokenKind::Identifier},
      {"while_loop", TokenKind::Identifier},
      {"true_value", TokenKind::Identifier},
      {"falsehood", TokenKind::Identifier},
      {"TRUE", TokenKind::Identifier},
  };

  for (const auto& test : cases) {
    expect_tokens(test.text, {
        {test.kind, std::string(test.text), {1, 1}},
        {TokenKind::EndOfFile, "", {1, test.text.size() + 1}},
    });
  }
}

void integers() {
  for (const std::string_view text : {"0", "007", "123456789012345678901234567890"}) {
    expect_tokens(text, {
        {TokenKind::Integer, std::string(text), {1, 1}},
        {TokenKind::EndOfFile, "", {1, text.size() + 1}},
    });
  }
}

void operators() {
  const struct {
    std::string_view text;
    TokenKind kind;
  } cases[] = {
      {"=", TokenKind::Assign},
      {"+", TokenKind::Plus},
      {"-", TokenKind::Minus},
      {"*", TokenKind::Star},
      {"/", TokenKind::Slash},
      {"%", TokenKind::Percent},
      {"==", TokenKind::EqualEqual},
      {"!=", TokenKind::BangEqual},
      {"<", TokenKind::Less},
      {"<=", TokenKind::LessEqual},
      {">", TokenKind::Greater},
      {">=", TokenKind::GreaterEqual},
      {"!", TokenKind::Bang},
      {"&&", TokenKind::AndAnd},
      {"||", TokenKind::OrOr},
      {",", TokenKind::Comma},
  };

  for (const auto& test : cases) {
    expect_tokens(test.text, {
        {test.kind, std::string(test.text), {1, 1}},
        {TokenKind::EndOfFile, "", {1, test.text.size() + 1}},
    });
  }
}

void operator_boundaries() {
  expect_tokens("=== !== <== >== &&! ||!", {
      {TokenKind::EqualEqual, "==", {1, 1}},
      {TokenKind::Assign, "=", {1, 3}},
      {TokenKind::BangEqual, "!=", {1, 5}},
      {TokenKind::Assign, "=", {1, 7}},
      {TokenKind::LessEqual, "<=", {1, 9}},
      {TokenKind::Assign, "=", {1, 11}},
      {TokenKind::GreaterEqual, ">=", {1, 13}},
      {TokenKind::Assign, "=", {1, 15}},
      {TokenKind::AndAnd, "&&", {1, 17}},
      {TokenKind::Bang, "!", {1, 19}},
      {TokenKind::OrOr, "||", {1, 21}},
      {TokenKind::Bang, "!", {1, 23}},
      {TokenKind::EndOfFile, "", {1, 24}},
  });
  expect_tokens("-42,a-b+2*3%4", {
      {TokenKind::Minus, "-", {1, 1}},
      {TokenKind::Integer, "42", {1, 2}},
      {TokenKind::Comma, ",", {1, 4}},
      {TokenKind::Identifier, "a", {1, 5}},
      {TokenKind::Minus, "-", {1, 6}},
      {TokenKind::Identifier, "b", {1, 7}},
      {TokenKind::Plus, "+", {1, 8}},
      {TokenKind::Integer, "2", {1, 9}},
      {TokenKind::Star, "*", {1, 10}},
      {TokenKind::Integer, "3", {1, 11}},
      {TokenKind::Percent, "%", {1, 12}},
      {TokenKind::Integer, "4", {1, 13}},
      {TokenKind::EndOfFile, "", {1, 14}},
  });
  expect_tokens("++--+=/=", {
      {TokenKind::Plus, "+", {1, 1}},
      {TokenKind::Plus, "+", {1, 2}},
      {TokenKind::Minus, "-", {1, 3}},
      {TokenKind::Minus, "-", {1, 4}},
      {TokenKind::Plus, "+", {1, 5}},
      {TokenKind::Assign, "=", {1, 6}},
      {TokenKind::Slash, "/", {1, 7}},
      {TokenKind::Assign, "=", {1, 8}},
      {TokenKind::EndOfFile, "", {1, 9}},
  });
}

void division_and_comments() {
  expect_tokens("8/2// line\r\n/ /* block\rcomment */ /", {
      {TokenKind::Integer, "8", {1, 1}},
      {TokenKind::Slash, "/", {1, 2}},
      {TokenKind::Integer, "2", {1, 3}},
      {TokenKind::Slash, "/", {2, 1}},
      {TokenKind::Slash, "/", {3, 12}},
      {TokenKind::EndOfFile, "", {3, 13}},
  });
  expect_tokens("8/**//2", {
      {TokenKind::Integer, "8", {1, 1}},
      {TokenKind::Slash, "/", {1, 6}},
      {TokenKind::Integer, "2", {1, 7}},
      {TokenKind::EndOfFile, "", {1, 8}},
  });
  expect_tokens("/* /* */ /", {
      {TokenKind::Slash, "/", {1, 10}},
      {TokenKind::EndOfFile, "", {1, 11}},
  });
  expect_tokens("=/**/=", {
      {TokenKind::Assign, "=", {1, 1}},
      {TokenKind::Assign, "=", {1, 6}},
      {TokenKind::EndOfFile, "", {1, 7}},
  });
}

void positions_and_comments() {
  expect_tokens("// first\r\n\tint/* block\ncomment */\r\n  main // last", {
      {TokenKind::KwInt, "int", {2, 2}},
      {TokenKind::Identifier, "main", {4, 3}},
      {TokenKind::EndOfFile, "", {4, 15}},
  });
  expect_tokens("int/* note */return", {
      {TokenKind::KwInt, "int", {1, 1}},
      {TokenKind::KwReturn, "return", {1, 14}},
      {TokenKind::EndOfFile, "", {1, 20}},
  });
  expect_tokens("int\rreturn", {
      {TokenKind::KwInt, "int", {1, 1}},
      {TokenKind::KwReturn, "return", {2, 1}},
      {TokenKind::EndOfFile, "", {2, 7}},
  });
}

void invalid_character() {
  expect_error("int @", {1, 5}, "Unexpected character \"@\".");
  expect_error("&", {1, 1}, "Unexpected character \"&\".");
  expect_error("|", {1, 1}, "Unexpected character \"|\".");
  expect_error("true && false\n  &", {2, 3}, "Unexpected character \"&\".");
  expect_error("true || false | true", {1, 15}, "Unexpected character \"|\".");
  expect_error(std::string_view("int\0return", 10), {1, 4}, "Unexpected byte 0x00.");
}

void unterminated_comment() {
  expect_error("/*", {1, 1}, "Unterminated block comment.");
  expect_error("/*/", {1, 1}, "Unterminated block comment.");
  expect_error("int\n  /* missing", {2, 3}, "Unterminated block comment.");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test name.\n";
    return 1;
  }

  const struct {
    const char* name;
    void (*run)();
  } cases[] = {
      {"hello", hello},
      {"empty", empty},
      {"keywords", keywords},
      {"identifiers", identifiers},
      {"integers", integers},
      {"operators", operators},
      {"operator_boundaries", operator_boundaries},
      {"division_and_comments", division_and_comments},
      {"positions_and_comments", positions_and_comments},
      {"invalid_character", invalid_character},
      {"unterminated_comment", unterminated_comment},
  };

  for (const auto& test : cases) {
    if (std::string_view(argv[1]) == test.name) {
      try {
        test.run();
        std::cout << "PASS " << test.name << '\n';
        return 0;
      } catch (const std::exception& error) {
        std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
        return 1;
      }
    }
  }

  std::cerr << "Unknown test: " << argv[1] << '\n';
  return 1;
}
