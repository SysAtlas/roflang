#pragma once

#include <common.hpp>
#include <target/x86/operands.hpp>
#include <target/x86/registers.hpp>
#include <vector>

namespace X86 {

// TODO: RegImmOrMem

class Instruction {
protected:
  std::vector<OpInfo> Operands;
  std::string OpCode;

  Instruction(const std::string &OpCode, const std::vector<OpInfo> &Operands)
      : Operands{Operands}, OpCode{OpCode} {}

public:
  std::string toString() const {
    std::string Res = OpCode;
    for (const auto &OI : Operands) {
      Res += ' ' + OI.Op.toString() + ',';
    }
    if (!Operands.empty()) {
      Res.pop_back();
    }
    return Res;
  }
};

// Supported instructions
// TODO: Support mem or reg..

template <typename T>
concept RegImm = std::is_same_v<T, Register> || std::is_same_v<T, Immediate>;

struct Mov : public Instruction {
  template <RegImm SrcType>
  Mov(const Register &Dst, const SrcType &Src)
      : Instruction("mov", {OpInfo{Dst, true}, OpInfo{Src, false}}) {}
};

struct Push : public Instruction {
  Push(const Register &Src) : Instruction{"push", {{OpInfo{Src, false}}}} {}
};

struct Pop : public Instruction {
  Pop(const Register &Dst) : Instruction{"pop", {OpInfo{Dst, true}}} {}
};

struct Xchg : public Instruction {
  Xchg(const Register &Op1, const Register &Op2)
      : Instruction{"xchg",
                    {OpInfo{Op1, true, true}, OpInfo{Op2, true, true}}} {}
};

// Arithmetic

struct Add : public Instruction {
  Add(const Register &Dst, const Register &Src)
      : Instruction{"add", {OpInfo{Dst, true}, OpInfo{Src, false}}} {}
};

struct Sub : public Instruction {
  Sub(const Register &Dst, const Register &Src)
      : Instruction{"sub", {OpInfo{Dst, true}, OpInfo{Src, false}}} {}
};

struct IMul : public Instruction {
  IMul(const Register &Dst, const Register &Src0)
      : Instruction{"imul", {OpInfo{Dst, true}, OpInfo{Src0, false}}} {}
};

// Bit manipulation

struct MovSX : public Instruction {
  MovSX(const Register &Dst, const Register &Src)
      : Instruction("movsx", {OpInfo{Dst, true}, OpInfo{Src, false}}) {}
};

// Cmp

struct Cmp : public Instruction {
  template <RegImm SrcType>
  Cmp(const Register &Dst, const SrcType &Src)
      : Instruction{"cmp", {OpInfo{Dst, true}, OpInfo{Src, false}}} {}
};

struct SetZ : public Instruction {
  SetZ(const Register &Destination)
      : Instruction{"setz", {OpInfo{Destination, false}}} {}
};

struct SetNE : public Instruction {
  SetNE(const Register &Destination)
      : Instruction{"setne", {OpInfo{Destination, false}}} {}
};

struct SetL : public Instruction {
  SetL(const Register &Destination)
      : Instruction{"setl", {OpInfo{Destination, false}}} {}
};

struct SetLE : public Instruction {
  SetLE(const Register &Destination)
      : Instruction{"setle", {OpInfo{Destination, false}}} {}
};

struct SetG : public Instruction {
  SetG(const Register &Destination)
      : Instruction{"setg", {OpInfo{Destination, false}}} {}
};

struct SetGE : public Instruction {
  SetGE(const Register &Destination)
      : Instruction{"setge", {OpInfo{Destination, false}}} {}
};

// Control flow

struct Ret : public Instruction {
  Ret() : Instruction{"ret", {}} {}
};

struct Jmp : public Instruction {
  Jmp(const BasicBlock &Destination)
      : Instruction{"jmp", {OpInfo{Label{Destination}, false}}} {}
};

struct Jz : public Instruction {
  Jz(const BasicBlock &Destination)
      : Instruction{"jz", {OpInfo{Label{Destination}, false}}} {}
};

struct Jne : public Instruction {
  Jne(const BasicBlock &Destination)
      : Instruction{"jne", {OpInfo{Label{Destination}, false}}} {}
};

struct Call : public Instruction {
  Call(const BasicBlock &Destination)
      : Instruction{"call", {OpInfo{Label{Destination}, false}}} {}
};

// signed

struct Jl : public Instruction {
  Jl(const BasicBlock &Destination)
      : Instruction{"jl", {OpInfo{Label{Destination}, false}}} {}
};

struct Jle : public Instruction {
  Jle(const BasicBlock &Destination)
      : Instruction{"jle", {OpInfo{Label{Destination}, false}}} {}
};

struct Jg : public Instruction {
  Jg(const BasicBlock &Destination)
      : Instruction{"jg", {OpInfo{Label{Destination}, false}}} {}
};

struct Jge : public Instruction {
  Jge(const BasicBlock &Destination)
      : Instruction{"jge", {OpInfo{Label{Destination}, false}}} {}
};

} // namespace X86