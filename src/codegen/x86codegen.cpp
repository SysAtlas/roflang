#include <codegen/x86codegen.hpp>
#include <common.hpp>
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
    Register Reg;
    bool IsOccupied = false;
    std::optional<std::string> ValueName = std::nullopt;

    RegisterInfo(Register Reg) : Reg{Reg} {}
  };

  std::array<RegisterInfo, 14> RegInfos = {
    RegisterInfo{RAX()},
    RegisterInfo{RBX()},
    RegisterInfo{RCX()},
    RegisterInfo{RDX()},
    RegisterInfo{R8()},
    RegisterInfo{R9()},
    RegisterInfo{R10()},
    RegisterInfo{R11()},
    RegisterInfo{R12()},
    RegisterInfo{R13()},
    RegisterInfo{R14()},
    RegisterInfo{R15()},
    RegisterInfo{RDI()},
    RegisterInfo{RSI()},
  };

public:
  // Occupy a free register cell, optionally with an identifier of name Name
  // and return a pointer to the occupied register
  Register
  occupyFreeGPReg(std::optional<std::string_view> Name = std::nullopt) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (!RegInfo.IsOccupied) {
        RegInfo.IsOccupied = true;
        RegInfo.ValueName = Name;
        return RegInfo.Reg;
      }
    }
    std::cerr << "Register allocator must be improved! Goodbye!" << '\n';
    std::abort();
  }

  void freeOccupiedGPReg(const Register &Reg) {
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
      if (RegInfo.IsOccupied && RegInfo.Reg == Reg && !RegInfo.ValueName) {
        RegInfo.IsOccupied = false;
        RegInfo.ValueName = std::nullopt;
        return;
      }
    }
  }

  std::string_view getVarName(const Register &Reg) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.Reg == Reg) {
        assert(RegInfo.IsOccupied || !RegInfo.ValueName.has_value());
        return *RegInfo.ValueName;
      }
    }
    std::cerr << "error" << '\n';
    abort();
  }

  bool holdsVar(const Register &Reg) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.Reg == Reg) {
        assert(RegInfo.IsOccupied || !RegInfo.ValueName.has_value());
        return RegInfo.ValueName.has_value();
      }
    }
    std::cerr << "Bad access.." << '\n';
    abort();
  }

  // Reassigns a slot that belongs to an unnamed value to a named variable
  void assignToVar(const Register &Reg, std::string_view Name) {
    for (RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.Reg == Reg) {
        if (RegInfo.ValueName) {
          std::cerr << "Register already occupied by another variable" << '\n';
          abort();
        }
        RegInfo.ValueName = Name;
        return;
      }
    }
    abort();
  }

  // In case no register holds the specified name, return nullptr
  Register findRegisterHoldingValue(std::string_view Name) const {
    for (const RegisterInfo &RegInfo : RegInfos) {
      if (RegInfo.ValueName && *RegInfo.ValueName == Name) {
        return RegInfo.Reg;
      }
    }
    std::cerr << "Trying to access an unassigned value! Semantic analysis must "
                 "be improved! Goodbye!"
              << '\n';
    std::abort();
  }
};

Register CodeGen::codegen(const AST::NumberExpr &NNode) {
  Register Reg = Tracker->occupyFreeGPReg();
  TheBuilder->addInstruction(Mov{Reg, Immediate::get(NNode.Val)});
  return Reg;
}

Register CodeGen::codegen(const AST::VariableExpr &VNode) {
  Register Res = Tracker->findRegisterHoldingValue(VNode.Name);
  return Res;
}

void CodeGen::compareSetHelper(BinOpType Op, const Register &Dst) {
  switch (Op) {
  case BinOpType::Lt:
    TheBuilder->addInstruction(SetL{Dst});
    break;
  case BinOpType::Leq:
    TheBuilder->addInstruction(SetLE{Dst});
    break;
  case BinOpType::Gt:
    TheBuilder->addInstruction(SetG{Dst});
    break;
  case BinOpType::Geq:
    TheBuilder->addInstruction(SetGE{Dst});
    break;
  case BinOpType::Eq:
    TheBuilder->addInstruction(SetZ{Dst});
    break;
  case BinOpType::Neq:
    TheBuilder->addInstruction(SetNE{Dst});
    break;
  default:
    std::cerr << "Not a comparison binop!" << '\n';
    abort();
  }
  return;
}

