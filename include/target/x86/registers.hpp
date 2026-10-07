#pragma once

#include <common.hpp>

namespace X86 {

enum class RegisterType {
  None,
  RAX,
  RBX,
  RCX,
  RDX,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  RSI,
  RDI,
  RSP,
  RBP,
};

enum class RegisterSize { R64, R32, R16, R8 };

struct Register {
  const RegisterSize Size;
  const RegisterType Type;

  bool operator==(const Register &Other) const = default;
  Register getLo8() const;
};

Register RAX(RegisterSize Size = RegisterSize::R64);
Register RBX(RegisterSize Size = RegisterSize::R64);
Register RCX(RegisterSize Size = RegisterSize::R64);
Register RDX(RegisterSize Size = RegisterSize::R64);
Register R8(RegisterSize Size = RegisterSize::R64);
Register R9(RegisterSize Size = RegisterSize::R64);
Register R10(RegisterSize Size = RegisterSize::R64);
Register R11(RegisterSize Size = RegisterSize::R64);
Register R12(RegisterSize Size = RegisterSize::R64);
Register R13(RegisterSize Size = RegisterSize::R64);
Register R14(RegisterSize Size = RegisterSize::R64);
Register R15(RegisterSize Size = RegisterSize::R64);
Register RSI(RegisterSize Size = RegisterSize::R64);
Register RDI(RegisterSize Size = RegisterSize::R64);
Register RSP(RegisterSize Size = RegisterSize::R64);
Register RBP(RegisterSize Size = RegisterSize::R64);

Register ReturnRegister();

} // namespace X86