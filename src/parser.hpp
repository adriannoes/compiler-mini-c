#pragma once

#include "ast.hpp"

#include <stdexcept>
#include <vector>

namespace minic {

struct ParseError : std::runtime_error {
  ParseError(SourceLocation location, const std::string& message);

  SourceLocation location;
};

Program parse(const std::vector<Token>& tokens);

}  // namespace minic
