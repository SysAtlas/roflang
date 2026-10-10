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

Register returnRegister() { return RAX(); }

bool Register::isVirtual() {
  assert((type != RegisterType::NONE) == (vid == -1));
  return vid >= 0 && type == RegisterType::NONE;
}

Register Register::getLo8() const { return Register{type, 8, vid}; }

Register argumentRegister(u8 n) {
  switch (n) {
  case 0:
    return RSI();
  case 1:
    return RDI();
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