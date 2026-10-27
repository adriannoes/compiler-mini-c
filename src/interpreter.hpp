#pragma once

#include "ast.hpp"
#include "runtime.hpp"

#include <string>

namespace minic {

Value interpret(const Program& program, const std::string& entry = "main");

}  // namespace minic
