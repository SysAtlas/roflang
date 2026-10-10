#pragma once

#include "backend/x86/registers.hpp"
#include <backend/x86/module.hpp>

namespace X86 {
// Creates a memory slot for every virtual register.
// Used for testing purposes 
class LSRA {

  Module* mod;
  // Sorted in order of increasing end point
  // TODO: Use heap?
  std::vector<LiveInterval> active_intervals;
  std::list<Register> free_registers;
  std::set<u32> spilled_variables;

  std::unordered_map<u32, Register> id_to_phys_reg;

  void add_to_active(LiveInterval& i);
  void expire_old_intervals(LiveInterval& i);
  void spill_at_interval(LiveInterval& i);

  void allocate(Function& function);
  void virt_to_phys(Function& function);

  void runPipeline();
  LSRA(Module* mod);
  public:
  static void runPass(Module* mod);
};

}