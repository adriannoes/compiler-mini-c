#include "ast_printer.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using minic::Expr;
using minic::ExprPtr;
using minic::TokenKind;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Node>
const Node& node(const Expr& expression) {
  const auto* result = std::get_if<Node>(&expression.node);
  require(result != nullptr, "Unexpected expression node.");
  return *result;
}

const minic::BinaryExpr& binary(const Expr& expression, TokenKind op) {
  const auto& result = node<minic::BinaryExpr>(expression);
  require(result.op == op, "Unexpected binary operator.");
  return result;
}

void name(const Expr& expression, std::string_view expected) {
  require(node<minic::NameExpr>(expression).name == expected, "Unexpected name.");
}

void integer(const Expr& expression, std::string_view expected) {
  require(node<minic::IntegerExpr>(expression).text == expected, "Unexpected integer text.");
}

void location(minic::SourceLocation actual, std::size_t line, std::size_t column) {
  require(actual.line == line && actual.column == column, "Unexpected source location.");
}

ExprPtr expression(std::string_view text) {
  const std::string source = "int f() { return " + std::string(text) + "; }";
  auto program = minic::parse(minic::lex(source));
  return std::move(std::get<minic::ReturnStmt>(program.functions[0].body.statements[0]->node).value);
}

void functions() {
  const auto program = minic::parse(minic::lex("int first() { { return 0; } } void second() { return; } bool third() {}"));
  require(program.functions.size() == 3, "Expected three functions.");
  const auto& first = program.functions[0];
  require(first.name == "first" && first.return_type.keyword == TokenKind::KwInt,
          "Unexpected first function.");
  const auto& block = std::get<minic::BlockStmt>(first.body.statements[0]->node);
  const auto& result = std::get<minic::ReturnStmt>(block.statements[0]->node);
  integer(*result.value, "0");
  const auto& second = program.functions[1];
  require(second.return_type.keyword == TokenKind::KwVoid, "Expected void return syntax.");
  require(!std::get<minic::ReturnStmt>(second.body.statements[0]->node).value,
          "Expected a return without a value.");
  require(program.functions[2].return_type.keyword == TokenKind::KwBool &&
              program.functions[2].body.statements.empty(),
          "Expected a bool function with an empty block.");
}

void empty() {
  const auto program = minic::parse(minic::lex(" /* empty */\r\n"));
  require(program.functions.empty(), "Expected an empty program.");
  require(minic::print_ast(program) == "Program\n", "Unexpected empty AST output.");
}

void precedence() {
  const auto result = expression("a = b || c && d == e < f + g * -h");
  const auto& assignment = node<minic::AssignmentExpr>(*result);
  name(*assignment.target, "a");
  const auto& logical_or = binary(*assignment.value, TokenKind::OrOr);
  name(*logical_or.left, "b");
  const auto& logical_and = binary(*logical_or.right, TokenKind::AndAnd);
  name(*logical_and.left, "c");
  const auto& equality = binary(*logical_and.right, TokenKind::EqualEqual);
  name(*equality.left, "d");
  const auto& comparison = binary(*equality.right, TokenKind::Less);
  name(*comparison.left, "e");
  const auto& addition = binary(*comparison.right, TokenKind::Plus);
  name(*addition.left, "f");
  const auto& multiplication = binary(*addition.right, TokenKind::Star);
  name(*multiplication.left, "g");
  const auto& negation = node<minic::UnaryExpr>(*multiplication.right);
  require(negation.op == TokenKind::Minus, "Expected unary minus.");
  name(*negation.operand, "h");
}

const struct OperatorCase {
  const char* text;
  TokenKind kind;
} operators[] = {
    {"+", TokenKind::Plus}, {"-", TokenKind::Minus}, {"*", TokenKind::Star},
    {"/", TokenKind::Slash}, {"%", TokenKind::Percent}, {"==", TokenKind::EqualEqual},
    {"!=", TokenKind::BangEqual}, {"<", TokenKind::Less}, {"<=", TokenKind::LessEqual},
    {">", TokenKind::Greater}, {">=", TokenKind::GreaterEqual},
    {"&&", TokenKind::AndAnd}, {"||", TokenKind::OrOr},
};

void all_operators() {
  for (const auto& op : operators) {
    const auto result = expression(std::string("1 ") + op.text + " 2");
    const auto& operation = binary(*result, op.kind);
    integer(*operation.left, "1");
    integer(*operation.right, "2");
  }
}

void associativity() {
  for (const auto& op : operators) {
    const auto result = expression(std::string("a ") + op.text + " b " + op.text + " c");
    const auto& outer = binary(*result, op.kind);
    const auto& inner = binary(*outer.left, op.kind);
    name(*inner.left, "a");
    name(*inner.right, "b");
    name(*outer.right, "c");
  }
  const auto result = expression("a = b = c");
  const auto& outer = node<minic::AssignmentExpr>(*result);
  const auto& inner = node<minic::AssignmentExpr>(*outer.value);
  name(*outer.target, "a");
  name(*inner.target, "b");
  name(*inner.value, "c");

  const auto unary = expression("!-!false");
  const auto& first = node<minic::UnaryExpr>(*unary);
  const auto& second = node<minic::UnaryExpr>(*first.operand);
  const auto& third = node<minic::UnaryExpr>(*second.operand);
  require(first.op == TokenKind::Bang && second.op == TokenKind::Minus && third.op == TokenKind::Bang,
          "Unexpected unary grouping.");
  require(!node<minic::BooleanExpr>(*third.operand).value, "Expected false.");
}

