#pragma once

#include <helper.hpp>
#include <lexer.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace AST {

struct NumberExpr;
struct VariableExpr;
struct BinaryExpr;
struct CallExpr;
struct IfStatement;
struct ReturnStatement;
struct Prototype;
struct Function;
struct Module;

using Expr = std::variant<std::unique_ptr<NumberExpr>,
                          std::unique_ptr<VariableExpr>,
                          std::unique_ptr<BinaryExpr>,
                          std::unique_ptr<CallExpr>>;
using Statement = std::
  variant<std::unique_ptr<Expr>, std::unique_ptr<IfStatement>, std::unique_ptr<ReturnStatement>>;

// Not used in AST itself, but used by other consumers.
using ASTNodeView = std::variant<const NumberExpr *,
                                 const VariableExpr *,
                                 const BinaryExpr *,
                                 const CallExpr *,
                                 const IfStatement *,
                                 const ReturnStatement *,
                                 const Prototype *,
                                 const Function *,
                                 const Module *>;

/// NumberExprAST - Expression struct for numeric literals like "1.0".
struct NumberExpr {
  i64 Val;

  explicit NumberExpr(i64 Val) : Val(Val) {}
};

/// VariableExprAST - Expression struct for referencing a variable, like "a".
struct VariableExpr {
  std::string Name;

  explicit VariableExpr(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST - Expression struct for a binary operator.
struct BinaryExpr {
  BinOpType Op;
  Expr LHS, RHS;

  BinaryExpr(BinOpType Op, Expr &&LHS, Expr &&RHS);
};

/// CallExprAST - Expression struct for function calls.
struct CallExpr {
  std::string Callee;
  std::vector<Expr> Args;

  CallExpr(const std::string &Callee, std::vector<Expr> &&Args);
};

// if (expr) { statementsequence }
struct IfStatement {
  Expr Condition;
  using IfBodyType = std::vector<Statement>;
  IfBodyType Body;

  IfStatement(Expr &&Condition, IfBodyType Body);
};

// return expr;
struct ReturnStatement {
  std::optional<Expr> Value;

  explicit ReturnStatement(std::optional<Expr> &&Value);
};

/// PrototypeAST - This struct represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
struct Prototype {
  std::string Name;
  std::vector<std::string> Args;
  RLType ReturnType;

  Prototype(const std::string &Name,
            std::vector<std::string> Args,
            RLType ReturnType);
};

/// Function - This struct represents a function definition itself.
struct Function {
  std::unique_ptr<Prototype> Proto;
  std::vector<Statement> Body;

  Function(std::unique_ptr<Prototype> Proto, std::vector<Statement> &&Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

// Represents the whole program as an abstract syntax tree
struct Module {
  using TopLevelItem =
    std::variant<std::unique_ptr<Function>, std::unique_ptr<Prototype>>;
  std::vector<TopLevelItem> TopLevelItems;

  Module(std::vector<TopLevelItem> TopLevelItems)
      : TopLevelItems{std::move(TopLevelItems)} {}
};

void getSubtree(const NumberExpr &E, std::vector<ASTNodeView> &Acc);
void getSubtree(const VariableExpr &E, std::vector<ASTNodeView> &Acc);
void getSubtree(const BinaryExpr &E, std::vector<ASTNodeView> &Acc);

void getSubtree(const CallExpr &E, std::vector<ASTNodeView> &Acc);

void getSubtree(const Expr &E, std::vector<ASTNodeView> &Acc);

void getSubtree(const IfStatement &S, std::vector<ASTNodeView> &Acc);
void getSubtree(const ReturnStatement &S, std::vector<ASTNodeView> &Acc);
void getSubtree(const Statement &S, std::vector<ASTNodeView> &Acc);

void getSubtree(const Prototype &P, std::vector<ASTNodeView> &Acc);
void getSubtree(const Function &F, std::vector<ASTNodeView> &Acc);
void getSubtree(const Module &M, std::vector<ASTNodeView> &Acc);

} // namespace AST