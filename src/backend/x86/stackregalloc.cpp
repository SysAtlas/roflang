#include "backend/x86/registers.hpp"
#include <backend/x86/stackregalloc.hpp>
#include <backend/x86/builder.hpp>

namespace X86 {

void StackRegAlloc::allocate(Function &function) {
  builder->setBBInsertionPoint(function);
  builder->setInsertionPoint(function.bbs.front());

  // Allocate stack space for all registers
  builder->addInstruction(
      Sub{RSP(),
          Immediate(static_cast<i32>(isel_info.virtual_register_count) * 8)});
  
  for (auto& bb : function.bbs) {
    for (auto& instr : bb.instructions) {
      for (OpInfo &op_info : instr.op_infos) {
        if (Register* res = std::get_if<Register>(&op_info.op)) {
          // op_info.op = 
        }
      }
    }
  }

  // Deallocate
  builder->addInstruction(
      Sub{RSP(),
          Immediate(-static_cast<i32>(isel_info.virtual_register_count) * 8)});
}

void StackRegAlloc::allocate() {
  for (auto &function : mod->functions) {
    allocate(*function);
  }
}

StackRegAlloc::StackRegAlloc(std::unique_ptr<Module> mod, ISelInfo isel_info)
    : mod{std::move(mod)}, isel_info{isel_info}, builder{std::make_unique<Builder>(mod.get())} {}

std::unique_ptr<Module> StackRegAlloc::allocate(std::unique_ptr<Module> mod,
                                                ISelInfo isel_info) {
  StackRegAlloc reg_alloc{std::move(mod), isel_info};
  reg_alloc.allocate();
}

} // namespace X86