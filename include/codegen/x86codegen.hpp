#pragma once

#include <target/x86/x86.hpp>
#include <frontend/ast.hpp>
#include <memory>

namespace X86 {

class RegisterTracker;

class CodeGen {
private:
  const GPRegister<64>& codegen(const AST::NumberExpr &NNode);
  const GPRegister<64>& codegen(const AST::VariableExpr &VNode);
  const GPRegister<64>& codegen(const AST::BinaryExpr &BNode);
  const GPRegister<64>& codegen(const AST::CallExpr &CNode);
  const GPRegister<64>& codegen(const AST::Expr &ENode);
  void codegen(const AST::Statement &SNode);
  void codegen(const AST::IfStatement &SNode);
  void codegen(const AST::ReturnStatement& SNode);
  void codegen(const AST::LocalVarDecl& SNode);
  void codegen(const AST::AssignmentStatement& SNode);
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

public:
  static std::string generateAsm(std::unique_ptr<AST::Module> AST);
};

} // namespace X86