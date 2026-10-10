#include <backend/x86/basicblock.hpp>
#include <backend/x86/instructions.hpp>

namespace X86 {

std::string BasicBlock::toString() const {
  std::string res = std::format("{}:\n", name);
  for (const auto &instr : instructions) {
    res += std::format("  {}\n", instr.dbgString());
  }
  return res;
}

std::string BasicBlock::dbgString() const {
  std::string res = std::format("{}:\n", name);
  for (const auto &instr : instructions) {
    res += std::format("  {}: {}\n", instr.number, instr.dbgString());
  }
  return res;
}

BasicBlock::BasicBlock(std::string_view name) : name{name} {}

} // namespace X86