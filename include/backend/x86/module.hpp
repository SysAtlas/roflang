#pragma once

#include <common.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <backend/x86/function.hpp>

namespace X86 {

class Builder;
class Function;

enum class Linkage { EXTERN, INTERNAL };

class Module {
  public:
  std::string emitHeader();

  std::vector<std::unique_ptr<Function>> functions;
  std::vector<Function*> getFunctionDefs() {
    std::vector<Function*> res;
    for (auto& f : functions) {
      if (!f->is_decl) {
        res.push_back(f.get());
      }
    }
    return res;
  }

  Function &addFunction(std::unique_ptr<Function> function);

  public:
  std::string toString();
  std::string dbgString();
  Module() = default;
  friend Builder;
};

} // namespace X86