void CodeGen::binOpHelper(BinOpType Op, const Register &L, const Register &R) {
  static const std::array CompOps = {BinOpType::Eq,
                                     BinOpType::Neq,
                                     BinOpType::Lt,
                                     BinOpType::Gt,
                                     BinOpType::Geq,
                                     BinOpType::Leq};

  if (Op == BinOpType::Add) {
    TheBuilder->addInstruction(Add{L, R});
  } else if (Op == BinOpType::Sub) {
    TheBuilder->addInstruction(Sub{L, R});
  } else if (Op == BinOpType::Mul) {
    TheBuilder->addInstruction(IMul{L, R});
  } else if (std::ranges::find(CompOps, Op) != CompOps.end()) {
    TheBuilder->addInstruction(Cmp{L, R});
    compareSetHelper(Op, L.getLo8());
    TheBuilder->addInstruction(MovSX{L, L.getLo8()});
  } else {
    logError("invalid binary operator");
    abort();
  }
}

Register CodeGen::codegen(const AST::BinaryExpr &BNode) {
  Register L = codegen(BNode.LHS);
  Register R = codegen(BNode.RHS);

  bool LHoldsVar = Tracker->holdsVar(L);
  bool RHoldsVar = Tracker->holdsVar(R);

  BinOpType Op = BNode.Op;

  if (Op == BinOpType::Div || Op == BinOpType::Mod) {
    abort();
  } else {
    if (!LHoldsVar) {
      binOpHelper(Op, L, R);
      Tracker->freeOccupiedGPRegIfUnnamed(R);
      return L;
    } else {
      TheBuilder->addInstruction(Push{L});
      binOpHelper(BNode.Op, L, R);
      const auto &NewReg = Tracker->occupyFreeGPReg();
      TheBuilder->addInstruction(Mov{NewReg, L});
      TheBuilder->addInstruction(Pop{L});
      Tracker->freeOccupiedGPRegIfUnnamed(L);
      Tracker->freeOccupiedGPRegIfUnnamed(R);
      return NewReg;
    }
  }
}

// Start very simple.
Register CodeGen::codegen(const AST::CallExpr &CNode) {
  // std::vector<std::pair<const Register *, std::string_view>> SavedNames;

  // for (usize I = 0; I < CNode.Args.size(); ++I) {
  //   if (Counter > RF::FunctionArguments.size()) {
  //     std::cerr << "Passing more than 6 arguments not supported! (for now.)"
  //     << '\n'; abort();
  //   }

  //   const GPRegister<64> &Res = codegen(Arg);
  //   ++Counter;
  // }
  // // Everything that is a variable is saved and will be restored afterwards
  // for (usize I = 0; I < CNode.Args.size(); ++I) {
  //   const Register &IthArgumentReg = *RF::FunctionArguments[I];
  //   if (Tracker->holdsVar(IthArgumentReg)) {
  //     TheBuilder->addInstruction(Push{IthArgumentReg});
  //     SavedNames.emplace_back(&IthArgumentReg,
  //     Tracker->getVarName(IthArgumentReg));
  //     Tracker->freeOccupiedGPReg(IthArgumentReg);
  //   }
  // }

  // // Restore state
  // for () {
  // }
}

Register CodeGen::codegen(const AST::Expr &ENode) {
  return std::visit<Register>(
    overloaded{[this](const auto &Arg) -> Register { return codegen(*Arg); }},
    ENode);
}

void CodeGen::codegen(const AST::Statement &SNode) {
  std::visit(overloaded{[this](const auto &Arg) { codegen(*Arg); }}, SNode);
}

