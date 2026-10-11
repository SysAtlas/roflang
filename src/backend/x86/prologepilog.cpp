#include <backend/x86/builder.hpp>
#include <backend/x86/function.hpp>
#include <backend/x86/prologepilog.hpp>

namespace X86 {

void PrologEpilog::runPass(Module *mod) {
  Builder b{mod};
  for (Function *f : mod->getFunctionDefs()) {
    if (f->bbs.empty()) {
      continue;
    }
    b.setFunction(*f);
    b.setInsertionBB(*f->entry_block);
    auto it = f->entry_block->instructions.begin();
    b.insertInstructionBefore(Push{RBP()}, it);
    b.insertInstructionBefore(Mov{RBP(), RSP()}, it);
    i32 variable_and_spill_size = (f->variable_slots + f->spill_slots) * 8;
    b.insertInstructionBefore(Sub{RSP(), Immediate(variable_and_spill_size, 64)}, it);

    b.setInsertionBB(*f->exit_block);
    auto end_it = --f->exit_block->instructions.end();
    b.insertInstructionBefore(Mov{RSP(), RBP()}, end_it);
    b.insertInstructionBefore(Pop{RBP()}, end_it);
  }
}

} // namespace X86