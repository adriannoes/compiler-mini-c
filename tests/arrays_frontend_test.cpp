#include "ast_printer.hpp"
#include "bytecode_compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "type_checker.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

minic::Program parse(std::string_view source) { return minic::parse(minic::lex(source)); }

void tokens() {
  const auto result = minic::lex("int xs[2];\r\nxs[0] = 1;");
  require(result.size() == 14, "Unexpected array token count.");
  require(result[2].kind == minic::TokenKind::LeftBracket && result[4].kind == minic::TokenKind::RightBracket,
          "Expected bracket tokens.");
  require(result[2].lexeme == "[" && result[2].location.line == 1 && result[2].location.column == 7 &&
              result[7].location.line == 2 && result[7].location.column == 3, "Unexpected bracket positions.");
  require(std::string(minic::token_kind_name(result[2].kind)) == "LEFT_BRACKET" &&
              std::string(minic::token_kind_name(result[4].kind)) == "RIGHT_BRACKET", "Unexpected bracket token names.");
}

void syntax() {
  const auto program = parse("int main() { int a[0002]; a[1] = 3; return a[1]; }");
  const auto& body = program.functions[0].body;
  const auto& declaration = std::get<minic::VarDecl>(body.statements[0]->node);
  require(declaration.array_extent && declaration.array_extent->text == "0002" && !declaration.initializer,
          "Expected owned array extent text without conversion.");
  const auto& assignment = std::get<minic::AssignmentExpr>(std::get<minic::ExprStmt>(body.statements[1]->node).expression->node);
  require(std::get<minic::IndexExpr>(assignment.target->node).name == "a", "Expected an indexed assignment target.");
  const auto text = minic::print_ast(program);
  require(text.find("Length \"0002\"") != std::string::npos && text.find("Index \"a\"") != std::string::npos,
          "Expected array syntax in the AST output.");
  parse("void main() { int a[999999999999999999999]; a[true]; }");
}

void syntax_errors() {
  for (const std::string source : {
           "void main() { int a[]; }", "void main() { int a[-1]; }", "void main() { int a[1 + 1]; }",
           "void main() { int a[2][3]; }", "void main() { int a[2] = {1, 2}; }",
           "int f(int a[2]) { return 0; }", "void main() { int a[2]; a[]; }",
           "void main() { int a[2]; a[0][1]; }", "void main() { int a[2]; a[0](1); }"}) {
    try {
      parse(source);
      throw std::runtime_error("Expected an array syntax error.");
    } catch (const minic::ParseError&) {}
  }
}

void bindings() {
  const auto program = parse("int main() { int a[2]; int i = 0; a[i] = 1; return a[i]; }");
  const auto resolution = minic::resolve_names(program);
  const auto& uses = resolution.bindings();
  require(uses.size() == 4 && uses[0].kind == minic::UseKind::Write && uses[1].kind == minic::UseKind::Read &&
              uses[2].kind == minic::UseKind::Read && uses[0].symbol == uses[2].symbol && uses[1].symbol == uses[3].symbol,
          "Expected indexed read and write bindings, with index reads in order.");
  const auto shadowed = parse("int main() { int a[2]; { bool a; a = true; } return a[0]; }");
  const auto names = minic::resolve_names(shadowed);
  require(names.bindings().front().symbol.index != names.bindings().back().symbol.index,
          "Expected scalar shadowing to preserve the outer array binding.");
}

void types() {
  const auto program = parse("int main() { int a[3]; bool b[2]; a[0] = 1; b[0] = true; return a[0]; }");
  const auto analysis = minic::check_types(program);
  const auto& statements = program.functions[0].body.statements;
  require(analysis.array_length(std::get<minic::VarDecl>(statements[0]->node)) == 3 &&
              analysis.array_length(std::get<minic::VarDecl>(statements[1]->node)) == 2, "Unexpected validated array lengths.");
  const auto& value = *std::get<minic::ReturnStmt>(statements.back()->node).value;
  require(analysis.type_of(value) == minic::Type::Int, "Expected the scalar element type.");
  const auto unexecuted = parse("bool f(int n) { bool a[2]; return a[n]; }");
  minic::check_types(unexecuted);
  const auto large_index = parse("int main() { int a[2]; return a[9223372036854775808]; }");
  minic::check_types(large_index);
}

