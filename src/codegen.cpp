#include <ast.hpp>
#include <codegen.hpp>
#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <memory>
#include <variant>

using namespace llvm;

Value *LogErrorV(const char *Str) {
  std::cerr << Str << std::endl;
  abort();
  return nullptr;
}

CodeGenerator::CodeGenerator(std::unique_ptr<ModuleAST> ModuleTree,
                             LLVMContext &Context)
    : TheContext(Context),
      TheModule(std::make_unique<Module>("my cool jit", TheContext)),
      Builder(std::make_unique<IRBuilder<>>(TheContext)),
      ModuleTree{std::move(ModuleTree)} {}

std::unique_ptr<Module> CodeGenerator::generate(std::unique_ptr<ModuleAST> AST, llvm::LLVMContext & Context) {
  CodeGenerator C{std::move(AST), Context};
  C.codegen();
  return std::move(C.TheModule);
}

Value *CodeGenerator::codegen(const NumberExprAST &NNode) {
  return ConstantFP::get(TheContext, APFloat(NNode.Val));
}

Value *CodeGenerator::codegen(const VariableExprAST &VNode) {
  // Look this variable up in the function.
  Value *V = NamedValues[VNode.Name];
  if (!V) {
    return LogErrorV("Unknown variable name");
  }
  return V;
}

Value *CodeGenerator::codegen(const BinaryExprAST &BNode) {
  Value *L = codegen(*BNode.LHS);
  Value *R = codegen(*BNode.RHS);
  if (!L || !R)
    return nullptr;

  switch (BNode.Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '/':
    return Builder->CreateFDiv(L, R, "divtmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}

Value *CodeGenerator::codegen(const CallExprAST &CNode) {
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
    ArgsV.push_back(codegen(*CNode.Args[i]));
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *CodeGenerator::codegen(const ExprASTWrapper &ENode) {
  return std::visit(
    overloaded{[this](const auto &Arg) { return codegen(*Arg); }}, ENode.Value);
}

Function *CodeGenerator::codegen(const PrototypeAST &PNode) {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(PNode.Args.size(), Type::getDoubleTy(TheContext));
  FunctionType *FT =
    FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

  Function *F = Function::Create(
    FT, Function::ExternalLinkage, PNode.Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(PNode.Args[Idx++]);
  }

  return F;
}

Function *CodeGenerator::codegen(const FunctionAST &FNode) {
  // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = TheModule->getFunction(FNode.Proto->getName());

  if (!TheFunction) {
    TheFunction = codegen(*FNode.Proto);
  }

  if (!TheFunction)
    return nullptr;

  if (!TheFunction->empty())
    return (Function *)LogErrorV("Function cannot be redefined.");

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : TheFunction->args())
    NamedValues[std::string(Arg.getName())] = &Arg;

  if (Value *RetVal = codegen(*FNode.Body)) {
    // Finish off the function.
    Builder->CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}

Module *CodeGenerator::codegen() {
  for (const TopLevelItem &TLI : ModuleTree->TopLevelItems) {
    Function *X = std::visit(
      overloaded{
        [this](auto &arg) { return codegen(*arg); },
      },
      TLI);
  }
  return TheModule.get();
}
