#pragma once

#include <backend/x86/module.hpp>

namespace X86 {
// Creates a memory slot for every virtual register.
// Used for testing purposes 
class StackRegAlloc {
  std::unique_ptr<Module> mod;
  ISelInfo isel_info;
  std::unique_ptr<Builder> builder;

  void allocate(Function& function);
  void allocate();
  StackRegAlloc(std::unique_ptr<Module> mod, ISelInfo isel_info);
  public:
  static std::unique_ptr<Module> allocate(std::unique_ptr<Module> mod, ISelInfo isel_info);
};

}