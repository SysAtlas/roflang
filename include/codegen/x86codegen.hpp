#pragma once

#include "target/x86/registers.hpp"
#include <target/x86/x86.hpp>
#include <frontend/ast.hpp>
#include <memory>

namespace X86 {

class RegisterTracker;

class CodeGen {
private:
  // Helper functions
  void compareSetHelper(BinOpType T, const Register& Dst);
  void binOpHelper(BinOpType T, const Register& L, const Register& R);

  // Main functions
  Register codegen(const AST::NumberExpr &NNode);
  Register codegen(const AST::VariableExpr &VNode);
  Register codegen(const AST::BinaryExpr &BNode);
  Register codegen(const AST::CallExpr &CNode);
  Register codegen(const AST::Expr &ENode);
  void codegen(const AST::Statement &SNode);
  void codegen(const AST::IfStatement &SNode);
  void codegen(const AST::ReturnStatement& SNode);
  Register codegen(const AST::LocalVarDecl& SNode);
  Register codegen(const AST::AssignmentStatement& SNode);
  void codegen(const AST::WhileStatement &SNode);
  void codegen(const AST::Prototype &PNode);
  void codegen(const AST::Function &FNode);
  void codegen();

  [[noreturn]] void logError(std::string_view ErrMsg);
  std::unique_ptr<RegisterTracker> Tracker;
  std::unique_ptr<Module> TheModule;
  std::unique_ptr<AST::Module> ModuleTree;
  std::unique_ptr<Builder> TheBuilder;

  CodeGen(std::unique_ptr<AST::Module> ModuleTree);
  BasicBlock* EndBB = nullptr;

public:
  static std::string generateAsm(std::unique_ptr<AST::Module> AST);
};

} // namespace X86