#include "ast_printer.hpp"
#include "bytecode_compiler.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "resolution_printer.hpp"
#include "type_checker.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_usage() {
  std::cout
      << "minic: Mini-C educational compiler\n"
      << "\n"
      << "Usage:\n"
      << "  minic --help\n"
      << "  minic --tokens <file>\n"
      << "  minic --ast <file>\n"
      << "  minic --resolve <file>\n"
      << "  minic --check <file>\n"
      << "  minic --bytecode <file>\n"
      << "  minic --run <file>\n"
      << "\n"
      << "Print token types, source text, and line:column positions.\n"
      << "Use --ast to print the syntax tree.\n"
      << "Use --resolve to print scopes, symbols, and name bindings.\n"
      << "Use --check to analyze Mini-C types.\n"
      << "Use --bytecode to print Mini-C instructions.\n"
      << "Use --run to execute Mini-C programs.\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    const std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
      print_usage();
      return 0;
    }
  }

  const bool ast_mode = argc >= 2 && std::string(argv[1]) == "--ast";
  const bool resolve_mode = argc >= 2 && std::string(argv[1]) == "--resolve";
  const bool check_mode = argc >= 2 && std::string(argv[1]) == "--check";
  const bool run_mode = argc >= 2 && std::string(argv[1]) == "--run";
  const bool bytecode_mode = argc >= 2 && std::string(argv[1]) == "--bytecode";
  if (argc != 3 || (!ast_mode && !resolve_mode && !check_mode && !run_mode && !bytecode_mode &&
                    std::string(argv[1]) != "--tokens")) {
    if (bytecode_mode) {
      std::cerr << "minic: error: Expected --bytecode <file>. Use --help for usage.\n";
      return 1;
    }
    if (run_mode) {
      std::cerr << "minic: error: Expected --run <file>. Use --help for usage.\n";
      return 1;
    }
    if (check_mode) {
      std::cerr << "minic: error: Expected --check <file>. Use --help for usage.\n";
      return 1;
    }
    if (resolve_mode) {
      std::cerr << "minic: error: Expected --resolve <file>. Use --help for usage.\n";
      return 1;
    }
    if (ast_mode) {
      std::cerr << "minic: error: Expected --ast <file>. Use --help for usage.\n";
      return 1;
    }
    std::cerr << "minic: error: Expected --tokens <file>. Use --help for usage.\n";
    return 1;
  }

  const std::string path = argv[2];
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    std::cerr << path << ": error: Cannot open file.\n";
    return 1;
  }

  std::string source;
  char buffer[4096];
  while (input.read(buffer, sizeof buffer) || input.gcount() > 0) {
    source.append(buffer, static_cast<std::size_t>(input.gcount()));
  }
  if (!input.eof()) {
    std::cerr << path << ": error: Cannot read file.\n";
    return 1;
  }

  try {
    const auto tokens = minic::lex(source);
    if (ast_mode) {
      std::cout << minic::print_ast(minic::parse(tokens));
    } else if (resolve_mode) {
      const auto program = minic::parse(tokens);
      const auto resolution = minic::resolve_names(program);
      std::cout << minic::print_resolution(resolution);
    } else if (check_mode) {
      const auto program = minic::parse(tokens);
      minic::check_types(program);
    } else if (run_mode) {
      const auto program = minic::parse(tokens);
      const auto value = minic::interpret(program);
      const auto text = minic::format_value(value);
      if (!text.empty()) {
        std::cout << text << '\n';
      }
    } else if (bytecode_mode) {
      const auto program = minic::parse(tokens);
      std::cout << minic::print_bytecode(minic::compile_bytecode(program));
    } else {
      for (const auto& token : tokens) {
        std::cout << token.location.line << ':' << token.location.column << ' '
                  << minic::token_kind_name(token.kind) << ' '
                  << std::quoted(token.lexeme) << '\n';
      }
    }
  } catch (const minic::LexError& error) {
    std::cerr << path << ':' << error.location.line << ':'
              << error.location.column << ": error: " << error.what() << '\n';
    return 1;
  } catch (const minic::ParseError& error) {
    std::cerr << path << ':' << error.location.line << ':'
              << error.location.column << ": error: " << error.what() << '\n';
    return 1;
  } catch (const minic::ResolveError& error) {
    std::cerr << path << ':' << error.location.line << ':'
              << error.location.column << ": error: " << error.what() << '\n';
    return 1;
  } catch (const minic::TypeError& error) {
    std::cerr << path << ':' << error.location.line << ':'
              << error.location.column << ": error: " << error.what() << '\n';
    return 1;
  } catch (const minic::RuntimeError& error) {
    std::cerr << path << ':' << error.location.line << ':'
              << error.location.column << ": error: " << error.what() << '\n';
    return 1;
  }

  return 0;
}
