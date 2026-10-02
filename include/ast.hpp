#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <my_types.hpp>
#include <string>
#include <variant>
#include <vector>

struct LLVMCodeGen;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

struct Expr;

/// NumberExprAST - Expression struct for numeric literals like "1.0".
struct NumberExpr {
  i64 Val;

  NumberExpr(i64 Val) : Val(Val) {}
};

/// VariableExprAST - Expression struct for referencing a variable, like "a".
struct VariableExpr {
  std::string Name;

  VariableExpr(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST - Expression struct for a binary operator.
struct BinaryExpr {
  char Op;
  std::unique_ptr<Expr> LHS, RHS;

  BinaryExpr(char Op, std::unique_ptr<Expr> LHS, std::unique_ptr<Expr> RHS);
};

/// CallExprAST - Expression struct for function calls.
struct CallExpr {
  std::string Callee;
  std::vector<std::unique_ptr<Expr>> Args;

  CallExpr(const std::string &Callee, std::vector<std::unique_ptr<Expr>> Args);
};

struct Expr {
  using ExprAST = std::variant<std::unique_ptr<NumberExpr>,
                               std::unique_ptr<VariableExpr>,
                               std::unique_ptr<BinaryExpr>,
                               std::unique_ptr<CallExpr>>;
  ExprAST Value;

  // TODO: Ensure T is one of ExprAST
  template <typename T>
  Expr(std::unique_ptr<T> Value) : Value{std::move(Value)} {}

  template <typename T>
  static std::unique_ptr<Expr> from(std::unique_ptr<T> Value) {
    return std::make_unique<Expr>(std::move(Value));
  }
};

struct Statement;

// if (expr) { statementsequence }
struct IfStatement {
  std::unique_ptr<Expr> Condition;
  using IfBodyType = std::vector<std::unique_ptr<Statement>>;
  IfBodyType Body;

  IfStatement(std::unique_ptr<Expr> Condition, IfBodyType Body);
};

// return expr;
struct ReturnStatement {
  std::unique_ptr<Expr> Value;

  ReturnStatement(std::unique_ptr<Expr> Value);
};

struct Statement {
  using StatementType = std::variant<std::unique_ptr<Expr>,
                                     std::unique_ptr<IfStatement>,
                                     std::unique_ptr<ReturnStatement>>;

  template <typename T>
  Statement(std::unique_ptr<T> Value) : Value{std::move(Value)} {}

  StatementType Value;
};

enum class RLType { I64, Void };
/// PrototypeAST - This struct represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
struct PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  RLType ReturnType;

  PrototypeAST(const std::string &Name,
               std::vector<std::string> Args,
               RLType ReturnType);

  const std::string &getName() const {
    return Name;
  }
  friend LLVMCodeGen;
};

/// FunctionAST - This struct represents a function definition itself.
struct FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<Statement>> Body;

  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<Statement>> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  friend LLVMCodeGen;
};

using TopLevelItem =
  std::variant<std::unique_ptr<FunctionAST>, std::unique_ptr<PrototypeAST>>;
// Represents the whole program as an abstract syntax tree
struct ModuleAST {
  std::vector<TopLevelItem> TopLevelItems;

  ModuleAST(std::vector<TopLevelItem> TopLevelItems)
      : TopLevelItems{std::move(TopLevelItems)} {}
  friend LLVMCodeGen;
};