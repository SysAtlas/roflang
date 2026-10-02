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
  std::unordered_map<std::string, llvm::Value *> NamedValues;
  std::unique_ptr<ModuleAST> ModuleTree;

  llvm::Value *codegen(const NumberExpr &NNode);
  llvm::Value *codegen(const VariableExpr &VNode);
  llvm::Value *codegen(const BinaryExpr &BNode);
  llvm::Value *codegen(const CallExpr &CNode);
  llvm::Value *codegen(const Expr &ENode);
  void codegen(const Statement &SNode);
  void codegen(const IfStatement &SNode);
  void codegen(const ReturnStatement& SNode);

  llvm::Function *codegen(const PrototypeAST &PNode);
  llvm::Function *codegen(const FunctionAST &FNode);
  llvm::Module *codegen();

  LLVMCodeGen(std::unique_ptr<ModuleAST> AST, llvm::LLVMContext &Context);

public:
  static std::unique_ptr<llvm::Module> generate(std::unique_ptr<ModuleAST> AST,
                                                llvm::LLVMContext &Context);
};
