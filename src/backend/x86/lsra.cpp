#include "backend/x86/livevariables.hpp"
#include <backend/x86/builder.hpp>
#include <backend/x86/function.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/lsra.hpp>
#include <backend/x86/registers.hpp>

namespace X86 {

void LSRA::expire_old_intervals(LiveInterval &i) {
  auto it = active_intervals.begin();

  while (it != active_intervals.end() && it->end < i.start) {
    free_registers.push_front(*it->alloc_reg);
    ++it;
  }

  active_intervals.erase(active_intervals.begin(), it);
}

void LSRA::spill_at_interval(LiveInterval &i) {
  // Edge case: no register is assigned to a live range, but all registers are
  // banned should be okay to assert, as this shouldn't happen.
  assert(!active_intervals.empty());
  LiveInterval spill = active_intervals.back();
  bool should_swap =
      std::ranges::find(i.clobber_set, spill.alloc_reg) == i.clobber_set.end();
  if (spill.end > i.end && should_swap) {
    i.alloc_reg = *spill.alloc_reg;
    spilled_variables.insert(spill.variable_id);
    active_intervals.pop_back();
    add_to_active(i);
  } else {
    spilled_variables.insert(i.variable_id);
  }
}

// Keeps the invariant of sortedness by increasing end point
void LSRA::add_to_active(LiveInterval &i) {
  for (auto it = active_intervals.begin(), ie = active_intervals.end();
       it != ie; ++it) {
    if (it->end >= i.end) {
      active_intervals.insert(it, i);
      return;
    }
  }
  active_intervals.insert(active_intervals.end(), i);
}

void LSRA::allocate(Function &f) {
  // Sorted in order of increasing start point
  std::vector<LiveInterval> live_intervals;
  live_intervals.reserve(f.live_int_map.size());

  for (auto &i : f.live_int_map) {
    live_intervals.push_back(i.second);
  }

  std::sort(live_intervals.begin(), live_intervals.end(),
            [](const LiveInterval &first, const LiveInterval &second) {
              return first.start < second.start;
            });
  assert(live_intervals.front().start <= live_intervals.back().start);

  // amount of physical registers available to the allocator
  for (LiveInterval &i : live_intervals) {
    expire_old_intervals(i);
    bool should_spill = true;
    for (auto it = free_registers.begin(), ie = free_registers.end(); it != ie;
         ++it) {
      if (std::ranges::find(i.clobber_set, *it) == i.clobber_set.end()) {
        i.alloc_reg = *it;
        free_registers.erase(it);
        add_to_active(i);
        should_spill = false;
        break;
      }
    }
    if (should_spill) {
      spill_at_interval(i);
    }
  }

  // For convenience, map all variable names that use physical registers to
  // their physical registers
  for (auto &li : live_intervals) {
    if (!spilled_variables.contains(li.variable_id)) {
      assert(!li.alloc_reg->isVirtual());
      id_to_phys_reg[li.variable_id] = *li.alloc_reg;
    }
  }
}

void LSRA::virt_to_phys(Function &function) {
  for (auto &bb : function.bbs) {
    for (auto &i : bb.instructions) {
      for (auto &oi : i->op_infos) {
        if (!oi.is<Register>()) {
          continue;
        }
        Register reg = oi.get<Register>();

        if (!reg.isVirtual()) {
          continue;
        }

        // Another pass will handle allocation of memory slots
        if (spilled_variables.contains(reg.vid)) {
          continue;
        }

        Register phys_register =
            id_to_phys_reg.at(reg.vid).getSubregister(reg.size);
        assert(!phys_register.isVirtual());
        oi.op = phys_register;
      }
    }
  }
}

void LSRA::runPipeline() {
  for (auto &function : mod->getFunctionDefs()) {
    // TODO: pressure regalloc, test with different configurations and different free registers
    free_registers = {getAllocatableRegisters().begin(), getAllocatableRegisters().end()};
    active_intervals.clear();
    spilled_variables.clear();
    id_to_phys_reg.clear();
    allocate(*function);
    virt_to_phys(*function);
  }
}

LSRA::LSRA(Module *mod) : mod{mod} {}

void LSRA::runPass(Module *mod) {
  LSRA reg_alloc{mod};
  reg_alloc.runPipeline();
}

} // namespace X86