#include "ast_printer.hpp"
#include "bytecode_compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "type_checker.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>

namespace {

using minic::OpCode;
using minic::Type;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void validate(const minic::BytecodeProgram& program) {
  for (const auto& function : program.functions) {
    require(function.parameter_count <= function.locals.size(), "Expected parameter slots before local slots.");
    require(!function.code.empty(), "Expected a function body.");
    for (const auto& instruction : function.code) {
      switch (instruction.opcode) {
        case OpCode::LoadConstant:
          require(instruction.operand < function.constants.size(), "Invalid constant index.");
          break;
        case OpCode::LoadLocal:
        case OpCode::StoreLocal:
        case OpCode::ResetLocal:
          require(instruction.operand < function.locals.size(), "Invalid local index.");
          break;
        case OpCode::Jump:
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
          require(instruction.operand < function.code.size(), "Invalid jump target.");
          break;
        case OpCode::Call:
          require(instruction.operand < program.functions.size(), "Invalid function index.");
          break;
        default: break;
      }
      require(instruction.location.line > 0 && instruction.location.column > 0, "Expected a source position.");
    }

    using Stack = std::vector<Type>;
    std::vector<std::optional<Stack>> states(function.code.size());
    std::vector<std::pair<std::size_t, Stack>> pending{{0, {}}};
    while (!pending.empty()) {
      const auto offset = pending.back().first;
      auto stack = std::move(pending.back().second);
      pending.pop_back();
      require(offset < function.code.size(), "Control flow left the instruction vector.");
      if (states[offset]) {
        require(*states[offset] == stack, "Branch paths have different stack types.");
        continue;
      }
      states[offset] = stack;
      const auto& instruction = function.code[offset];
      const auto pop = [&]() {
        require(!stack.empty(), "The generated code underflows the value stack.");
        const auto type = stack.back();
        stack.pop_back();
        return type;
      };
      const auto take = [&](Type type) { require(pop() == type, "Unexpected stack operand type."); };
      switch (instruction.opcode) {
        case OpCode::Step:
        case OpCode::ResetLocal: break;
        case OpCode::LoadConstant:
          stack.push_back(std::holds_alternative<bool>(function.constants[instruction.operand]) ? Type::Bool : Type::Int);
          break;
        case OpCode::LoadLocal:
          stack.push_back(function.locals[instruction.operand].type);
          break;
        case OpCode::StoreLocal:
          take(function.locals[instruction.operand].type);
          stack.push_back(function.locals[instruction.operand].type);
          break;
        case OpCode::Pop: pop(); break;
        case OpCode::Negate: take(Type::Int); stack.push_back(Type::Int); break;
        case OpCode::Not: take(Type::Bool); stack.push_back(Type::Bool); break;
        case OpCode::Add:
        case OpCode::Subtract:
        case OpCode::Multiply:
        case OpCode::Divide:
        case OpCode::Remainder:
          take(Type::Int); take(Type::Int); stack.push_back(Type::Int);
          break;
        case OpCode::Less:
        case OpCode::LessEqual:
        case OpCode::Greater:
        case OpCode::GreaterEqual:
          take(Type::Int); take(Type::Int); stack.push_back(Type::Bool);
          break;
        case OpCode::Equal:
        case OpCode::NotEqual: {
          const auto right = pop();
          take(right);
          stack.push_back(Type::Bool);
          break;
        }
        case OpCode::Jump:
          pending.emplace_back(instruction.operand, stack);
          continue;
        case OpCode::JumpIfFalse:
        case OpCode::JumpIfTrue:
          take(Type::Bool);
          pending.emplace_back(instruction.operand, stack);
          break;
        case OpCode::Call: {
          const auto& callee = program.functions[instruction.operand];
          for (std::size_t argument = callee.parameter_count; argument > 0; --argument) {
            take(callee.locals.at(argument - 1).type);
          }
          if (callee.return_type != Type::Void) {
            stack.push_back(callee.return_type);
          }
          break;
        }
        case OpCode::Return:
          require(function.return_type != Type::Void, "A void function returned a value.");
          take(function.return_type);
          require(stack.empty(), "A return left intermediate values on the stack.");
          continue;
        case OpCode::ReturnVoid:
          require(function.return_type == Type::Void && stack.empty(), "Invalid void return.");
          continue;
        case OpCode::Unreachable:
          throw std::runtime_error("A non-void function can fall through its generated code.");
      }
      pending.emplace_back(offset + 1, std::move(stack));
    }
  }
}

minic::BytecodeProgram compile(std::string_view source) {
  auto result = minic::compile_bytecode(minic::parse(minic::lex(source)));
  validate(result);
  return result;
}

std::vector<std::size_t> operands(const minic::BytecodeFunction& function, OpCode opcode) {
  std::vector<std::size_t> result;
  for (const auto& instruction : function.code) {
    if (instruction.opcode == opcode) {
      result.push_back(instruction.operand);
    }
  }
  return result;
}

void empty() { require(compile("").functions.empty(), "Expected empty compilation without an entry."); }

void entry_independent() {
  const auto program = compile("int helper(int n) { return n; }");
  require(program.functions[0].name == "helper" && program.functions[0].parameter_count == 1,
          "Compilation must not impose entry rules.");
}

void expressions() {
  const auto program = compile("bool f(int n, bool b) { return !b == (-(n + 2 * 3 - 4 / 2 % 2) < 0) "
                               "!= (n <= 1) == (n > 2) != (n >= 3); }");
  const auto& function = program.functions[0];
  for (const auto opcode : {OpCode::Not, OpCode::Negate, OpCode::Add, OpCode::Subtract, OpCode::Multiply,
                            OpCode::Divide, OpCode::Remainder, OpCode::Less, OpCode::LessEqual,
                            OpCode::Greater, OpCode::GreaterEqual, OpCode::Equal, OpCode::NotEqual}) {
    require(!operands(function, opcode).empty(), "Expected every arithmetic and comparison operation.");
  }
}

void assignments() {
  const auto program = compile("int main() { int a; int b; int c = a = b = 3; return c; }");
  require(operands(program.functions[0], OpCode::ResetLocal) == std::vector<std::size_t>({0, 1, 2}),
          "Expected a reset for each declaration.");
  require(operands(program.functions[0], OpCode::StoreLocal) == std::vector<std::size_t>({1, 0, 2}),
          "Expected chained stores from the inner assignment to the initializer.");
  require(operands(program.functions[0], OpCode::LoadLocal) == std::vector<std::size_t>({2}),
          "Assignment must not read an uninitialized target.");
}

void scopes() {
  const auto program = compile("int f(int n) { { int n = 2; } int x = n; { x = x + 1; int x = x; } return x; }");
  const auto& function = program.functions[0];
  require(function.locals.size() == 4 && function.locals[0].name == "n" && function.locals[1].name == "n",
          "Expected distinct parameter and shadowed local slots.");
  require(operands(function, OpCode::LoadLocal) == std::vector<std::size_t>({0, 2, 3, 2}),
          "Expected lexical bindings before shadowing and in self-initializers.");
}

void loop_slots() {
  const auto program = compile("int main() { int i = 0; while (i < 2) { int x; if (i == 0) x = 1; "
                               "i = i + 1; x; } return 0; }");
  const auto& function = program.functions[0];
  require(function.locals.size() == 2, "A repeated declaration must keep one lexical slot.");
  const auto back = std::find_if(function.code.begin(), function.code.end(), [](const auto& instruction) {
    return instruction.opcode == OpCode::Jump;
  });
  require(back != function.code.end(), "Expected a loop back edge.");
  require(back->operand < static_cast<std::size_t>(std::distance(function.code.begin(), back)),
          "Expected the loop jump to target an earlier instruction.");
  const auto reset = std::find_if(function.code.begin() + back->operand, back, [](const auto& instruction) {
    return instruction.opcode == OpCode::ResetLocal && instruction.operand == 1;
  });
  require(reset != back, "Each iteration must clear the loop-local slot.");
}

void conditionals() {
  compile("int main() { int x = 0; if (true) if (false) x = 1; else x = 2; return x; }");
  compile("int f(bool b) { if (b) { return 1; } else { return 2; } }");
  compile("void f(bool b) { if (b) ; }");
}

void loops() {
  const auto program = compile("void f() { while (true) while (false) ; }");
  const auto& function = program.functions[0];
  std::size_t backwards = 0;
  for (std::size_t offset = 0; offset < function.code.size(); ++offset) {
    const auto& instruction = function.code[offset];
    if (instruction.opcode == OpCode::Jump && instruction.operand < offset) {
      const auto& target = function.code[instruction.operand];
      require(target.opcode == OpCode::Step, "The loop must reevaluate its condition step.");
      ++backwards;
    }
  }
  require(backwards == 2, "Expected a back edge for each loop.");
}

void short_circuit() {
  const auto program = compile("bool main() { return false && (1 / 0 == 0) || true; }");
  const auto& function = program.functions[0];
  require(operands(function, OpCode::JumpIfFalse).size() == 1 && operands(function, OpCode::JumpIfTrue).size() == 1,
          "Expected separate short-circuit paths.");
  for (const auto opcode : {OpCode::JumpIfFalse, OpCode::JumpIfTrue}) {
    const auto target = operands(function, opcode)[0];
    const auto& selected = function.code[target];
    require(selected.opcode == OpCode::LoadConstant, "The skipped path must supply its boolean result.");
    require(std::get<bool>(function.constants[selected.operand]) == (opcode == OpCode::JumpIfTrue),
            "Unexpected short-circuit result.");
  }
}

void calls() {
  const auto program = compile("int main() { int x = f(1, true); f(x, false); return x; } "
                               "int f(int n, bool b) { if (b) return n; return 0; }");
  require(operands(program.functions[0], OpCode::Call) == std::vector<std::size_t>({1, 1}),
          "Expected calls to a later function by source-order index.");
  require(program.functions[1].locals[0].name == "n" && program.functions[1].locals[1].name == "b",
          "Expected parameter slots in argument order.");
  compile("int main() { int x = f(); int f = 7; return x + f; } int f() { return 1; }");
}

void evaluation_order() {
  const auto program = compile("int pair(int a, int b) { return a * 10 + b; } "
                               "int main() { int x = 0; return pair(x = 1, x = 2) + pair(3, pair(4, 5)); }");
  const auto& function = program.functions[1];
  require(operands(function, OpCode::Call) == std::vector<std::size_t>({0, 0, 0}), "Expected nested calls in order.");
  std::vector<std::string> digits;
  for (const auto& instruction : function.code) {
    if (instruction.opcode == OpCode::LoadConstant) {
      digits.push_back(std::get<minic::IntegerConstant>(function.constants[instruction.operand]).digits);
    }
  }
  require(digits == std::vector<std::string>({"0", "1", "2", "3", "4", "5"}),
          "Expected left-to-right expression and argument evaluation.");
}

void recursion() {
  const auto program = compile("bool even(int n) { if (n == 0) return true; return odd(n - 1); } "
                               "bool odd(int n) { if (n == 0) return false; return even(n - 1); } "
                               "int sum(int n) { if (n == 0) return n; return n + sum(n - 1); }");
  require(operands(program.functions[0], OpCode::Call) == std::vector<std::size_t>({1}) &&
              operands(program.functions[1], OpCode::Call) == std::vector<std::size_t>({0}) &&
              operands(program.functions[2], OpCode::Call) == std::vector<std::size_t>({2}),
          "Expected mutual and direct recursive function indices.");
}

void returns() {
  compile("void f() { return; 1 / 0; } void main() { f(); { while (true) { return; } } }");
  compile("int f() { while (true) { if (true) return 7; } return 0; }");
  const auto program = compile("void noop() {} void main() { noop(); }");
  require(operands(program.functions[1], OpCode::Pop).empty(), "A void call must not create a value to discard.");
  require(program.functions[0].code.back().opcode == OpCode::ReturnVoid, "Expected void fallthrough.");
}

void numeric_text() {
  const auto program = compile("int main() { return -0009223372036854775808; return 99999999999999999999999999; }");
  const auto& constants = program.functions[0].constants;
  const auto& minimum = std::get<minic::IntegerConstant>(constants[0]);
  const auto& large = std::get<minic::IntegerConstant>(constants[1]);
  require(minimum.negative && minimum.digits == "0009223372036854775808", "Expected unconverted minimum integer text.");
  require(!large.negative && large.digits == "99999999999999999999999999", "Expected unconverted unreachable literal.");
  compile("int main() { int x = x; return 1 / 0 + 9223372036854775808; }");
}

void step_markers() {
  const auto program = compile("int main() {\n  int x;\n  return x = -1;\n}");
  std::vector<std::pair<std::size_t, std::size_t>> positions;
  for (const auto& instruction : program.functions[0].code) {
    if (instruction.opcode == OpCode::Step) {
      positions.emplace_back(instruction.location.line, instruction.location.column);
    }
  }
  require(positions == std::vector<std::pair<std::size_t, std::size_t>>({{1, 12}, {2, 7}, {3, 3}, {3, 12}, {3, 14}, {3, 15}}),
          "Expected source steps without a target read, including two negative-literal steps.");
}

void static_errors() {
  try {
    compile("int main() { return 1 / 0; } int helper() { return missing; }");
    throw std::runtime_error("Expected a name error in an uncalled function.");
  } catch (const minic::ResolveError& error) {
    require(std::string(error.what()) == "Undeclared name \"missing\".", "Unexpected name diagnostic.");
  }
  try {
    compile("int main() { return 0; } int helper() { return true; }");
    throw std::runtime_error("Expected a type error in an uncalled function.");
  } catch (const minic::TypeError& error) {
    require(std::string(error.what()) == "Expected type int. Received bool.", "Unexpected type diagnostic.");
  }
  for (const std::string source : {"void f() {} int main() { return f(); }",
                                  "int f(int n) { return n; } int main() { return f(); }"}) {
    try {
      compile(source);
      throw std::runtime_error("Expected a static call error.");
    } catch (const minic::TypeError&) {}
  }
}

void ownership() {
  const auto program = minic::parse(minic::lex("int main() { int x = 007; return x; }"));
  const auto before = minic::print_ast(program);
  const auto first = minic::compile_bytecode(program);
  const auto second = minic::compile_bytecode(program);
  require(minic::print_ast(program) == before, "Compilation changed the AST.");
  require(minic::print_bytecode(first) == minic::print_bytecode(second), "Expected deterministic repeated compilation.");
  const auto owned = compile("int main() { return 42; }");
  require(owned.functions[0].name == "main" && !minic::print_bytecode(owned).empty(),
          "Expected bytecode to outlive a temporary AST.");
}

void fixtures(const std::string& directory) {
  for (const std::string name : {"hello", "lexer", "calls", "control_flow", "scopes", "recursion", "types",
                                 "interpreter", "short_circuit", "runtime_error", "factorial", "fibonacci"}) {
    std::ifstream input(directory + "/" + name + ".mc", std::ios::binary);
    require(input.is_open(), "Cannot open the example " + name + ".");
    const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const auto result = compile(source);
    require(!result.functions.empty(), "Expected compiled functions in the example " + name + ".");
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3 && std::string_view(argv[1]) == "fixtures") {
    try {
      fixtures(argv[2]);
      std::cout << "PASS fixtures\n";
      return 0;
    } catch (const std::exception& error) {
      std::cerr << "FAIL fixtures: " << error.what() << '\n';
      return 1;
    }
  }
  const struct { const char* name; void (*run)(); } cases[] = {
      {"empty", empty}, {"entry_independent", entry_independent}, {"expressions", expressions},
      {"assignments", assignments}, {"scopes", scopes}, {"loop_slots", loop_slots},
      {"conditionals", conditionals}, {"loops", loops}, {"short_circuit", short_circuit},
      {"calls", calls}, {"evaluation_order", evaluation_order}, {"recursion", recursion},
      {"returns", returns}, {"numeric_text", numeric_text}, {"step_markers", step_markers},
      {"static_errors", static_errors}, {"ownership", ownership},
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