void parentheses() {
  const auto result = expression("(a + b) * c");
  const auto& multiplication = binary(*result, TokenKind::Star);
  const auto& addition = binary(*multiplication.left, TokenKind::Plus);
  name(*addition.left, "a");
  name(*addition.right, "b");
  name(*multiplication.right, "c");
  const auto assignment = expression("(a) = b");
  name(*node<minic::AssignmentExpr>(*assignment).target, "a");
}

void literals() {
  for (const std::string_view text : {"0", "007", "1234567890123456789012345678901234567890"}) {
    const auto result = expression(text);
    integer(*result, text);
  }
  const auto negative = expression("-007");
  const auto& negation = node<minic::UnaryExpr>(*negative);
  require(negation.op == TokenKind::Minus, "Expected unary minus.");
  integer(*negation.operand, "007");
  const auto boolean = expression("true");
  require(node<minic::BooleanExpr>(*boolean).value, "Expected true.");
}

void positions() {
  const auto program = minic::parse(minic::lex("/* comment */\r\nint main() {\r\treturn (\n  007 + value);\r\n}\r\n"));
  const auto& function = program.functions[0];
  location(function.location, 2, 5);
  location(function.return_type.location, 2, 1);
  location(function.body.location, 2, 12);
  const auto& result = std::get<minic::ReturnStmt>(function.body.statements[0]->node);
  location(result.location, 3, 2);
  location(result.value->location, 4, 7);
  const auto& addition = binary(*result.value, TokenKind::Plus);
  location(addition.left->location, 4, 3);
  location(addition.right->location, 4, 9);
  name(*addition.right, "value");
}

void expect_syntax_error(std::string_view source, minic::SourceLocation expected_location,
                         std::string_view message) {
  try {
    minic::parse(minic::lex(source));
  } catch (const minic::ParseError& error) {
    location(error.location, expected_location.line, expected_location.column);
    require(error.what() == message, "Unexpected diagnostic: " + std::string(error.what()));
    return;
  }
  throw std::runtime_error("Expected a syntax error.");
}

void declarations() {
  const auto program = minic::parse(minic::lex(
      "int f() {\nint value;\nvalue = 007;\nbool flag = true;\n"
      "{ int value = 2; }\n;\nreturn flag;\n}"));
  const auto& statements = program.functions[0].body.statements;
  require(statements.size() == 6, "Expected six block items.");
  const auto& value = std::get<minic::VarDecl>(statements[0]->node);
  require(value.name == "value" && value.type.keyword == TokenKind::KwInt && !value.initializer,
          "Expected a declaration without an initializer.");
  location(value.location, 2, 5);
  location(value.type.location, 2, 1);
  const auto& assignment = node<minic::AssignmentExpr>(
      *std::get<minic::ExprStmt>(statements[1]->node).expression);
  name(*assignment.target, "value");
  integer(*assignment.value, "007");
  const auto& flag = std::get<minic::VarDecl>(statements[2]->node);
  require(flag.name == "flag" && flag.type.keyword == TokenKind::KwBool &&
              node<minic::BooleanExpr>(*flag.initializer).value,
          "Expected a bool declaration with an initializer.");
  const auto& inner = std::get<minic::BlockStmt>(statements[3]->node);
  integer(*std::get<minic::VarDecl>(inner.statements[0]->node).initializer, "2");
  location(std::get<minic::EmptyStmt>(statements[4]->node).location, 6, 1);
  name(*std::get<minic::ReturnStmt>(statements[5]->node).value, "flag");
}

void parameters() {
  const auto program = minic::parse(minic::lex("int sum(int a, bool enabled, void marker) {}"));
  const auto& parameters = program.functions[0].parameters;
  require(parameters.size() == 3, "Expected three parameters.");
  require(parameters[0].name == "a" && parameters[0].type.keyword == TokenKind::KwInt,
          "Unexpected first parameter.");
  require(parameters[1].name == "enabled" && parameters[1].type.keyword == TokenKind::KwBool,
          "Unexpected second parameter.");
  require(parameters[2].name == "marker" && parameters[2].type.keyword == TokenKind::KwVoid,
          "Expected void parameter syntax without type analysis.");
  location(parameters[0].location, 1, 13);
  location(parameters[0].type.location, 1, 9);
}

