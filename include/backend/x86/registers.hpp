#pragma once

#include <cassert>
#include <common.hpp>
#include <vector>

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

class Register {
  public:
  RegisterType type;
  u32 size;
  i32 vid = -1;

  bool isVirtual();
  static Register getVirtual(u32 size, u32 vid) { return Register{.type = RegisterType::NONE, .size = size, .vid = static_cast<i32>(vid) }; }

  bool operator==(const Register &other) const {
    return type == other.type && vid == other.vid;
  }
  // Non-virtual registers shouldn't be compared
  bool operator<(const Register& other) const {
    assert(vid == -1 && other.vid == -1);
    return type < other.type;
  }
  Register getSubregister(u8 size) const;
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

const std::array<Register, 14>& getAllocatableRegisters();
const std::array<Register, 7> &getCalleeSaved();
const std::array<Register, 9> &getCallerSaved();
bool isCallerSaved(const Register &reg);
bool isCalleeSaved(const Register &reg);

} // namespace X86