#include "ast_printer.hpp"
#include "lexer.hpp"
#include "parser.hpp"

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
      << "\n"
      << "Print token types, source text, and line:column positions.\n"
      << "Use --ast to print the syntax tree.\n";
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
  if (argc != 3 || (!ast_mode && std::string(argv[1]) != "--tokens")) {
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
  }

  return 0;
}
