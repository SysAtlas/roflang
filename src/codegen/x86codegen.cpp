#include <codegen/x86codegen.hpp>
#include <memory>
#include <optional>
#include <target/x86/builder.hpp>
#include <target/x86/instructions.hpp>
#include <target/x86/operands.hpp>
#include <target/x86/registers.hpp>
#include <target/x86/x86.hpp>

namespace X86 {

void CodeGen::logError(std::string_view ErrMsg) {
  std::cerr << ErrMsg << '\n';
  exit(1);
}

class RegisterTracker {
private:
  struct RegisterInfo {
    const Register *Reg;
    bool IsOccupied = false;
    std::optional<std::string> ValueName = std::nullopt;

    RegisterInfo(const Register *Reg) : Reg{Reg} {}
  };

  std::array<RegisterInfo, 14> RegInfos = {
    RegisterInfo{&RF::get<RAX>()},
    RegisterInfo{&RF::get<RBX>()},
    RegisterInfo{&RF::get<RCX>()},
    RegisterInfo{&RF::get<RDX>()},
    RegisterInfo{&RF::get<R8>()},
    RegisterInfo{&RF::get<R9>()},
    RegisterInfo{&RF::get<R10>()},
    RegisterInfo{&RF::get<R11>()},
    RegisterInfo{&RF::get<R12>()},
    RegisterInfo{&RF::get<R13>()},
    RegisterInfo{&RF::get<R14>()},
    RegisterInfo{&RF::get<R15>()},
    RegisterInfo{&RF::get<RSI>()},
    RegisterInfo{&RF::get<RDI>()},
  };

public:
  // Occupy a free register cell, optionally with an identifier of name Name
  // and return a pointer to the occupied register
  template <u32 Size>
  const GPRegister<Size> &
  occupyFreeGPReg(std::optional<std::string_view> Name = std::nullopt) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (!RegInfo.IsOccupied) {
        RegInfo.IsOccupied = true;
        RegInfo.ValueName = Name;
        return static_cast<const GPRegister<Size> &>(*RegInfo.Reg);
      }
    }
    std::cerr << "Register allocator must be improved! Goodbye!" << '\n';
    std::abort();
  }

  void freeOccupiedGPReg(const Register *Reg) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.IsOccupied && RegInfo.Reg == Reg) {
        RegInfo.IsOccupied = false;
        RegInfo.ValueName = std::nullopt;
        return;
      }
    }
    std::abort();
  }

  void freeOccupiedGPRegIfUnnamed(const Register &Reg) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.IsOccupied && RegInfo.Reg == &Reg && !RegInfo.ValueName) {
        RegInfo.IsOccupied = false;
        RegInfo.ValueName = std::nullopt;
        return;
      }
    }
  }

  // In case no register holds the specified name, return nullptr
  const GPRegister<64> &findRegisterHoldingValue(std::string_view Name) const {
    for (const RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.ValueName && *RegInfo.ValueName == Name) {
        return *static_cast<const GPRegister<64> *>(RegInfo.Reg);
      }
    }
    std::cerr << "Trying to access an unassigned value! Semantic analysis must "
                 "be improved! Goodbye!"
              << '\n';
    std::abort();
  }
};

const GPRegister<64> &CodeGen::codegen(const AST::NumberExpr &NNode) {
  const GPRegister<64> &Reg = Tracker->occupyFreeGPReg<64>();
  TheBuilder->insertInstruction(MovInstruction(Reg, Immediate::get(NNode.Val)));
  return Reg;
}

const GPRegister<64> &CodeGen::codegen(const AST::VariableExpr &VNode) {
  const GPRegister<64> &Res = Tracker->findRegisterHoldingValue(VNode.Name);
  return Res;
}

