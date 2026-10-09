#include <backend/llvm/llvmcodegen.hpp>

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
#include <memory>
#include <variant>

using namespace llvm;

llvm::Value *LogErrorV(const char *Str) {
  std::cerr << Str << std::endl;
  abort();
  return nullptr;
}

LLVMCodeGen::LLVMCodeGen(std::unique_ptr<AST::Module> module_tree,
                         LLVMContext &context)
    : TheContext(context),
      TheModule(std::make_unique<Module>("roflang", TheContext)),
      Builder(std::make_unique<IRBuilder<>>(TheContext)),
      ModuleTree{std::move(module_tree)} {}

std::unique_ptr<Module> LLVMCodeGen::generate(std::unique_ptr<AST::Module> AST,
                                              llvm::LLVMContext &Context) {
  LLVMCodeGen C{std::move(AST), Context};
  C.codegen();
  return std::move(C.TheModule);
}

Value *LLVMCodeGen::codegen(const AST::NumberExpr &NNode) {
  return ConstantInt::get(Type::getInt32Ty(TheContext), NNode.value);
}

Value *LLVMCodeGen::codegen(const AST::VariableExpr &VNode) {
  // Look this variable up in the function.
  AllocaInst *V = NamedValues[VNode.name];
  if (!V) {
    return LogErrorV("Unknown variable name");
  }
  return Builder->CreateLoad(
    V->getAllocatedType(), V, std::format("{}_ld", VNode.name));
}

Value *LLVMCodeGen::codegen(const AST::BinaryExpr &BNode) {
  Value *L = codegen(BNode.lhs);
  Value *R = codegen(BNode.rhs);
  if (!L || !R) {
    return nullptr;
  }

  switch (BNode.op) {
  case BinOp::ADD:
    return Builder->CreateAdd(L, R, "add");
  case BinOp::SUB:
    return Builder->CreateSub(L, R, "sub");
  case BinOp::MUL:
    return Builder->CreateMul(L, R, "mul");
  case BinOp::DIV:
    return Builder->CreateSDiv(L, R, "div");
  case BinOp::MOD:
    return Builder->CreateURem(L, R, "mod");
  case BinOp::LT:
    L = Builder->CreateICmpSLT(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOp::LEQ:
    L = Builder->CreateICmpSLE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOp::GT:
    L = Builder->CreateICmpSGT(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOp::GEQ:
    L = Builder->CreateICmpSGE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOp::NEQ:
    L = Builder->CreateICmpNE(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  case BinOp::EQ:
    L = Builder->CreateICmpEQ(L, R, "cmp");
    return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Value *LLVMCodeGen::codegen(const AST::CallExpr &CNode) {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(CNode.callee_name);

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = CNode.args.size(); i != e; ++i) {
    ArgsV.push_back(codegen(CNode.args[i]));
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
  Value *Condition = codegen(SNode.condition);
  BasicBlock *IfBody = BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  BasicBlock *AfterIf =
    BasicBlock::Create(TheContext, "bb", TheFunction, EndBB);
  Builder->CreateCondBr(Condition, IfBody, AfterIf);
  Builder->SetInsertPoint(IfBody);
  for (const auto &S : SNode.body) {
    codegen(S);
  }
  Builder->CreateBr(AfterIf);
  Builder->SetInsertPoint(AfterIf);
}

void LLVMCodeGen::codegen(const AST::ReturnStatement &SNode) {
  if (SNode.value) {
    Value *RetVal = codegen(*SNode.value);
    Builder->CreateStore(RetVal, RetSlot, false);
  }
  Builder->CreateBr(EndBB);
}

void LLVMCodeGen::codegen(const AST::LocalVarDeclStmt &SNode) {
  AllocaInst *NewSlot = Builder->CreateAlloca(
    Type::getIntNTy(TheContext, SNode.type_info->size_in_bits), nullptr, std::format("{}_st", SNode.name));
  Builder->CreateStore(codegen(SNode.value), NewSlot);
  NamedValues[SNode.name] = NewSlot;
}

void LLVMCodeGen::codegen(const AST::AssignmentStatement &SNode) {
  Value *RHS = codegen(SNode.value);
  Builder->CreateStore(RHS, NamedValues[SNode.name]);
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
  Value *Condition = codegen(SNode.cond);
  Builder->CreateCondBr(Condition, WhileBody, AfterWhile);
  Builder->SetInsertPoint(WhileBody);
  for (const auto &S : SNode.body) {
    codegen(S);
  }
  Builder->CreateBr(CondBlock);
  Builder->SetInsertPoint(AfterWhile);
}

void LLVMCodeGen::codegen(const AST::Statement &SNode) {
  std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, SNode);
}

Function *LLVMCodeGen::codegen(const AST::Signature &PNode) {
  std::vector<Type *> Params;
  for (const auto& Arg : PNode.args) {
    Params.push_back(Type::getIntNTy(TheContext, Arg.type_info->size_in_bits));
  }

  Type *RetType =
    PNode.return_type_info->type == RLType::VOID
      ? Type::getVoidTy(TheContext)
      : Type::getIntNTy(TheContext, PNode.return_type_info->size_in_bits);
  FunctionType *FT = FunctionType::get(RetType, Params, false);

  Function *F = Function::Create(
    FT, Function::ExternalLinkage, PNode.name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (llvm::Argument &Arg : F->args()) {
    const auto& NameOpt = PNode.args[Idx++].name;
    Arg.setName(NameOpt ? *NameOpt : "arg");
  }

  return F;
}

Function *LLVMCodeGen::codegen(const AST::Function &FNode) {
  // First, check for an existing function from a previous 'extern' declaration.
  TheFunction = TheModule->getFunction(FNode.proto->name);

  if (!TheFunction) {
    TheFunction = codegen(*FNode.proto);
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

  for (const auto &S : FNode.body) {
    codegen(S);
  }

  Builder->CreateBr(EndBB);

  verifyFunction(*TheFunction);

  return TheFunction;
}

void LLVMCodeGen::codegen() {
  for (const AST::Module::TopLevelItem &TLI : ModuleTree->top_level_items) {
    std::visit(
      overloaded{
        [this](auto &arg) { return codegen(*arg); },
      },
      TLI);
  }
}
