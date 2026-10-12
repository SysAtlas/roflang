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

  std::vector<UP<Function>> functions;
  std::vector<Function *> getFunctionDefs();

  Function &addFunction(UP<Function> function);

  public:
  std::string toString();
  std::string dbgString();
  Module() = default;
  friend Builder;
};

} // namespace X86