void calls() {
  const auto result = expression("-add(1 + 2, other(), x = 3) * 4");
  const auto& product = binary(*result, TokenKind::Star);
  integer(*product.right, "4");
  const auto& negation = node<minic::UnaryExpr>(*product.left);
  require(negation.op == TokenKind::Minus, "Expected unary minus before the call.");
  const auto& call = node<minic::CallExpr>(*negation.operand);
  require(call.name == "add" && call.arguments.size() == 3, "Unexpected call arguments.");
  const auto& sum = binary(*call.arguments[0], TokenKind::Plus);
  integer(*sum.left, "1");
  integer(*sum.right, "2");
  const auto& nested = node<minic::CallExpr>(*call.arguments[1]);
  require(nested.name == "other" && nested.arguments.empty(), "Expected a call without arguments.");
  const auto& assignment = node<minic::AssignmentExpr>(*call.arguments[2]);
  name(*assignment.target, "x");
  integer(*assignment.value, "3");
}

void syntax_only() {
  const auto program = minic::parse(minic::lex(
      "void f(void p) { void x; int x; bool y = 1; missing(true, x); return 42; }"));
  require(program.functions.size() == 1 && program.functions[0].body.statements.size() == 5,
          "Expected syntax parsing without semantic analysis.");
}

void declaration_call_errors() {
  const struct {
    const char* source;
    const char* message;
  } cases[] = {
      {"int f(int\n) {}", "Expected a parameter name. Received RIGHT_PAREN."},
      {"int f(int a,\n) {}", "Expected a type. Received RIGHT_PAREN."},
      {"int f(int a\nbool b) {}", "Expected \")\" after parameters. Received BOOL."},
      {"int f(void\n) {}", "Expected a parameter name. Received RIGHT_PAREN."},
      {"int f() { int\n= 1; }", "Expected a variable name. Received ASSIGN."},
      {"int f() { int a =\n; }", "Expected an expression. Received SEMICOLON."},
      {"int f() { int a\n, b; }", "Expected \";\" after declaration. Received COMMA."},
      {"int f() { int a\n}", "Expected \";\" after declaration. Received RIGHT_BRACE."},
      {"int f() { g(1\n2); }", "Expected \")\" after arguments. Received INTEGER."},
      {"int f() { g(1,\n); }", "Expected an expression. Received RIGHT_PAREN."},
      {"int f() { g(1\n; }", "Expected \")\" after arguments. Received SEMICOLON."},
      {"int f() { g()\n(); }", "Expected \";\" after expression. Received LEFT_PAREN."},
      {"int f() { (g)\n(); }", "Expected \";\" after expression. Received LEFT_PAREN."},
      {"int f() { g()\n= 1; }", "Expected a name before \"=\"."},
      {"int f() { int g\n() {} }", "Expected \";\" after declaration. Received LEFT_PAREN."},
  };
  for (const auto& test : cases) {
    expect_syntax_error(test.source, {2, 1}, test.message);
  }
}

void syntax_errors() {
  const struct {
    const char* source;
    minic::SourceLocation location;
    const char* message;
  } cases[] = {
      {"int main() { return 0 }", {1, 23}, "Expected \";\" after return value. Received RIGHT_BRACE."},
      {"int f() { return 1 + ; }", {1, 22}, "Expected an expression. Received SEMICOLON."},
      {"int f() { return (1 + 2; }", {1, 24}, "Expected \")\" after expression. Received SEMICOLON."},
      {"int f() { return 1 = 2; }", {1, 20}, "Expected a name before \"=\"."},
      {"int f() { return a + b = 2; }", {1, 24}, "Expected a name before \"=\"."},
      {"int f() { return ++a; }", {1, 18}, "Expected an expression. Received PLUS."},
      {"int f() { return a += 1; }", {1, 21}, "Expected an expression. Received ASSIGN."},
      {"int f() {\r\n\treturn 1;\r\n", {3, 1}, "Expected \"}\" after block. Received EOF."},
      {"int f() { return", {1, 17}, "Expected an expression. Received EOF."},
      {"main() {}", {1, 1}, "Expected a type. Received IDENTIFIER."},
      {"int () {}", {1, 5}, "Expected a function name. Received LEFT_PAREN."},
      {"int f();", {1, 8}, "Expected \"{\" before block. Received SEMICOLON."},
      {"int x;", {1, 6}, "Expected \"(\" after function name. Received SEMICOLON."},
      {"int f() {} 1", {1, 12}, "Expected a type. Received INTEGER."},
  };
  for (const auto& test : cases) {
    expect_syntax_error(test.source, test.location, test.message);
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Expected one test name.\n";
    return 1;
  }
  const struct {
    const char* name;
    void (*run)();
  } cases[] = {
      {"functions", functions}, {"empty", empty}, {"precedence", precedence},
      {"operators", all_operators}, {"associativity", associativity},
      {"parentheses", parentheses}, {"literals", literals},
      {"positions", positions}, {"syntax_errors", syntax_errors},
      {"declarations", declarations}, {"parameters", parameters}, {"calls", calls},
      {"syntax_only", syntax_only}, {"declaration_call_errors", declaration_call_errors},
  };
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
  std::cerr << "Unknown test: " << argv[1] << '\n';
  return 1;
}
