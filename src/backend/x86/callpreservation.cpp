#include "backend/x86/basicblock.hpp"
#include "backend/x86/function.hpp"
#include "backend/x86/instructions.hpp"
#include "backend/x86/registers.hpp"
#include <backend/x86/builder.hpp>
#include <backend/x86/callpreservation.hpp>

namespace X86 {

// Maps function names to registers they clobber. Functions with no definition
// clobber everything

void CallPreservation::patchFunctions() {
  for (Function *f : mod->getFunctionDefs()) {
    auto &regs_to_save = should_preserve.at(f);
    builder->setFunction(*f);

    // Push registers to save
    BasicBlock &start_bb = builder->getBasicBlock(f->name);
    builder->setInsertionBB(start_bb);
    auto it = builder->cur_bb->instructions.begin();
    // Preamble (push rbp, mov rbp, rsp)
    std::advance(it, 2);
    for (const Register &reg_to_save : regs_to_save) {
      builder->insertInstructionBefore(Push{reg_to_save}, it);
    }

    // Pop registers to save
    BasicBlock &end_bb = *builder->cur_function->exit_block;
    builder->setInsertionBB(end_bb);
    auto ret_it = --builder->cur_bb->instructions.end();
    for (auto rit = regs_to_save.rbegin(), rie = regs_to_save.rend();
         rit != rie; ++rit) {
      ret_it =
          builder->insertInstructionBefore(Pop{*rit}, ret_it);
    }
  }
}

void CallPreservation::patchCallSites() {
  for (Function *f : mod->getFunctionDefs()) {
    builder->setFunction(*f);
    for (BasicBlock &bb : f->bbs) {
      builder->setInsertionBB(bb);
      for (auto it = bb.instructions.begin(), ie = bb.instructions.end();
           it != ie; ++it) {
        Instruction &instr = **(it);
        if (instr.opcode == Opcode::CALL) {
          auto &regs_to_save =
              preserve_before_call.at(&instr.op_infos[0].get<FunctionRef>().fn);
          u8 alignment = 8; // PREAMBLE TAKES 8 BYTES
          for (const Register& reg_to_save : regs_to_save) {
            builder->insertInstructionBefore(
                Push{reg_to_save,}, it);
            alignment += 8;
          }
          // Stack should be aligned to 16 bytes before a call
          bool should_align = alignment % 16 != 0;
          if (should_align) {
            builder->insertInstructionBefore(Sub{RSP(), Immediate(8)}, it);
          }
          ++it;
          if (should_align) {
            builder->insertInstructionBefore(Add{RSP(), Immediate(8)}, it);
          }
          for (auto rit = regs_to_save.rbegin(), rie = regs_to_save.rend();
               rit != rie; ++rit) {
            builder->insertInstructionBefore(Pop{*rit}, it);
          }
        }
      }
    }
  }
}

void CallPreservation::initClobberMap() {
  for (auto &f : mod->functions) {
    preserve_before_call.insert({f.get(), {}});
    should_preserve.insert({f.get(), {}});
    auto &bc = preserve_before_call.at(f.get());
    auto &sp = should_preserve.at(f.get());

    if (f->is_decl) {
      bc.insert(getCallerSaved().begin(), getCallerSaved().end());
      continue;
    }

    for (const BasicBlock &bb : f->bbs) {
      for (const auto &instr : bb.instructions) {
        for (const auto &write_reg : instr->getWriteRegs()) {
          if (isCallerSaved(write_reg)) {
            bc.insert(write_reg);
          } else {
            sp.insert(write_reg);
          }
        }
      }
    }
  }
}

CallPreservation::CallPreservation(Module *mod)
    : mod{mod}, builder{std::make_unique<Builder>(mod)} {}

void CallPreservation::runPass(Module *mod) {
  CallPreservation pass(mod);
  pass.initClobberMap();
  pass.patchCallSites();
  pass.patchFunctions();
}

} // namespace X86