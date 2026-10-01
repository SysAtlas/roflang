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

class CodeGenerator {
private:
  llvm::LLVMContext &TheContext;
  std::unique_ptr<llvm::Module> TheModule;
  std::unique_ptr<llvm::IRBuilder<>> Builder;
  std::unordered_map<std::string, llvm::Value *> NamedValues;
  std::unique_ptr<ModuleAST> ModuleTree;

  llvm::Value *codegen(const NumberExprAST &NNode);
  llvm::Value *codegen(const VariableExprAST &VNode);
  llvm::Value *codegen(const BinaryExprAST &BNode);
  llvm::Value *codegen(const CallExprAST &CNode);
  llvm::Value *codegen(const ExprASTWrapper &ENode);

  llvm::Function *codegen(const PrototypeAST &PNode);
  llvm::Function *codegen(const FunctionAST &FNode);
  llvm::Module *codegen();

  CodeGenerator(std::unique_ptr<ModuleAST> AST, llvm::LLVMContext &Context);

public:
  static std::unique_ptr<llvm::Module> generate(std::unique_ptr<ModuleAST> AST,
                                                llvm::LLVMContext &Context);
};