#pragma once

#include "token.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace minic {

using Value = std::variant<std::monostate, std::int64_t, bool>;

struct RuntimeError : std::runtime_error {
  RuntimeError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

std::string format_value(const Value& value);

namespace detail {

std::int64_t parse_integer(std::string_view digits, bool negative, SourceLocation location);
Value runtime_unary(TokenKind op, const Value& operand, SourceLocation location);
Value runtime_binary(TokenKind op, const Value& left, const Value& right, SourceLocation location);

}  // namespace detail
}  // namespace minic
