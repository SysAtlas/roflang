#include <common.hpp>
#include <frontend/lexer.hpp>
#include <frontend/ast.hpp>
#include <iostream>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <codegen/llvmcodegen.hpp>
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
      TheModule(std::make_unique<Module>("roflang", TheContext)),
      Builder(std::make_unique<IRBuilder<>>(TheContext)),
      ModuleTree{std::move(ModuleTree)} {}

std::unique_ptr<Module> LLVMCodeGen::generate(std::unique_ptr<AST::Module> AST,
                                              llvm::LLVMContext &Context) {
  LLVMCodeGen C{std::move(AST), Context};
  C.codegen();
  return std::move(C.TheModule);
}

Value *LLVMCodeGen::codegen(const AST::NumberExpr &NNode) {
  return ConstantInt::get(Type::getInt32Ty(TheContext), NNode.Val);
}

Value *LLVMCodeGen::codegen(const AST::VariableExpr &VNode) {
  // Look this variable up in the function.
  AllocaInst *V = NamedValues[VNode.Name];
  if (!V) {
    return LogErrorV("Unknown variable name");
  }
  return Builder->CreateLoad(
    V->getAllocatedType(), V, std::format("{}_ld", VNode.Name));
}

Value *LLVMCodeGen::codegen(const AST::BinaryExpr &BNode) {
  Value *L = codegen(BNode.LHS);
  Value *R = codegen(BNode.RHS);
  if (!L || !R) {
    return nullptr;
  }

  switch (BNode.Op) {
  case BinOpType::Add:
    return Builder->CreateAdd(L, R, "add");
  case BinOpType::Sub:
    return Builder->CreateSub(L, R, "sub");
  case BinOpType::Mul:
    return Builder->CreateMul(L, R, "mul");
  case BinOpType::Div:
    return Builder->CreateSDiv(L, R, "div");
  case BinOpType::Mod:
    return Builder->CreateURem(L, R, "mod");
  case BinOpType::Lt:
    L = Builder->CreateICmpSLT(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOpType::Leq:
    L = Builder->CreateICmpSLE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOpType::Gt:
    L = Builder->CreateICmpSGT(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOpType::Geq:
    L = Builder->CreateICmpSGE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOpType::Neq:
    L = Builder->CreateICmpNE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOpType::Eq:
    L = Builder->CreateICmpEQ(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Value *LLVMCodeGen::codegen(const AST::CallExpr &CNode) {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(CNode.Callee);

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = CNode.Args.size(); i != e; ++i) {
    ArgsV.push_back(codegen(CNode.Args[i]));
  }

  return Builder->CreateCall(
    CalleeF,
    ArgsV,
    CalleeF->getReturnType() == Type::getVoidTy(TheContext) ? "" : "calltmp");
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

void LLVMCodeGen::codegen(const AST::LocalVarDecl &SNode) {
  AllocaInst *NewSlot = Builder->CreateAlloca(
    Type::getIntNTy(TheContext, SNode.TypeInfo->SizeInBits), nullptr, std::format("{}_st", SNode.Name));
  Builder->CreateStore(codegen(SNode.Value), NewSlot);
  NamedValues[SNode.Name] = NewSlot;
}

void LLVMCodeGen::codegen(const AST::AssignmentStatement &SNode) {
  Value *RHS = codegen(SNode.Value);
  Builder->CreateStore(RHS, NamedValues[SNode.Name]);
}

void LLVMCodeGen::codegen(const AST::WhileStatement &SNode) {
  BasicBlock *CondBlock =
    BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  BasicBlock *WhileBody =
    BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  BasicBlock *AfterWhile =
    BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  Builder->CreateBr(CondBlock);
  Builder->SetInsertPoint(CondBlock);
  Value *Condition = codegen(SNode.Cond);
  Builder->CreateCondBr(Condition, WhileBody, AfterWhile);
  Builder->SetInsertPoint(WhileBody);
  for (const auto &S : SNode.Body) {
    codegen(S);
  }
  Builder->CreateBr(CondBlock);
  Builder->SetInsertPoint(AfterWhile);
}

void LLVMCodeGen::codegen(const AST::Statement &SNode) {
  std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, SNode);
}

Function *LLVMCodeGen::codegen(const AST::Prototype &PNode) {
  std::vector<Type *> Params;
  for (const auto& Arg : PNode.Args) {
    Params.push_back(Type::getIntNTy(TheContext, Arg.TypeInfo->SizeInBits));
  }

  Type *RetType =
    PNode.ReturnTypeInfo->Type == RLType::Void
      ? Type::getVoidTy(TheContext)
      : Type::getIntNTy(TheContext, PNode.ReturnTypeInfo->SizeInBits);
  FunctionType *FT = FunctionType::get(RetType, Params, false);

  Function *F = Function::Create(
    FT, Function::ExternalLinkage, PNode.Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (llvm::Argument &Arg : F->args()) {
    const auto& NameOpt = PNode.Args[Idx++].Name;
    Arg.setName(NameOpt ? *NameOpt : "arg");
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
    RetSlot = Builder->CreateAlloca(RetType, nullptr, "retslot");
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
  for (llvm::Argument &Arg : TheFunction->args()) {
    AllocaInst *ArgSlot = Builder->CreateAlloca(Arg.getType());
    Builder->CreateStore(&Arg, ArgSlot);
    NamedValues[std::string(Arg.getName())] = ArgSlot;
  }

  for (const auto &S : FNode.Body) {
    codegen(S);
  }

  Builder->CreateBr(EndBB);

  verifyFunction(*TheFunction);

  return TheFunction;
}

void LLVMCodeGen::codegen() {
  for (const AST::Module::TopLevelItem &TLI : ModuleTree->TopLevelItems) {
    std::visit(
      overloaded{
        [this](auto &arg) { return codegen(*arg); },
      },
      TLI);
  }
}
