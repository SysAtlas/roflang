#include <target/x86/operands.hpp>

namespace X86 {

std::string toString(const Register &R) {
  return R.Name;
}

std::string toString(const Immediate &Imm) {
  return std::to_string(Imm.Value);
}

} // namespace X86