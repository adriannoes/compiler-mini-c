#pragma once

#include "token.hpp"

#include <stdexcept>
#include <string_view>
#include <vector>

namespace minic {

struct LexError : std::runtime_error {
  LexError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

std::vector<Token> lex(std::string_view source);

}  // namespace minic
