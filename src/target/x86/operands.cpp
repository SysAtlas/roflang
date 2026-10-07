#include <target/x86/basicblock.hpp>
#include <target/x86/operands.hpp>

namespace X86 {

std::string toString(const Register &R) {
  switch (R.Type) {
  case (RegisterType::RAX): {
    if (R.Size == RegisterSize::R8) {
      return "al";
    }
    return "rax";
  }
  case (RegisterType::RBX): {
    if (R.Size == RegisterSize::R8) {
      return "bl";
    }
    return "rbx";
  }
  case (RegisterType::RCX): {
    if (R.Size == RegisterSize::R8) {
      return "cl";
    }
    return "rcx";
  }
  case (RegisterType::RDX): {
    if (R.Size == RegisterSize::R8) {
      return "dl";
    }
    return "rdx";
  }
  case (RegisterType::R8): {
    if (R.Size == RegisterSize::R8) {
      return "r8b";
    }
    return "r8";
  }
  case (RegisterType::R9): {
    if (R.Size == RegisterSize::R8) {
      return "r9b";
    }
    return "r9";
  }
  case (RegisterType::R10): {
    if (R.Size == RegisterSize::R8) {
      return "r10b";
    }
    return "r10";
  }
  case (RegisterType::R11): {
    if (R.Size == RegisterSize::R8) {
      return "r11b";
    }
    return "r11";
  }
  case (RegisterType::R12): {
    if (R.Size == RegisterSize::R8) {
      return "r12b";
    }
    return "r12";
  }
  case (RegisterType::R13): {
    if (R.Size == RegisterSize::R8) {
      return "r13b";
    }
    return "r13";
  }
  case (RegisterType::R14): {
    if (R.Size == RegisterSize::R8) {
      return "r14b";
    }
    return "r14";
  }
  case (RegisterType::R15): {
    if (R.Size == RegisterSize::R8) {
      return "r15b";
    }
    return "r15";
  }
  case (RegisterType::RDI): {
    if (R.Size == RegisterSize::R8) {
      return "dil";
    }
    return "rd";
  }
  case (RegisterType::RSI): {
    if (R.Size == RegisterSize::R8) {
      return "sil";
    }
    return "rsi";
  }
  case (RegisterType::RSP): {
    if (R.Size == RegisterSize::R8) {
      return "spl";
    }
    return "rsp";
  }
  case (RegisterType::RBP): {
    if (R.Size == RegisterSize::R8) {
      return "bpl";
    }
    return "rbp";
  }
  default: {
    abort();
  }
  }
}

std::string toString(const Immediate &Imm) {
  return std::to_string(Imm.Value);
}

std::string toString(const Label &L) {
  return L.BB.Name;
}

} // namespace X86