#include "lexer.hpp"
#include <ast.hpp>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvmcodegen.hpp>
#include <memory>
#include <variant>

using namespace llvm;

llvm::Value *LogErrorV(const char *Str) {
  std::cerr << Str << std::endl;
  abort();
  return nullptr;
}

LLVMCodeGen::LLVMCodeGen(std::unique_ptr<AST::Module> ModuleTree,
                         LLVMContext &Context)
    : TheContext(Context),
      TheModule(std::make_unique<Module>("my cool jit", TheContext)),
      Builder(std::make_unique<IRBuilder<>>(TheContext)),
      ModuleTree{std::move(ModuleTree)} {}

std::unique_ptr<Module> LLVMCodeGen::generate(std::unique_ptr<AST::Module> AST,
                                              llvm::LLVMContext &Context) {
  LLVMCodeGen C{std::move(AST), Context};
  C.codegen();
  return std::move(C.TheModule);
}

Value *LLVMCodeGen::codegen(const AST::NumberExpr &NNode) {
  return ConstantInt::get(Type::getInt64Ty(TheContext), NNode.Val);
}

Value *LLVMCodeGen::codegen(const AST::VariableExpr &VNode) {
  // Look this variable up in the function.
  Value *V = NamedValues[VNode.Name];
  if (!V) {
    return LogErrorV("Unknown variable name");
  }
  return V;
}

Value *LLVMCodeGen::codegen(const AST::BinaryExpr &BNode) {
  Value *L = codegen(BNode.LHS);
  Value *R = codegen(BNode.RHS);
  if (!L || !R) {
    return nullptr;
  }

  switch (BNode.Op) {
  case BinOpType::Add:
    return Builder->CreateAdd(L, R, "addtmp");
  case BinOpType::Sub:
    return Builder->CreateSub(L, R, "subtmp");
  case BinOpType::Mul:
    return Builder->CreateMul(L, R, "multmp");
  case BinOpType::Div:
    return Builder->CreateSDiv(L, R, "divtmp");
  case BinOpType::Lt:
    L = Builder->CreateICmpULT(L, R, "cmptmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Value *LLVMCodeGen::codegen(const AST::CallExpr &CNode) {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(CNode.Callee);
  if (!CalleeF) {
    return LogErrorV("Unknown function referenced");
  }

  // If argument mismatch error.
  if (CalleeF->arg_size() != CNode.Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = CNode.Args.size(); i != e; ++i) {
    ArgsV.push_back(codegen(CNode.Args[i]));
    if (!ArgsV.back()) {
      return nullptr;
    }
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *LLVMCodeGen::codegen(const AST::Expr &ENode) {
  return std::visit<Value *>(
    overloaded{[this](const auto &Arg) { return codegen(*Arg); }}, ENode);
}

void LLVMCodeGen::codegen(const AST::IfStatement &SNode) {
  Value *Condition = codegen(SNode.Condition);
  BasicBlock *IfBody = BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  BasicBlock *AfterIf =
    BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  Builder->CreateCondBr(Condition, IfBody, AfterIf);
  Builder->SetInsertPoint(IfBody);
  for (const auto &S : SNode.Body) {
    codegen(S);
  }
  Builder->CreateBr(AfterIf);
  Builder->SetInsertPoint(AfterIf);
}

void LLVMCodeGen::codegen(const AST::ReturnStatement &SNode) {
  if (SNode.Value) {
    Value *RetVal = codegen(*SNode.Value);
    Builder->CreateStore(RetVal, RetSlot, false);
  }
  Builder->CreateBr(EndBB);
}

void LLVMCodeGen::codegen(const AST::Statement &SNode) {
  std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, SNode);
}

Function *LLVMCodeGen::codegen(const AST::Prototype &PNode) {
  std::vector<Type *> Ints(PNode.Args.size(), Type::getInt64Ty(TheContext));

  Type *RetType = PNode.ReturnType == RLType::Void
                    ? Type::getVoidTy(TheContext)
                    : Type::getInt64Ty(TheContext);
  FunctionType *FT = FunctionType::get(RetType, Ints, false);

  Function *F = Function::Create(
    FT, Function::ExternalLinkage, PNode.Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(PNode.Args[Idx++]);
  }

  return F;
}

Function *LLVMCodeGen::codegen(const AST::Function &FNode) {
  // First, check for an existing function from a previous 'extern' declaration.
  TheFunction = TheModule->getFunction(FNode.Proto->Name);

  if (!TheFunction) {
    TheFunction = codegen(*FNode.Proto);
  }

  if (!TheFunction) {
    return nullptr;
  }

  if (!TheFunction->empty()) {
    return (Function *)LogErrorV("Function cannot be redefined.");
  }

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);

  // Create end basic block with ret where everything will converge
  EndBB = BasicBlock::Create(TheContext, "end", TheFunction);

  Builder->SetInsertPoint(BB);

  // Set up end block machinery for return.
  if (Type *RetType = TheFunction->getReturnType();
      RetType != Type::getVoidTy(TheContext)) {
    RetSlot = Builder->CreateAlloca(RetType);
    Builder->SetInsertPoint(EndBB);
    LoadInst *Ld = Builder->CreateLoad(RetType, RetSlot, false, "ld");
    Builder->CreateRet(Ld);
  } else {
    Builder->SetInsertPoint(EndBB);
    Builder->CreateRetVoid();
  }

  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args()) {
    NamedValues[std::string(Arg.getName())] = &Arg;
  }

  for (const auto &S : FNode.Body) {
    codegen(S);
  }

  verifyFunction(*TheFunction);

  return TheFunction;
}

Module *LLVMCodeGen::codegen() {
  for (const AST::Module::TopLevelItem &TLI : ModuleTree->TopLevelItems) {
    Function *X = std::visit(
      overloaded{
        [this](auto &arg) { return codegen(*arg); },
      },
      TLI);
  }
  return TheModule.get();
}
