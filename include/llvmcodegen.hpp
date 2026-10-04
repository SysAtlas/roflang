#pragma once

#include <ast.hpp>
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
  std::unique_ptr<llvm::Module> TheModule;
  std::unique_ptr<llvm::IRBuilder<>> Builder;
  std::unordered_map<std::string, llvm::AllocaInst *> NamedValues;
  std::unique_ptr<AST::Module> ModuleTree;

  llvm::Value *codegen(const AST::NumberExpr &NNode);
  llvm::Value *codegen(const AST::VariableExpr &VNode);
  llvm::Value *codegen(const AST::BinaryExpr &BNode);
  llvm::Value *codegen(const AST::CallExpr &CNode);
  llvm::Value *codegen(const AST::Expr &ENode);
  void codegen(const AST::Statement &SNode);
  void codegen(const AST::IfStatement &SNode);
  void codegen(const AST::ReturnStatement& SNode);
  void codegen(const AST::LocalVarDecl& SNode);
  void codegen(const AST::AssignmentStatement& SNode);
  void codegen(const AST::WhileStatement &SNode);

  llvm::Function *codegen(const AST::Prototype &PNode);
  llvm::Function *codegen(const AST::Function &FNode);
  llvm::Module *codegen();

  LLVMCodeGen(std::unique_ptr<AST::Module> AST, llvm::LLVMContext &Context);

public:
  static std::unique_ptr<llvm::Module> generate(std::unique_ptr<AST::Module> AST,
                                                llvm::LLVMContext &Context);
};
