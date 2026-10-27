#pragma once

#include "name_resolution.hpp"
#include "type_rules.hpp"

#include <optional>
#include <unordered_map>

namespace minic {

class TypeAnalysis {
 public:
  const NameResolution& resolution() const { return resolution_; }
  std::optional<Type> type_of(const Expr& expression) const;
  std::optional<std::size_t> array_length(const VarDecl& declaration) const;

 private:
  friend class TypeChecker;

  NameResolution resolution_;
  std::unordered_map<const Expr*, Type> types_;
  std::unordered_map<const VarDecl*, std::size_t> array_lengths_;
};

TypeAnalysis check_types(const Program& program);
TypeAnalysis check_types(Program&& program) = delete;
TypeAnalysis check_types(const Program&& program) = delete;

}  // namespace minic
