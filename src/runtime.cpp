#include "runtime.hpp"
#include "type_rules.hpp"

#include <limits>

namespace minic {

RuntimeError::RuntimeError(SourceLocation location, const std::string& message)
    : std::runtime_error(message), location(location) {}

std::string format_value(const Value& value) {
  if (const auto* integer = std::get_if<std::int64_t>(&value)) {
    return std::to_string(*integer);
  }
  if (const auto* boolean = std::get_if<bool>(&value)) {
    return *boolean ? "true" : "false";
  }
  return "";
}

namespace detail {

void StorageSlot::reset(std::size_t length) {
  if (length > max_array_length) {
    throw std::invalid_argument("The array length is outside the supported range.");
  }
  scalar.reset();
  elements.assign(length, std::nullopt);
}

std::size_t StorageSlot::check_index(const Value& index, const std::string& name, SourceLocation location) const {
  const auto* integer = std::get_if<std::int64_t>(&index);
  if (!integer) {
    throw std::invalid_argument("The array index must have type int.");
  }
  if (*integer < 0 || static_cast<std::uint64_t>(*integer) >= elements.size()) {
    throw RuntimeError(location, "The array index " + std::to_string(*integer) + " is outside \"" + name +
                                    "\" with length " + std::to_string(elements.size()) + '.');
  }
  return static_cast<std::size_t>(*integer);
}

Value StorageSlot::load_element(const Value& index, const std::string& name, SourceLocation location) const {
  const auto offset = check_index(index, name, location);
  const auto& value = elements[offset];
  if (!value) {
    throw RuntimeError(location, "The array element \"" + name + '[' + std::to_string(offset) + "]\" has no value.");
  }
  return *value;
}

namespace {

constexpr auto minimum = std::numeric_limits<std::int64_t>::min();
constexpr auto maximum = std::numeric_limits<std::int64_t>::max();

[[noreturn]] void overflow(SourceLocation location) {
  throw RuntimeError(location, "Integer arithmetic exceeds the signed 64-bit range.");
}

std::int64_t add(std::int64_t left, std::int64_t right, SourceLocation location) {
  if ((right > 0 && left > maximum - right) || (right < 0 && left < minimum - right)) {
    overflow(location);
  }
  return left + right;
}

std::int64_t subtract(std::int64_t left, std::int64_t right, SourceLocation location) {
  if ((right > 0 && left < minimum + right) || (right < 0 && left > maximum + right)) {
    overflow(location);
  }
  return left - right;
}

std::int64_t multiply(std::int64_t left, std::int64_t right, SourceLocation location) {
  if (left > 0) {
    if ((right > 0 && left > maximum / right) || (right < 0 && right < minimum / left)) {
      overflow(location);
    }
  } else if (left < 0) {
    if ((right > 0 && left < minimum / right) || (right < 0 && left < maximum / right)) {
      overflow(location);
    }
  }
  return left * right;
}

void divisor(std::int64_t left, std::int64_t right, SourceLocation location) {
  if (right == 0) {
    throw RuntimeError(location, "The divisor is zero.");
  }
  if (left == minimum && right == -1) {
    overflow(location);
  }
}

}  // namespace

std::int64_t parse_integer(std::string_view digits, bool negative, SourceLocation location) {
  if (digits.empty()) {
    throw std::invalid_argument("The integer text must contain decimal digits.");
  }
  const auto limit = static_cast<std::uint64_t>(maximum) + (negative ? 1U : 0U);
  std::uint64_t magnitude = 0;
  for (const char character : digits) {
    if (character < '0' || character > '9') {
      throw std::invalid_argument("The integer text must contain decimal digits.");
    }
    const auto digit = static_cast<std::uint64_t>(character - '0');
    if (magnitude > (limit - digit) / 10) {
      throw RuntimeError(location, "The integer literal is outside the signed 64-bit range.");
    }
    magnitude = magnitude * 10 + digit;
  }
  if (!negative) {
    return static_cast<std::int64_t>(magnitude);
  }
  if (magnitude == static_cast<std::uint64_t>(maximum) + 1U) {
    return minimum;
  }
  return -static_cast<std::int64_t>(magnitude);
}

Value runtime_unary(TokenKind op, const Value& operand, SourceLocation location) {
  if (op == TokenKind::Bang) {
    return !std::get<bool>(operand);
  }
  if (op != TokenKind::Minus) {
    throw std::invalid_argument("The token is not a unary operator.");
  }
  const auto value = std::get<std::int64_t>(operand);
  if (value == minimum) {
    overflow(location);
  }
  return -value;
}

Value runtime_binary(TokenKind op, const Value& left, const Value& right, SourceLocation location) {
  switch (op) {
    case TokenKind::EqualEqual: return left == right;
    case TokenKind::BangEqual: return left != right;
    case TokenKind::AndAnd: return std::get<bool>(left) && std::get<bool>(right);
    case TokenKind::OrOr: return std::get<bool>(left) || std::get<bool>(right);
    default: break;
  }
  const auto first = std::get<std::int64_t>(left);
  const auto second = std::get<std::int64_t>(right);
  switch (op) {
    case TokenKind::Plus: return add(first, second, location);
    case TokenKind::Minus: return subtract(first, second, location);
    case TokenKind::Star: return multiply(first, second, location);
    case TokenKind::Slash:
      divisor(first, second, location);
      return first / second;
    case TokenKind::Percent:
      divisor(first, second, location);
      return first % second;
    case TokenKind::Less: return first < second;
    case TokenKind::LessEqual: return first <= second;
    case TokenKind::Greater: return first > second;
    case TokenKind::GreaterEqual: return first >= second;
    default: throw std::invalid_argument("The token is not a binary operator.");
  }
}

}  // namespace detail
}  // namespace minic
