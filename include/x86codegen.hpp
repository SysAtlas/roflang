#pragma once

#include <ast.hpp>
#include <memory>

class X86CodeGen {
private:
  void codegen(const NumberExpr &NNode);
  void codegen(const VariableExpr &VNode);
  void codegen(const BinaryExpr &BNode);
  void codegen(const CallExpr &CNode);
  void codegen(const Expr &ENode);
  void codegen(const Statement &SNode);
  void codegen(const IfStatement &SNode);
  void codegen(const PrototypeAST &PNode);
  void codegen(const FunctionAST &FNode);
  void codegen(const ModuleAST& MNode);

  std::string ProgramText;

public:
  static std::string generate(std::unique_ptr<ModuleAST> AST);
};
