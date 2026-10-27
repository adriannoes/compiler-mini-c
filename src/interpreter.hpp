#pragma once

#include "ast.hpp"
#include "runtime.hpp"

#include <cstddef>
#include <string>

namespace minic {

struct ExecutionLimits {
  std::size_t max_steps = 1'000'000;
  std::size_t max_call_depth = 128;
};

Value interpret(const Program& program, const std::string& entry = "main");
Value interpret(const Program& program, const std::string& entry, ExecutionLimits limits);

}  // namespace minic
