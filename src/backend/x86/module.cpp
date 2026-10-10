#include <backend/x86/instructions.hpp>
#include <backend/x86/module.hpp>
#include <cassert>

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

Function &Module::addFunction(std::unique_ptr<Function> function) {
  assert(function != nullptr);
  functions.emplace_back(std::move(function));
  return *functions.back();
}
} // namespace X86