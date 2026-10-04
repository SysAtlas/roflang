#include <target/x86/registers.hpp>

namespace X86 {

RAX RAX_{};
RBX RBX_{};
RCX RCX_{};
RDX RDX_{};
R8 R8_{};
R9 R9_{};
R10 R10_{};
R11 R11_{};
R12 R12_{};
R13 R13_{};
R14 R14_{};
R15 R15_{};
RSI RSI_{};
RDI RDI_{};
RSP RSP_{};
RBP RBP_{};

const std::array<const Register *, 16> RF::Registers{&RAX_,
                                                               &RBX_,
                                                               &RCX_,
                                                               &RDX_,
                                                               &R8_,
                                                               &R9_,
                                                               &R10_,
                                                               &R11_,
                                                               &R12_,
                                                               &R13_,
                                                               &R14_,
                                                               &R15_,
                                                               &RSI_,
                                                               &RDI_,
                                                               &RSP_,
                                                               &RBP_};
const RAX &RF::getReturnRegister() {
  return get<RAX>();
}
} // namespace X86
