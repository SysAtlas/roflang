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