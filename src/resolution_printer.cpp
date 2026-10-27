#include "resolution_printer.hpp"

#include <iomanip>
#include <sstream>

namespace minic {
namespace {

const char* scope_kind_name(ScopeKind kind) {
  switch (kind) {
    case ScopeKind::Global: return "GLOBAL";
    case ScopeKind::Function: return "FUNCTION";
    case ScopeKind::Block: return "BLOCK";
  }
  throw std::logic_error("Unknown scope kind.");
}

const char* symbol_kind_name(SymbolKind kind) {
  switch (kind) {
    case SymbolKind::Function: return "FUNCTION";
    case SymbolKind::Parameter: return "PARAMETER";
    case SymbolKind::Variable: return "VARIABLE";
  }
  throw std::logic_error("Unknown symbol kind.");
}

const char* use_kind_name(UseKind kind) {
  switch (kind) {
    case UseKind::Read: return "Read";
    case UseKind::Write: return "Write";
    case UseKind::Call: return "Call";
  }
  throw std::logic_error("Unknown use kind.");
}

void position(std::ostream& output, SourceLocation location) {
  output << " @" << location.line << ':' << location.column;
}

}  // namespace

std::string print_resolution(const NameResolution& resolution) {
  std::ostringstream output;
  output << "Resolution\n";
  const auto& table = resolution.table();
  for (std::size_t i = 0; i < table.scopes().size(); ++i) {
    const auto& scope = table.scopes()[i];
    output << "  Scope #" << i << ' ' << scope_kind_name(scope.kind);
    if (scope.parent) {
      output << " parent=#" << scope.parent->index;
    }
    position(output, scope.location);
    output << '\n';
  }
  for (std::size_t i = 0; i < table.symbols().size(); ++i) {
    const auto& symbol = table.symbols()[i];
    output << "  Symbol #" << i << ' ' << symbol_kind_name(symbol.kind) << ' '
           << std::quoted(symbol.name) << " scope=#" << symbol.scope.index;
    position(output, symbol.location);
    output << '\n';
  }
  for (const auto& binding : resolution.bindings()) {
    const auto& symbol = table.symbol(binding.symbol);
    output << "  " << use_kind_name(binding.kind) << ' ' << std::quoted(symbol.name)
           << " scope=#" << binding.scope.index;
    position(output, binding.expression->location);
    output << " -> Symbol #" << binding.symbol.index;
    position(output, symbol.location);
    output << '\n';
  }
  return output.str();
}

}  // namespace minic
