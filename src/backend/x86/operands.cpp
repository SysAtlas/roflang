#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>
#include <backend/x86/operands.hpp>
#include <backend/x86/registers.hpp>

namespace X86 {

std::string toString(const Register &reg) {
  switch (reg.type) {
  case (RegisterType::RAX): {
    if (reg.size == 8) {
      return "al";
    }
    return "rax";
  }
  case (RegisterType::RBX): {
    if (reg.size == 8) {
      return "bl";
    }
    return "rbx";
  }
  case (RegisterType::RCX): {
    if (reg.size == 8) {
      return "cl";
    }
    return "rcx";
  }
  case (RegisterType::RDX): {
    if (reg.size == 8) {
      return "dl";
    }
    return "rdx";
  }
  case (RegisterType::R8): {
    if (reg.size == 8) {
      return "r8b";
    }
    return "r8";
  }
  case (RegisterType::R9): {
    if (reg.size == 8) {
      return "r9b";
    }
    return "r9";
  }
  case (RegisterType::R10): {
    if (reg.size == 8) {
      return "r10b";
    }
    return "r10";
  }
  case (RegisterType::R11): {
    if (reg.size == 8) {
      return "r11b";
    }
    return "r11";
  }
  case (RegisterType::R12): {
    if (reg.size == 8) {
      return "r12b";
    }
    return "r12";
  }
  case (RegisterType::R13): {
    if (reg.size == 8) {
      return "r13b";
    }
    return "r13";
  }
  case (RegisterType::R14): {
    if (reg.size == 8) {
      return "r14b";
    }
    return "r14";
  }
  case (RegisterType::R15): {
    if (reg.size == 8) {
      return "r15b";
    }
    return "r15";
  }
  case (RegisterType::RDI): {
    if (reg.size == 8) {
      return "dil";
    }
    return "rd";
  }
  case (RegisterType::RSI): {
    if (reg.size == 8) {
      return "sil";
    }
    return "rsi";
  }
  case (RegisterType::RSP): {
    if (reg.size == 8) {
      return "spl";
    }
    return "rsp";
  }
  case (RegisterType::RBP): {
    if (reg.size == 8) {
      return "bpl";
    }
    return "rbp";
  }
  case (RegisterType::NONE): {
    if (reg.vid == -1) {
      abort();
    }
    return std::format("v{}.{}", reg.vid, reg.size);
  }
  }
}

std::string toString(const Immediate &imm) { return std::to_string(imm.value); }

std::string toString(const Label &label) { return label.bb.name; }

std::string toString(const FunctionRef &fn_ref) { return fn_ref.fn.name; }

} // namespace X86