void lengths() {
  for (const std::string length : {"0", "0000", "1025", "999999999999999999999999999999"}) {
    const auto program = parse("void main() { int a[" + length + "]; }");
    const auto position = std::get<minic::VarDecl>(program.functions[0].body.statements[0]->node).array_extent->location;
    try {
      minic::check_types(program);
      throw std::runtime_error("Expected an array length error.");
    } catch (const minic::TypeError& error) {
      require(std::string(error.what()) == "Array length must be from 1 through 1024." &&
                  error.location.line == position.line && error.location.column == position.column,
              "Unexpected array length diagnostic or position.");
    }
  }
  const auto program = parse("void main() { bool a[0001024]; }");
  const auto analysis = minic::check_types(program);
  require(analysis.array_length(std::get<minic::VarDecl>(program.functions[0].body.statements[0]->node)) == 1024,
          "Expected decimal leading zeroes and the upper length boundary.");
}

void type_errors() {
  for (const std::string source : {
           "int main() { int a[2]; return a; }", "void main() { int a[2]; int b[2]; a = b; }",
           "void f(int n) {} void main() { int a[2]; f(a); }", "int f(int n) { return n[0]; }",
           "int main() { int n = 1; return n[0]; }", "int main() { int a[2]; return a[true]; }",
           "void main() { int a[2]; a[0] = true; }", "void main() { bool a[2]; a[0] = 1; }",
           "void main() { void a[2]; }"}) {
    const auto program = parse(source);
    try {
      minic::check_types(program);
      throw std::runtime_error("Expected an array type error.");
    } catch (const minic::TypeError&) {}
  }
}

void lowering() {
  const auto program = minic::compile_bytecode(parse("int f() { return 3; } int main() { int a[2]; a[1] = f(); return a[1]; }"));
  const auto& function = program.functions[1];
  require(function.locals[0].array_length == 2, "Expected array length in the local metadata.");
  std::vector<minic::OpCode> operations;
  for (const auto& instruction : function.code) {
    if (instruction.opcode == minic::OpCode::CheckIndex || instruction.opcode == minic::OpCode::Call ||
        instruction.opcode == minic::OpCode::StoreElement || instruction.opcode == minic::OpCode::LoadElement) {
      operations.push_back(instruction.opcode);
    }
  }
  require(operations == std::vector<minic::OpCode>({minic::OpCode::CheckIndex, minic::OpCode::Call,
                                                 minic::OpCode::StoreElement, minic::OpCode::LoadElement}),
          "Expected index validation before the right operand, followed by write and read instructions.");
  require(minic::print_bytecode(program).find("local 0 \"a\" int[2]") != std::string::npos,
          "Expected array metadata in disassembly.");
}

void stage_priority() {
  const auto program = parse("int main() { int a[0]; return 0; } int f() { return missing; }");
  try {
    minic::check_types(program);
    throw std::runtime_error("Expected name analysis before array length analysis.");
  } catch (const minic::ResolveError& error) {
    require(std::string(error.what()) == "Undeclared name \"missing\".", "Unexpected stage priority diagnostic.");
  }
  const auto syntax_only = parse("void main() { int a[0]; a[true]; }");
  minic::resolve_names(syntax_only);
}

}  // namespace

int main(int argc, char** argv) {
  const struct { const char* name; void (*run)(); } cases[] = {
      {"tokens", tokens}, {"syntax", syntax}, {"syntax_errors", syntax_errors}, {"bindings", bindings},
      {"types", types}, {"lengths", lengths}, {"type_errors", type_errors}, {"lowering", lowering},
      {"stage_priority", stage_priority},
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
