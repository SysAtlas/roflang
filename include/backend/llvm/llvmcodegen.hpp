#pragma once

#include <frontend/ast.hpp>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <unordered_map>

namespace llvm {
class Module;
class LLVMContext;
class Value;
}; // namespace llvm

class LLVMCodeGen {
private:
  struct FunctionInfo {
    llvm::Function* TheFunction;
  };

  llvm::BasicBlock* EndBB = nullptr;
  llvm::AllocaInst* RetSlot = nullptr;

  llvm::LLVMContext &TheContext;
  llvm::Function* TheFunction;
  UP<llvm::Module> TheModule;
  UP<llvm::IRBuilder<>> Builder;
  std::unordered_map<std::string, llvm::AllocaInst *> NamedValues;
  UP<AST::Module> ModuleTree;

  llvm::Value *codegen(const AST::NumberExpr &NNode);
  llvm::Value *codegen(const AST::VariableExpr &VNode);
  llvm::Value *codegen(const AST::BinaryExpr &BNode);
  llvm::Value *codegen(const AST::CallExpr &CNode);
  llvm::Value *codegen(const AST::Expr &ENode);
  void codegen(const AST::Statement &SNode);
  void codegen(const AST::IfStatement &SNode);
  void codegen(const AST::ReturnStatement& SNode);
  void codegen(const AST::LocalVarDeclStmt& SNode);
  void codegen(const AST::AssignmentStatement& SNode);
  void codegen(const AST::WhileStatement &SNode);

  llvm::Function *codegen(const AST::Function::Signature &PNode);
  llvm::Function *codegen(const AST::Function &FNode);
  void codegen();

  LLVMCodeGen(UP<AST::Module> AST, llvm::LLVMContext &Context);

public:
  static UP<llvm::Module> generate(UP<AST::Module> AST,
                                                llvm::LLVMContext &Context);
};
