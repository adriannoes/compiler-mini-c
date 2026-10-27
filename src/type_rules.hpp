#pragma once

#include "ast.hpp"

#include <stdexcept>
#include <string>

namespace minic {

enum class Type { Int, Bool, Void };

const char* type_name(Type type);

struct TypeError : std::runtime_error {
  TypeError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

namespace detail {

Type syntax_type(const TypeSyntax& syntax);
Type value_type(const TypeSyntax& syntax);
void require_type(Type actual, Type expected, SourceLocation location);
Type unary_type(TokenKind op, Type operand, SourceLocation location);
Type binary_type(TokenKind op, Type left, Type right, SourceLocation location);

}  // namespace detail
}  // namespace minic
