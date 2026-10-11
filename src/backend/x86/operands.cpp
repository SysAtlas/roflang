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
    if (reg.size == 16) {
      return "ax";
    }
    if (reg.size == 32) {
      return "eax";
    }
    return "rax";
  }
  case (RegisterType::RBX): {
    if (reg.size == 8) {
      return "bl";
    }
    if (reg.size == 16) {
      return "bx";
    }
    if (reg.size == 32) {
      return "ebx";
    }
    return "rbx";
  }
  case (RegisterType::RCX): {
    if (reg.size == 8) {
      return "cl";
    }
    if (reg.size == 16) {
      return "cx";
    }
    if (reg.size == 32) {
      return "ecx";
    }
    return "rcx";
  }
  case (RegisterType::RDX): {
    if (reg.size == 8) {
      return "dl";
    }
    if (reg.size == 16) {
      return "dx";
    }
    if (reg.size == 32) {
      return "edx";
    }
    return "rdx";
  }
  case (RegisterType::R8): {
    if (reg.size == 8) {
      return "r8b";
    }
    if (reg.size == 16) {
      return "r8w";
    }
    if (reg.size == 32) {
      return "r8d";
    }
    return "r8";
  }
  case (RegisterType::R9): {
    if (reg.size == 8) {
      return "r9b";
    }
    if (reg.size == 16) {
      return "r9w";
    }
    if (reg.size == 32) {
      return "r9d";
    }
    return "r9";
  }
  case (RegisterType::R10): {
    if (reg.size == 8) {
      return "r10b";
    }
    if (reg.size == 16) {
      return "r10w";
    }
    if (reg.size == 32) {
      return "r10d";
    }
    return "r10";
  }
  case (RegisterType::R11): {
    if (reg.size == 8) {
      return "r11b";
    }
    if (reg.size == 16) {
      return "r11w";
    }
    if (reg.size == 32) {
      return "r11d";
    }
    return "r11";
  }
  case (RegisterType::R12): {
    if (reg.size == 8) {
      return "r12b";
    }
    if (reg.size == 16) {
      return "r12w";
    }
    if (reg.size == 32) {
      return "r12d";
    }
    return "r12";
  }
  case (RegisterType::R13): {
    if (reg.size == 8) {
      return "r13b";
    }
    if (reg.size == 16) {
      return "r13w";
    }
    if (reg.size == 32) {
      return "r13d";
    }
    return "r13";
  }
  case (RegisterType::R14): {
    if (reg.size == 8) {
      return "r14b";
    }
    if (reg.size == 16) {
      return "r14w";
    }
    if (reg.size == 32) {
      return "r14d";
    }
    return "r14";
  }
  case (RegisterType::R15): {
    if (reg.size == 8) {
      return "r15b";
    }
    if (reg.size == 16) {
      return "r15w";
    }
    if (reg.size == 32) {
      return "r15d";
    }
    return "r15";
  }
  case (RegisterType::RDI): {
    if (reg.size == 8) {
      return "dil";
    }
    if (reg.size == 16) {
      return "di";
    }
    if (reg.size == 32) {
      return "edi";
    }
    return "rdi";
  }
  case (RegisterType::RSI): {
    if (reg.size == 8) {
      return "sil";
    }
    if (reg.size == 16) {
      return "si";
    }
    if (reg.size == 32) {
      return "esi";
    }
    return "rsi";
  }
  case (RegisterType::RSP): {
    if (reg.size == 8) {
      return "spl";
    }
    if (reg.size == 16) {
      return "sp";
    }
    if (reg.size == 32) {
      return "esp";
    }
    return "rsp";
  }
  case (RegisterType::RBP): {
    if (reg.size == 8) {
      return "bpl";
    }
    if (reg.size == 16) {
      return "bp";
    }
    if (reg.size == 32) {
      return "ebp";
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

std::string toString(const MemOp &mem_op) {
  std::string res = "[";
  if (mem_op.base) {
    res += toString(*mem_op.base);
  }
  if (mem_op.displacement) {
    assert(mem_op.base);
    res += std::format(" + {}", toString(*mem_op.displacement));
  }
  res += "]";
  return res;
}

} // namespace X86