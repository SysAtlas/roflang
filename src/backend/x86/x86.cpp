#include <backend/x86/instructions.hpp>
#include <backend/x86/x86.hpp>
#include <unordered_map>

namespace X86 {

std::string Module::emitHeader() {
  std::string res = ".intel_syntax noprefix\n\n.text\n";

  if (!functions.empty()) {
    res += ".global";
  }
  for (const auto &fn : functions) {
    if (fn->linkage == Linkage::EXTERN) {
      res += std::format(" {},", fn->name);
    }
  }
  if (!functions.empty()) {
    res.pop_back();
    res += '\n';
  }
  res += '\n';

  return res;
}
std::string Module::toString() {
  std::string result = emitHeader();
  for (const auto &fn : functions) {
    result += fn->toString();
  }
  return result;
}
Function &Module::addFunction(std::unique_ptr<Function> fn) {
  functions.emplace_back(std::move(fn));
  return *functions.back();
}
std::string BasicBlock::toString() const {
  std::string res = std::format("{}:\n", name);
  for (const auto &instr : instructions) {
    res += "  " + instr.toString() + '\n';
  }
  return res;
}
BasicBlock::BasicBlock(std::string_view name) : name{name} {}

std::string Function::toString() const {
  std::string res;
  for (const BasicBlock &bb : bbs) {
    res += bb.toString() + '\n';
  }
  return res;
}
Function::Function(std::string_view name, Linkage linkage)
    : name{name}, linkage{linkage} {}

BasicBlock &Function::addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after) {
  if (insert_after == nullptr) {
    bbs.push_front(std::move(bb));
    return bbs.front();
  }
  return *bbs.insert(
    ++std::ranges::find_if(
      bbs,
      [insert_after](const BasicBlock &arg) { return &arg == insert_after; }),
    std::move(bb));
}

} // namespace X86