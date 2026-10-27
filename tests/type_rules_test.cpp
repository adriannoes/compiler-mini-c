#include "type_rules.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

using minic::TokenKind;
using minic::Type;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Action>
void expect_error(Action action, const std::string& message) {
  try {
    action();
  } catch (const minic::TypeError& error) {
    require(error.location.line == 3 && error.location.column == 7, "Unexpected type error position.");
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a type error.");
}

void syntax() {
  require(minic::detail::syntax_type({TokenKind::KwInt, {3, 7}}) == Type::Int, "Expected type int.");
  require(minic::detail::syntax_type({TokenKind::KwBool, {3, 7}}) == Type::Bool, "Expected type bool.");
  require(minic::detail::syntax_type({TokenKind::KwVoid, {3, 7}}) == Type::Void, "Expected type void.");
  require(minic::detail::value_type({TokenKind::KwInt, {3, 7}}) == Type::Int, "Expected an int value type.");
  require(minic::detail::value_type({TokenKind::KwBool, {3, 7}}) == Type::Bool, "Expected a bool value type.");
  require(std::string(minic::type_name(Type::Int)) == "int" &&
              std::string(minic::type_name(Type::Bool)) == "bool" &&
              std::string(minic::type_name(Type::Void)) == "void", "Unexpected type names.");
  expect_error([] { minic::detail::value_type({TokenKind::KwVoid, {3, 7}}); },
               "Variables and parameters cannot have type void.");
}

void arithmetic() {
  for (const auto op : {TokenKind::Plus, TokenKind::Minus, TokenKind::Star, TokenKind::Slash, TokenKind::Percent}) {
    require(minic::detail::binary_type(op, Type::Int, Type::Int, {3, 7}) == Type::Int,
            "Expected an int arithmetic result.");
    for (const auto invalid : {Type::Bool, Type::Void}) {
      const auto message = std::string("Expected type int. Received ") + minic::type_name(invalid) + '.';
      expect_error([&] { minic::detail::binary_type(op, invalid, Type::Int, {3, 7}); }, message);
      expect_error([&] { minic::detail::binary_type(op, Type::Int, invalid, {3, 7}); }, message);
    }
  }
  require(minic::detail::unary_type(TokenKind::Minus, Type::Int, {3, 7}) == Type::Int,
          "Expected an int negation result.");
  expect_error([] { minic::detail::unary_type(TokenKind::Minus, Type::Bool, {3, 7}); },
               "Expected type int. Received bool.");
}

void comparisons() {
  for (const auto op : {TokenKind::Less, TokenKind::LessEqual, TokenKind::Greater, TokenKind::GreaterEqual}) {
    require(minic::detail::binary_type(op, Type::Int, Type::Int, {3, 7}) == Type::Bool,
            "Expected a bool comparison result.");
    expect_error([&] { minic::detail::binary_type(op, Type::Bool, Type::Int, {3, 7}); },
                 "Expected type int. Received bool.");
    expect_error([&] { minic::detail::binary_type(op, Type::Int, Type::Bool, {3, 7}); },
                 "Expected type int. Received bool.");
  }
}

void logic() {
  for (const auto op : {TokenKind::AndAnd, TokenKind::OrOr}) {
    require(minic::detail::binary_type(op, Type::Bool, Type::Bool, {3, 7}) == Type::Bool,
            "Expected a bool logical result.");
    for (const auto invalid : {Type::Int, Type::Void}) {
      const auto message = std::string("Expected type bool. Received ") + minic::type_name(invalid) + '.';
      expect_error([&] { minic::detail::binary_type(op, invalid, Type::Bool, {3, 7}); }, message);
      expect_error([&] { minic::detail::binary_type(op, Type::Bool, invalid, {3, 7}); }, message);
    }
  }
  require(minic::detail::unary_type(TokenKind::Bang, Type::Bool, {3, 7}) == Type::Bool,
          "Expected a bool logical negation result.");
  expect_error([] { minic::detail::unary_type(TokenKind::Bang, Type::Int, {3, 7}); },
               "Expected type bool. Received int.");
}

void equality() {
  for (const auto op : {TokenKind::EqualEqual, TokenKind::BangEqual}) {
    for (const auto type : {Type::Int, Type::Bool}) {
      require(minic::detail::binary_type(op, type, type, {3, 7}) == Type::Bool,
              "Expected equality between matching value types.");
    }
    expect_error([&] { minic::detail::binary_type(op, Type::Int, Type::Bool, {3, 7}); },
                 "Expected type int. Received bool.");
    expect_error([&] { minic::detail::binary_type(op, Type::Bool, Type::Int, {3, 7}); },
                 "Expected type bool. Received int.");
    expect_error([&] { minic::detail::binary_type(op, Type::Void, Type::Void, {3, 7}); },
                 "Equality requires int or bool operands.");
  }
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"syntax", syntax}, {"arithmetic", arithmetic}, {"comparisons", comparisons},
      {"logic", logic}, {"equality", equality},
  };
  if (argc == 2) {
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
  }
  std::cerr << "Expected one known test name.\n";
  return 1;
}
