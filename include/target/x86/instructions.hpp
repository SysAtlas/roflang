#pragma once

#include <common.hpp>
#include <concepts>
#include <target/x86/registers.hpp>
#include <target/x86/operands.hpp>

namespace X86 {

template <typename T>
concept RegOrImm =
  std::derived_from<T, Register> || std::is_same_v<T, Immediate>;

template <typename T>
concept Reg = std::derived_from<T, Register>;

// TODO: RegImmOrMem

struct InstructionBase {
  virtual std::string toString() const = 0;
  virtual ~InstructionBase() = default;
  std::string OpCode;

  InstructionBase(const std::string &OpCode) : OpCode{OpCode} {}
};

template <u32 Size> struct Instruction : public InstructionBase {
  std::array<OpInfo, Size> Operands;
  virtual ~Instruction() = default;

  std::string toString() const override {
    std::string Res = OpCode;
    for (const auto& OI : Operands) {
      Res += ' ' + OI.Op.toString() + ',';
    }
    if (!Operands.empty()) {
      Res.pop_back();
    }
    return Res;
  }

  Instruction(const std::string OpCode, std::array<OpInfo, Size> Operands)
      : InstructionBase(OpCode), Operands{Operands} {}
};

// Supported instructions
// TODO: Support mem or reg..

struct MovInstruction : public Instruction<2> {
  template <u32 Size, RegOrImm SrcType>
  MovInstruction(const GPRegister<Size> &Dst, const SrcType &Src)
      : Instruction("mov", {OpInfo{Dst, true}, OpInfo{Src, false}}) {}
};

struct PushInstruction : public Instruction<1> {
  template <RegOrImm SrcType>
  PushInstruction(const SrcType &Src)
      : Instruction{"push", {{OpInfo{Src, false}}}} {}
};

struct PopInstruction : public Instruction<1> {
  template <u32 Size>
  PopInstruction(const GPRegister<Size>& Dst) : Instruction{"pop", {OpInfo{Dst, true}}} {}
};

struct RetInstruction : public Instruction<0> {
  RetInstruction() : Instruction{"ret", {}} {}
};

struct AddInstruction : public Instruction<2> {
  template <RegOrImm SrcType>
  AddInstruction(const RAX &Dst, const SrcType &Src)
      : Instruction{"add", {OpInfo{Dst, true}, OpInfo{Src, false}}} {}
};

struct SubInstruction : public Instruction<2> {
  template <RegOrImm SrcType>
  SubInstruction(const RAX &Dst, const SrcType &Src)
      : Instruction{"sub", {OpInfo{Dst, true}, OpInfo{Src, false}}} {}
};

struct IMulInstruction : public Instruction<3> {
  template <u32 Size, RegOrImm Src1Type>
  IMulInstruction(const GPRegister<Size> &Dst,
                  const GPRegister<Size> &Src0,
                  const Src1Type &Src1)
      : Instruction{
          "imul",
          {OpInfo{Dst, true}, OpInfo{Src0, false}, OpInfo{Src1, false}}} {}
};
} // namespace X86