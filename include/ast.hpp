#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <my_types.hpp>
#include <string>
#include <variant>
#include <vector>

class CodeGenerator;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

class ExprASTWrapper;

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST {
  f64 Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
  friend CodeGenerator;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
  friend CodeGenerator;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST {
  char Op;
  std::unique_ptr<ExprASTWrapper> LHS, RHS;

public:
  BinaryExprAST(char Op, std::unique_ptr<ExprASTWrapper> LHS,
                std::unique_ptr<ExprASTWrapper> RHS);
  friend CodeGenerator;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprASTWrapper>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprASTWrapper>> Args);
  friend CodeGenerator;
};

class ExprASTWrapper {
  using ExprAST = std::variant<
      std::unique_ptr<NumberExprAST>, std::unique_ptr<VariableExprAST>,
      std::unique_ptr<BinaryExprAST>, std::unique_ptr<CallExprAST>>;
  ExprAST Value;

public:
  // TODO: Ensure T is one of ExprAST
  template <typename T>
  ExprASTWrapper(std::unique_ptr<T> Value) : Value{std::move(Value)} {}

  template <typename T>
  static std::unique_ptr<ExprASTWrapper> from(std::unique_ptr<T> Value) {
    return std::make_unique<ExprASTWrapper>(std::move(Value));
  }

  friend CodeGenerator;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
      : Name(Name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
  friend CodeGenerator;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprASTWrapper> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprASTWrapper> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  friend CodeGenerator;
};

using TopLevelItem =
    std::variant<std::unique_ptr<FunctionAST>, std::unique_ptr<PrototypeAST>>;
// Represents the whole program as an abstract syntax tree
class ModuleAST {
  std::vector<TopLevelItem> TopLevelItems;

public:
  ModuleAST(std::vector<TopLevelItem> TopLevelItems)
      : TopLevelItems{std::move(TopLevelItems)} {}
  friend CodeGenerator;
};