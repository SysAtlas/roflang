#pragma once
#include <common.hpp>
#include <list>
#include <memory>
#include <vector>

#include <backend/x86/registers.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>

namespace X86 {

class Builder;

enum class Linkage { EXTERN, INTERNAL };

struct Module {
  std::string emitHeader();

  std::vector<std::unique_ptr<Function>> functions;

  Function &addFunction(std::unique_ptr<Function> function);

  public:
  std::string toString();
  Module() = default;
  friend Builder;
};

struct ISelInfo {
  u32 virtual_register_count;
  std::unordered_map<u32, std::string> id_to_var_name;
};


} // namespace X86