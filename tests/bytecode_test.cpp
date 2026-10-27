#include "bytecode.hpp"

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void empty() {
  require(minic::print_bytecode({}) == "bytecode\n", "Unexpected empty bytecode output.");
}

void format() {
  const auto program = [] {
    minic::BytecodeFunction function{"answer", minic::Type::Int, 1, {2, 5}, {}, {}, {}};
    function.locals.push_back({"n", minic::Type::Int, {2, 16}});
    function.constants = {minic::IntegerConstant{"0009223372036854775808", true}, true, false,
                          minic::IntegerConstant{"999999999999999999999999999999", false}};
    function.code = {{minic::OpCode::Step, 0, {2, 19}}, {minic::OpCode::LoadConstant, 0, {3, 10}},
                     {minic::OpCode::Return, 0, {3, 3}}};
    return minic::BytecodeProgram{{function}};
  }();
  const std::string expected =
      "bytecode\n"
      "function 0 \"answer\" int parameters=1 @2:5\n"
      "  local 0 \"n\" int @2:16\n"
      "  constant 0 int \"-0009223372036854775808\"\n"
      "  constant 1 bool true\n"
      "  constant 2 bool false\n"
      "  constant 3 int \"999999999999999999999999999999\"\n"
      "  0 STEP @2:19\n"
      "  1 LOAD_CONSTANT 0 @3:10\n"
      "  2 RETURN @3:3\n"
      "end\n";
  require(minic::print_bytecode(program) == expected, "Unexpected bytecode format.");
  const auto copied = program;
  require(minic::print_bytecode(copied) == expected, "Expected independent, owned bytecode data.");
}

void instructions() {
  const struct { minic::OpCode opcode; const char* text; } cases[] = {
      {minic::OpCode::Step, "STEP"}, {minic::OpCode::LoadConstant, "LOAD_CONSTANT 7"},
      {minic::OpCode::LoadLocal, "LOAD_LOCAL 7"}, {minic::OpCode::StoreLocal, "STORE_LOCAL 7"},
      {minic::OpCode::ResetLocal, "RESET_LOCAL 7"}, {minic::OpCode::Pop, "POP"},
      {minic::OpCode::Negate, "NEGATE"}, {minic::OpCode::Not, "NOT"},
      {minic::OpCode::Add, "ADD"}, {minic::OpCode::Subtract, "SUBTRACT"},
      {minic::OpCode::Multiply, "MULTIPLY"}, {minic::OpCode::Divide, "DIVIDE"},
      {minic::OpCode::Remainder, "REMAINDER"}, {minic::OpCode::Equal, "EQUAL"},
      {minic::OpCode::NotEqual, "NOT_EQUAL"}, {minic::OpCode::Less, "LESS"},
      {minic::OpCode::LessEqual, "LESS_EQUAL"}, {minic::OpCode::Greater, "GREATER"},
      {minic::OpCode::GreaterEqual, "GREATER_EQUAL"}, {minic::OpCode::Jump, "JUMP 7"},
      {minic::OpCode::JumpIfFalse, "JUMP_IF_FALSE 7"}, {minic::OpCode::JumpIfTrue, "JUMP_IF_TRUE 7"},
      {minic::OpCode::Call, "CALL 7"}, {minic::OpCode::Return, "RETURN"},
      {minic::OpCode::ReturnVoid, "RETURN_VOID"}, {minic::OpCode::Unreachable, "UNREACHABLE"},
  };
  minic::BytecodeFunction function{"noop", minic::Type::Void, 0, {1, 6}, {}, {}, {}};
  std::string expected = "bytecode\nfunction 0 \"noop\" void parameters=0 @1:6\n";
  for (std::size_t i = 0; i < std::size(cases); ++i) {
    function.code.push_back({cases[i].opcode, 7, {3, 4}});
    expected += "  " + std::to_string(i) + " " + cases[i].text + " @3:4\n";
  }
  expected += "end\n";
  require(minic::print_bytecode({{function}}) == expected, "Unexpected instruction text or operand.");
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"format", format}, {"instructions", instructions},
  };
  if (argc == 2) {
    for (const auto& test : cases) {
      if (std::string_view(argv[1]) == test.name) {
        try {
          test.run();
          std::cout << "PASS " << test.name << '\n';
          return 0;
        } catch (const std::exception& error) {
          std::cerr << "FAIL " << test.name << ": " << error.what() << '\n';
          return 1;
        }
      }
    }
  }
  std::cerr << "Expected one known test name.\n";
  return 1;
}
