#pragma once

#include <common.hpp>

namespace X86 {

enum class RegisterType {
  NONE,
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

struct Register {
  RegisterType type;
  u32 size;
  i32 vid = -1;

  bool isVirtual();

  bool operator==(const Register &other) const = default;
  Register getLo8() const;
};

Register RAX(u32 size = 64);
Register RBX(u32 size = 64);
Register RCX(u32 size = 64);
Register RDX(u32 size = 64);
Register R8(u32 size = 64);
Register R9(u32 size = 64);
Register R10(u32 size = 64);
Register R11(u32 size = 64);
Register R12(u32 size = 64);
Register R13(u32 size = 64);
Register R14(u32 size = 64);
Register R15(u32 size = 64);
Register RSI(u32 size = 64);
Register RDI(u32 size = 64);
Register RSP(u32 size = 64);
Register RBP(u32 size = 64);

Register returnRegister();
Register argumentRegister(u8 n);

} // namespace X86