const GPRegister<64> &CodeGen::codegen(const AST::BinaryExpr &BNode) {
  const GPRegister<64> &L = codegen(BNode.LHS);
  const GPRegister<64> &R = codegen(BNode.RHS);

  const GPRegister<64> *Res = nullptr;
  switch (BNode.Op) {
  case BinOpType::Add: {
    Res = &Tracker->occupyFreeGPReg<64>();
    TheBuilder->insertInstruction(PushInstruction{RF::get<RAX>()});
    TheBuilder->insertInstruction(MovInstruction{RF::get<RAX>(), L});
    TheBuilder->insertInstruction(AddInstruction{RF::get<RAX>(), R});
    TheBuilder->insertInstruction(MovInstruction{*Res, RF::get<RAX>()});
    TheBuilder->insertInstruction(PopInstruction{RF::get<RAX>()});
    break;
  }
  case BinOpType::Sub: {
    Res = &Tracker->occupyFreeGPReg<64>();
    TheBuilder->insertInstruction(PushInstruction{RF::get<RAX>()});
    TheBuilder->insertInstruction(MovInstruction{RF::get<RAX>(), L});
    TheBuilder->insertInstruction(SubInstruction{RF::get<RAX>(), R});
    TheBuilder->insertInstruction(MovInstruction{*Res, RF::get<RAX>()});
    TheBuilder->insertInstruction(PopInstruction{RF::get<RAX>()});
    break;
  }
  case BinOpType::Mul: {
    break;
  }
  // case BinOpType::Div:
  //   return TheBuilder->CreateSDiv(L, R);
  // case BinOpType::Lt:
  //   L = TheBuilder->CreateICmpSLT(L, R);
  //   return TheBuilder->CreateTrunc(L, Type::getInt1Ty(TheContext),
  //   "booltmp");
  // case BinOpType::Leq:
  //   L = Builder->CreateICmpSLE(L, R, "cmp");
  //   return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  // case BinOpType::Gt:
  //   L = Builder->CreateICmpSGT(L, R, "cmp");
  //   return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  // case BinOpType::Geq:
  //   L = Builder->CreateICmpSGE(L, R, "cmp");
  //   return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  // case BinOpType::Neq:
  //   L = Builder->CreateICmpNE(L, R, "cmp");
  //   return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  // case BinOpType::Eq:
  //   L = Builder->CreateICmpEQ(L, R, "cmp");
  //   return Builder->CreateTrunc(L, Type::getInt1Ty(TheContext), "booltmp");
  default:
    logError("invalid binary operator");
  }
  Tracker->freeOccupiedGPRegIfUnnamed(L);
  Tracker->freeOccupiedGPRegIfUnnamed(R);
  return *Res;
}

const GPRegister<64> &CodeGen::codegen(const AST::CallExpr &CNode) {}

const GPRegister<64> &CodeGen::codegen(const AST::Expr &ENode) {
  return std::visit<const GPRegister<64> &>(
    overloaded{[this](const auto &Arg) -> const GPRegister<64>& {
      return codegen(*Arg);
    }},
    ENode);
}

void CodeGen::codegen(const AST::Statement &SNode) {
  std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, SNode);
}

void CodeGen::codegen(const AST::IfStatement &SNode) {}

void CodeGen::codegen(const AST::ReturnStatement &SNode) {
  if (SNode.Value.has_value()) {
    const GPRegister<64> &ToReturn = codegen(*SNode.Value);
    TheBuilder->insertInstruction(
      MovInstruction{RF::getReturnRegister(), ToReturn});
  }
  TheBuilder->insertInstruction(RetInstruction{});
}

void CodeGen::codegen(const AST::LocalVarDecl &SNode) {}
void CodeGen::codegen(const AST::AssignmentStatement &SNode) {}
void CodeGen::codegen(const AST::WhileStatement &SNode) {}

void CodeGen::codegen(const AST::Prototype &PNode) {
  TheBuilder->addFunction(Function(
    PNode.Name, PNode.IsExtern ? LinkageType::Extern : LinkageType::Internal));
}

void CodeGen::codegen(const AST::Function &FNode) {
  codegen(*FNode.Proto);
  for (const auto &S : FNode.Body) {
    codegen(S);
  }
}

void CodeGen::codegen() {
  TheModule = std::make_unique<Module>();
  TheBuilder = std::make_unique<Builder>(TheModule.get());

  for (const auto &TLI : ModuleTree->TopLevelItems) {
    std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, TLI);
  }
}

CodeGen::CodeGen(std::unique_ptr<AST::Module> ModuleTree)
    : Tracker{std::make_unique<RegisterTracker>()},
      ModuleTree{std::move(ModuleTree)} {}

std::string CodeGen::generateAsm(std::unique_ptr<AST::Module> AST) {
  CodeGen G{std::move(AST)};
  G.codegen();
  return G.TheModule->toString();
}

} // namespace X86