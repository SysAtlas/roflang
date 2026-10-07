#include <target/x86/registers.hpp>

namespace X86 {

Register RAX(RegisterSize Size) {
  return Register{Size, RegisterType::RAX};
}
Register RBX(RegisterSize Size) {
  return Register{Size, RegisterType::RBX};
}
Register RCX(RegisterSize Size) {
  return Register{Size, RegisterType::RCX};
}
Register RDX(RegisterSize Size) {
  return Register{Size, RegisterType::RDX};
}
Register R8(RegisterSize Size) {
  return Register{Size, RegisterType::R8};
}
Register R9(RegisterSize Size) {
  return Register{Size, RegisterType::R9};
}
Register R10(RegisterSize Size) {
  return Register{Size, RegisterType::R10};
}
Register R11(RegisterSize Size) {
  return Register{Size, RegisterType::R11};
}
Register R12(RegisterSize Size) {
  return Register{Size, RegisterType::R12};
}
Register R13(RegisterSize Size) {
  return Register{Size, RegisterType::R13};
}
Register R14(RegisterSize Size) {
  return Register{Size, RegisterType::R14};
}
Register R15(RegisterSize Size) {
  return Register{Size, RegisterType::R15};
}
Register RSI(RegisterSize Size) {
  return Register{Size, RegisterType::RSI};
}
Register RDI(RegisterSize Size) {
  return Register{Size, RegisterType::RDI};
}
Register RSP(RegisterSize Size) {
  return Register{Size, RegisterType::RSP};
}
Register RBP(RegisterSize Size) {
  return Register{Size, RegisterType::RBP};
}

Register ReturnRegister() {
  return RAX();
}

Register Register::getLo8() const {
  return Register{RegisterSize::R8, Type};
}

} // namespace X86