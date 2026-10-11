#include <backend/x86/registers.hpp>
#include <cassert>

namespace X86 {

Register RAX(u32 size) { return Register{RegisterType::RAX, size}; }
Register RBX(u32 size) { return Register{RegisterType::RBX, size}; }
Register RCX(u32 size) { return Register{RegisterType::RCX, size}; }
Register RDX(u32 size) { return Register{RegisterType::RDX, size}; }
Register R8(u32 size) { return Register{RegisterType::R8, size}; }
Register R9(u32 size) { return Register{RegisterType::R9, size}; }
Register R10(u32 size) { return Register{RegisterType::R10, size}; }
Register R11(u32 size) { return Register{RegisterType::R11, size}; }
Register R12(u32 size) { return Register{RegisterType::R12, size}; }
Register R13(u32 size) { return Register{RegisterType::R13, size}; }
Register R14(u32 size) { return Register{RegisterType::R14, size}; }
Register R15(u32 size) { return Register{RegisterType::R15, size}; }
Register RSI(u32 size) { return Register{RegisterType::RSI, size}; }
Register RDI(u32 size) { return Register{RegisterType::RDI, size}; }
Register RSP(u32 size) { return Register{RegisterType::RSP, size}; }
Register RBP(u32 size) { return Register{RegisterType::RBP, size}; }

const std::array<Register, 14> &getAllocatableRegisters() {
  static const std::array<Register, 14> ALL_REGS = {
      RAX(), RBX(), RCX(), RDX(), R8(),  R9(),  R10(), R11(),
      R12(), R13(), R14(), R15(), RSI(), RDI()};
  return ALL_REGS;
}

const std::array<Register, 9> &getCallerSaved() {
  static const std::array<Register, 9> CALLER_SAVED = {
      RAX(), RCX(), RDX(), RSI(), RDI(), R8(), R9(), R10(), R11()};
  return CALLER_SAVED;
}

const std::array<Register, 7> &getCalleeSaved() {
  static const std::array<Register, 7> CALLEE_SAVED = {
      RBX(), RBP(), RSP(), R12(), R13(), R14(), R15()};
  return CALLEE_SAVED;
}

bool isCallerSaved(const Register &reg) {
  switch (reg.type) {
  case (RegisterType::RAX):
  case (RegisterType::RCX):
  case (RegisterType::RDX):
  case (RegisterType::RSI):
  case (RegisterType::RDI):
  case (RegisterType::R8):
  case (RegisterType::R9):
  case (RegisterType::R10):
  case (RegisterType::R11):
    return true;
  default: {
    return false;
  }
  }
}

bool isCalleeSaved(const Register &reg) { return !isCallerSaved(reg); }

Register returnRegister() { return RAX(); }

bool Register::isVirtual() {
  assert((type != RegisterType::NONE) == (vid == -1));
  return vid >= 0 && type == RegisterType::NONE;
}

Register Register::getSubregister(u8 size) const {
  return Register{type, size, vid};
}

Register argumentRegister(u8 n) {
  switch (n) {
  case 0:
    return RDI();
  case 1:
    return RSI();
  case 2:
    return RDX();
  case 3:
    return RCX();
  case 4:
    return R8();
  case 5:
    return R9();
  default:
    std::cerr
        << "There are only 6 argument registers! (rsi, rdi, rdx, rcx, r8, r9)"
        << '\n';
    abort();
  }
}

} // namespace X86