void CodeGen::codegen(const AST::IfStatement &SNode) {
  const auto &CondRes = codegen(SNode.Condition);
  TheBuilder->addInstruction(Cmp{CondRes, Immediate::get(1)});
  BasicBlock &IfBody =
    TheBuilder->addBasicBlock(".L_ifbody", TheBuilder->CurBB);
  BasicBlock &AfterIf = TheBuilder->addBasicBlock(".L_afterif", &IfBody);
  TheBuilder->addInstruction(Jne{AfterIf});
  TheBuilder->setInsertionPoint(IfBody);
  for (const auto &S : SNode.Body) {
    codegen(S);
  }
  TheBuilder->setInsertionPoint(AfterIf);
  Tracker->freeOccupiedGPRegIfUnnamed(CondRes);
}

void CodeGen::codegen(const AST::ReturnStatement &SNode) {
  if (SNode.Value.has_value()) {
    Register ToReturn = codegen(*SNode.Value);
    if (ToReturn != ReturnRegister()) {
      TheBuilder->addInstruction(Mov{ReturnRegister(), ToReturn});
    }
  }
  if (TheBuilder->getNextBlock() != &TheBuilder->getEndBlock()) {
    TheBuilder->addInstruction(Jmp{TheBuilder->getEndBlock()});
  }
}

Register CodeGen::codegen(const AST::LocalVarDecl &SNode) {
  Register ExprValue = codegen(SNode.Value);
  if (Tracker->holdsVar(ExprValue)) {
    Register Res = Tracker->occupyFreeGPReg(SNode.Name);
    TheBuilder->addInstruction(Mov{Res, ExprValue});
    return Res;
  } else {
    Tracker->assignToVar(ExprValue, SNode.Name);
    return ExprValue;
  }
}

Register CodeGen::codegen(const AST::AssignmentStatement &SNode) {
  const auto &RHS = codegen(SNode.Value);
  const auto &LHS = Tracker->findRegisterHoldingValue(SNode.Name);
  if (RHS == LHS) {
    return LHS;
  }
  TheBuilder->addInstruction(Mov{LHS, RHS});
  Tracker->freeOccupiedGPRegIfUnnamed(RHS);
  return LHS;
}

void CodeGen::codegen(const AST::WhileStatement &SNode) {
  BasicBlock &CondBlock =
    TheBuilder->addBasicBlock(".L_while_cond", TheBuilder->CurBB);
  BasicBlock &WhileBody =
    TheBuilder->addBasicBlock(".L_while_body", &CondBlock);
  BasicBlock &AfterWhile =
    TheBuilder->addBasicBlock(".L_after_while", &WhileBody);

  TheBuilder->setInsertionPoint(CondBlock);
  Register CondRes = codegen(SNode.Cond);
  TheBuilder->addInstruction(Cmp{CondRes, Immediate::get(1)});
  TheBuilder->addInstruction(Jne{AfterWhile});
  TheBuilder->setInsertionPoint(WhileBody);
  for (const auto &S : SNode.Body) {
    codegen(S);
  }
  TheBuilder->addInstruction(Jmp{CondBlock});
  Tracker->freeOccupiedGPRegIfUnnamed(CondRes);
  TheBuilder->setInsertionPoint(AfterWhile);
}

void CodeGen::codegen(const AST::Prototype &PNode) {
  Function &F = TheBuilder->addFunction(Function(
    PNode.Name, PNode.IsExtern ? LinkageType::Extern : LinkageType::Internal));
  TheBuilder->setBBInsertionPoint(F);
}

void CodeGen::codegen(const AST::Function &FNode) {
  codegen(*FNode.Proto);
  BasicBlock &Start = TheBuilder->addBasicBlock(
    BasicBlock(TheBuilder->CurFunction->Name), nullptr);
  TheBuilder->setInsertionPoint(Start);
  EndBB = &TheBuilder->addBasicBlock(BasicBlock{"end"}, &Start);
  TheBuilder->addInstruction(Ret{}, EndBB);

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