#pragma once

#include "token.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace minic {

using Value = std::variant<std::monostate, std::int64_t, bool>;

struct ExecutionLimits {
  std::size_t max_steps = 1'000'000;
  std::size_t max_call_depth = 128;
};

struct RuntimeError : std::runtime_error {
  RuntimeError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

std::string format_value(const Value& value);

namespace detail {

struct StorageSlot {
  std::optional<Value> scalar;
  std::vector<std::optional<Value>> elements;

  void reset(std::size_t length = 0);
  std::size_t check_index(const Value& index, const std::string& name, SourceLocation location) const;
  Value load_element(const Value& index, const std::string& name, SourceLocation location) const;
};

std::int64_t parse_integer(std::string_view digits, bool negative, SourceLocation location);
Value runtime_unary(TokenKind op, const Value& operand, SourceLocation location);
Value runtime_binary(TokenKind op, const Value& left, const Value& right, SourceLocation location);

}  // namespace detail
}  // namespace minic
