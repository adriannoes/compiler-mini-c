#pragma once

#include "token.hpp"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minic {

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

struct Expr {
  SourceLocation location;
  std::variant<IntegerExpr, BooleanExpr, NameExpr, UnaryExpr, BinaryExpr,
               AssignmentExpr> node;
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

struct Stmt {
  std::variant<BlockStmt, ReturnStmt> node;
};

struct TypeSyntax {
  TokenKind keyword;
  SourceLocation location;
};

struct FunctionDecl {
  SourceLocation location;
  TypeSyntax return_type;
  std::string name;
  BlockStmt body;
};

struct Program {
  std::vector<FunctionDecl> functions;
};

}  // namespace minic
