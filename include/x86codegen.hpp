#pragma once

#include <ast.hpp>
#include <memory>

class X86CodeGen {
private:
  void codegen(const AST::NumberExpr &NNode);
  void codegen(const AST::VariableExpr &VNode);
  void codegen(const AST::BinaryExpr &BNode);
  void codegen(const AST::CallExpr &CNode);
  void codegen(const AST::Expr &ENode);
  void codegen(const AST::Statement &SNode);
  void codegen(const AST::IfStatement &SNode);
  void codegen(const AST::Prototype &PNode);
  void codegen(const AST::Function &FNode);
  void codegen(const AST::Module& MNode);

  std::string ProgramText;

public:
  static std::string generate(std::unique_ptr<AST::Module> AST);
};
