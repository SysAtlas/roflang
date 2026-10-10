#pragma once

#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>
#include <backend/x86/operands.hpp>
#include <backend/x86/registers.hpp>
#include <common.hpp>
#include <vector>

namespace X86 {

// TODO: RegImmOrMem

enum class Opcode {
  MOV,
  PUSH,
  POP,
  XCHG,
  ADD,
  SUB,
  IMUL,
  IDIV,
  MOVSX,
  CQO,
  XOR,
  CMP,
  SETZ,
  SETNE,
  SETLE,
  SETL,
  SETGE,
  SETG,
  JMP,
  CALL,
  RET,
  JZ,
  JG,
  JGE,
  JL,
  JLE,
  JNE
};

class Instruction {
protected:
  Instruction(Opcode opcode, const std::vector<OpInfo> &operands)
      : opcode{opcode}, op_infos{operands} {}

public:
  i32 number = -1;
  Opcode opcode;
  std::vector<OpInfo> op_infos;

  std::string toString() const;
  std::string dbgString() const;
  bool isJump() const;
  bool isUncondJump() const;
  bool isRet() const;
  bool isTerminator() const;

  // TODO: Replace with an iterator...
  std::vector<Register> getReadRegs();
  std::vector<Register> getWriteRegs();

  std::vector<Register> getClobberSet() const;
};

// Supported instructions
// TODO: Support mem or reg..

template <typename T>
concept RegImm = std::is_same_v<T, Register> || std::is_same_v<T, Immediate>;

struct Mov : public Instruction {
  template <RegImm SrcType>
  Mov(const Register &dst, const SrcType &src)
      : Instruction(Opcode::MOV, {OpInfo{dst, AccessType::DST},
                                  OpInfo{src, AccessType::SRC}}) {}
};

struct Push : public Instruction {
  Push(const Register &src)
      : Instruction{Opcode::PUSH, {{OpInfo{src, AccessType::SRC}}}} {}
};

struct Pop : public Instruction {
  Pop(const Register &dst)
      : Instruction{Opcode::POP, {OpInfo{dst, AccessType::DST}}} {}
};

struct Xchg : public Instruction {
  Xchg(const Register &op0, const Register &op1)
      : Instruction{Opcode::XCHG,
                    {OpInfo{op0, AccessType::DSTSRC},
                     OpInfo{op1, AccessType::DSTSRC}}} {}
};

// Arithmetic

struct Add : public Instruction {
  Add(const Register &dst, const Register &src)
      : Instruction{
            Opcode::ADD,
            {OpInfo{dst, AccessType::DSTSRC}, OpInfo{src, AccessType::SRC}}} {}
};

struct Sub : public Instruction {
  template <RegImm SrcType>
  Sub(const Register &dst, const SrcType &src)
      : Instruction{
            Opcode::SUB,
            {OpInfo{dst, AccessType::DSTSRC}, OpInfo{src, AccessType::SRC}}} {}
};

struct IMul : public Instruction {
  IMul(const Register &dst, const Register &src0)
      : Instruction{
            Opcode::IMUL,
            {OpInfo{dst, AccessType::DSTSRC}, OpInfo{src0, AccessType::SRC}}} {}
};

struct IDiv : public Instruction {
  IDiv(const Register &dst)
      : Instruction{Opcode::IDIV, {OpInfo{dst, AccessType::SRC}}} {}
};

// Bit manipulation

struct MovSX : public Instruction {
  MovSX(const Register &dst, const Register &src)
      : Instruction(Opcode::MOVSX, {OpInfo{dst, AccessType::DST},
                                    OpInfo{src, AccessType::SRC}}) {}
};

struct Xor : public Instruction {
  Xor(const Register &dst, const Register &src)
      : Instruction{
            Opcode::XOR,
            {OpInfo{dst, AccessType::DSTSRC}, OpInfo{src, AccessType::SRC}}} {}
};

// RDX:RAX = sign extend of RAX
struct Cqo : public Instruction {
  Cqo() : Instruction{Opcode::CQO, {}} {}
};

// Cmp

struct Cmp : public Instruction {
  template <RegImm SrcType>
  Cmp(const Register &dst, const SrcType &src)
      : Instruction{
            Opcode::CMP,
            {OpInfo{dst, AccessType::SRC}, OpInfo{src, AccessType::SRC}}} {}
};

struct SetZ : public Instruction {
  SetZ(const Register &dst);
};

struct SetNE : public Instruction {
  SetNE(const Register &dst);
};

struct SetL : public Instruction {
  SetL(const Register &dst);
};

struct SetLE : public Instruction {
  SetLE(const Register &dst);
};

struct SetG : public Instruction {
  SetG(const Register &dst);
};

struct SetGE : public Instruction {
  SetGE(const Register &dst);
};

// Control flow

struct Ret : public Instruction {
  Ret();
};

struct Call : public Instruction {
  Call(Function &dst);
};

struct Jmp : public Instruction {
  Jmp(BasicBlock &dst);
};

struct Jz : public Instruction {
  Jz(BasicBlock &dst);
};

struct Jne : public Instruction {
  Jne(BasicBlock &dst);
};

struct Jl : public Instruction {
  Jl(BasicBlock &dst);
};

struct Jle : public Instruction {
  Jle(BasicBlock &dst);
};

struct Jg : public Instruction {
  Jg(BasicBlock &dst);
};

struct Jge : public Instruction {
  Jge(BasicBlock &dst);
};

} // namespace X86