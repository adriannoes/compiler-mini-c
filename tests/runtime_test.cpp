#include "runtime.hpp"

#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

using minic::TokenKind;
using minic::Value;
constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Action>
void expect_error(Action action, const std::string& message) {
  try {
    action();
  } catch (const minic::RuntimeError& error) {
    require(error.location.line == 2 && error.location.column == 5, "Unexpected runtime error position.");
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a runtime error.");
}

Value binary(TokenKind op, std::int64_t left, std::int64_t right) {
  return minic::detail::runtime_binary(op, Value{left}, Value{right}, {2, 5});
}

void values() {
  require(minic::format_value(Value{}) == "", "Expected no text for void.");
  require(minic::format_value(Value{true}) == "true" && minic::format_value(Value{false}) == "false",
          "Expected boolean text.");
  require(minic::format_value(Value{minimum}) == "-9223372036854775808" &&
              minic::format_value(Value{maximum}) == "9223372036854775807", "Unexpected integer text.");
  require(Value{std::int64_t{1}} != Value{true}, "Expected distinct integer and boolean values.");
}

void literals() {
  require(minic::detail::parse_integer("0007", false, {2, 5}) == 7, "Expected decimal leading zeroes.");
  require(minic::detail::parse_integer("0", true, {2, 5}) == 0, "Expected negative zero to equal zero.");
  require(minic::detail::parse_integer("9223372036854775807", false, {2, 5}) == maximum,
          "Expected the largest positive integer.");
  require(minic::detail::parse_integer("0009223372036854775808", true, {2, 5}) == minimum,
          "Expected the smallest negative integer.");
  for (const auto text : {"9223372036854775808", "999999999999999999999999999999"}) {
    expect_error([&] { minic::detail::parse_integer(text, false, {2, 5}); },
                 "The integer literal is outside the signed 64-bit range.");
  }
  expect_error([] { minic::detail::parse_integer("9223372036854775809", true, {2, 5}); },
               "The integer literal is outside the signed 64-bit range.");
}

void arithmetic() {
  require(std::get<std::int64_t>(binary(TokenKind::Plus, 2, 3)) == 5, "Unexpected addition.");
  require(std::get<std::int64_t>(binary(TokenKind::Minus, 2, 3)) == -1, "Unexpected subtraction.");
  for (const auto left : {std::int64_t{-3}, std::int64_t{0}, std::int64_t{3}}) {
    for (const auto right : {std::int64_t{-2}, std::int64_t{0}, std::int64_t{2}}) {
      require(std::get<std::int64_t>(binary(TokenKind::Star, left, right)) == left * right,
              "Unexpected signed multiplication.");
    }
  }
  require(std::get<std::int64_t>(binary(TokenKind::Plus, minimum, maximum)) == -1,
          "Expected addition across the signed range.");
  require(std::get<std::int64_t>(binary(TokenKind::Star, minimum, 1)) == minimum &&
              std::get<std::int64_t>(binary(TokenKind::Star, maximum, 1)) == maximum,
          "Expected multiplication at the signed boundaries.");
  require(std::get<std::int64_t>(minic::detail::runtime_unary(TokenKind::Minus, Value{maximum}, {2, 5})) == -maximum,
          "Unexpected numeric negation.");
}

void overflow() {
  const struct { TokenKind op; std::int64_t left; std::int64_t right; } cases[] = {
      {TokenKind::Plus, maximum, 1}, {TokenKind::Plus, minimum, -1},
      {TokenKind::Minus, minimum, 1}, {TokenKind::Minus, maximum, -1},
      {TokenKind::Star, maximum, 2}, {TokenKind::Star, minimum, 2},
      {TokenKind::Star, 2, minimum}, {TokenKind::Star, minimum, -1},
      {TokenKind::Star, -1, minimum},
  };
  for (const auto& test : cases) {
    expect_error([&] { binary(test.op, test.left, test.right); },
                 "Integer arithmetic exceeds the signed 64-bit range.");
  }
  expect_error([] { minic::detail::runtime_unary(TokenKind::Minus, Value{minimum}, {2, 5}); },
               "Integer arithmetic exceeds the signed 64-bit range.");
}

void division() {
  require(std::get<std::int64_t>(binary(TokenKind::Slash, -7, 3)) == -2 &&
              std::get<std::int64_t>(binary(TokenKind::Slash, 7, -3)) == -2,
          "Expected division to truncate toward zero.");
  require(std::get<std::int64_t>(binary(TokenKind::Percent, -7, 3)) == -1 &&
              std::get<std::int64_t>(binary(TokenKind::Percent, 7, -3)) == 1,
          "Expected the remainder to have the dividend sign.");
  for (const auto op : {TokenKind::Slash, TokenKind::Percent}) {
    expect_error([&] { binary(op, 1, 0); }, "The divisor is zero.");
    expect_error([&] { binary(op, minimum, -1); }, "Integer arithmetic exceeds the signed 64-bit range.");
  }
}

void logic() {
  require(std::get<bool>(minic::detail::runtime_unary(TokenKind::Bang, Value{false}, {2, 5})),
          "Expected logical negation.");
  require(std::get<bool>(binary(TokenKind::Less, 1, 2)) && std::get<bool>(binary(TokenKind::LessEqual, 2, 2)) &&
              std::get<bool>(binary(TokenKind::Greater, 3, 2)) && std::get<bool>(binary(TokenKind::GreaterEqual, 2, 2)),
          "Unexpected integer comparisons.");
  require(std::get<bool>(binary(TokenKind::EqualEqual, 2, 2)) && std::get<bool>(binary(TokenKind::BangEqual, 1, 2)),
          "Unexpected integer equality.");
  require(!std::get<bool>(minic::detail::runtime_binary(TokenKind::AndAnd, Value{true}, Value{false}, {2, 5})) &&
              std::get<bool>(minic::detail::runtime_binary(TokenKind::OrOr, Value{false}, Value{true}, {2, 5})),
          "Unexpected boolean operations.");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"values", values}, {"literals", literals}, {"arithmetic", arithmetic},
      {"overflow", overflow}, {"division", division}, {"logic", logic},
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
