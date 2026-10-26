#include <iostream>
#include <string>

namespace {

void print_usage() {
  std::cout
      << "minic — Mini-C educational compiler\n"
      << "\n"
      << "Usage:\n"
      << "  minic --help\n"
      << "\n"
      << "Phases ahead: lexer → parser → semantics → interpreter → bytecode VM.\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc >= 2) {
    const std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
      print_usage();
      return 0;
    }
  }

  std::cerr << "minic: no command yet (phase 0 scaffold). Try --help.\n";
  return 1;
}
