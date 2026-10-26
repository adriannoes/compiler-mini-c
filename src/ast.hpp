#pragma once

#include "token.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minic {

struct TypeSyntax {
  TokenKind keyword;
  SourceLocation location;
};

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct IntegerExpr {
  std::string text;
};

struct BooleanExpr {
  bool value;
};

struct NameExpr {
  std::string name;
};

struct UnaryExpr {
  TokenKind op;
  ExprPtr operand;
};

struct BinaryExpr {
  ExprPtr left;
  TokenKind op;
  ExprPtr right;
};

struct AssignmentExpr {
  ExprPtr target;
  ExprPtr value;
};

struct CallExpr {
  std::string name;
  std::vector<ExprPtr> arguments;
};

struct Expr {
  SourceLocation location;
  std::variant<IntegerExpr, BooleanExpr, NameExpr, UnaryExpr, BinaryExpr,
               AssignmentExpr, CallExpr> node;
};

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct BlockStmt {
  SourceLocation location;
  std::vector<StmtPtr> statements;
};

struct ReturnStmt {
  SourceLocation location;
  ExprPtr value;
};

struct VarDecl {
  SourceLocation location;
  TypeSyntax type;
  std::string name;
  ExprPtr initializer;
};

struct ExprStmt {
  SourceLocation location;
  ExprPtr expression;
};

struct EmptyStmt {
  SourceLocation location;
};

struct IfStmt {
  SourceLocation location;
  ExprPtr condition;
  StmtPtr then_branch;
  StmtPtr else_branch;
};

struct WhileStmt {
  SourceLocation location;
  ExprPtr condition;
  StmtPtr body;
};

struct Stmt {
  std::variant<BlockStmt, ReturnStmt, VarDecl, ExprStmt, EmptyStmt, IfStmt, WhileStmt> node;
};

struct Parameter {
  SourceLocation location;
  TypeSyntax type;
  std::string name;
};

struct FunctionDecl {
  SourceLocation location;
  TypeSyntax return_type;
  std::string name;
  std::vector<Parameter> parameters;
  BlockStmt body;
};

struct Program {
  std::vector<FunctionDecl> functions;
};

}  // namespace minic
