#include "backend/x86/basicblock.hpp"
#include "backend/x86/operands.hpp"
#include "backend/x86/registers.hpp"
#include <backend/x86/livevariables.hpp>
#include <stack>

namespace X86 {

struct LiveInterval {
  u32 begin;
  u32 end;
};

static void compute_live_intervals(Function& f) {
}

static void reenumerate(Function& f) {
  static constexpr u8 ENUMERATION_GAP = 4;
  u32 index = 0;
  for (BasicBlock* bb : f.rpo) {
    for (Instruction &instr : bb->instructions) {
      instr.number = (index++) * ENUMERATION_GAP;
    }
  }
}

static void dfs(Function& f, BasicBlock& cur, std::set<BasicBlock*>& visited) {
  visited.insert(&cur);
  for (BasicBlock* succ : cur.succs) {
    if (!visited.contains(succ)) {
      dfs(f, *succ, visited);
    }
  }
  f.rpo.push_back(&cur);
}

static void reorderBBs(Function &f) {
  f.rpo.clear();
  std::set<BasicBlock*> visited;
  dfs(f, *f.entry_block, visited);
  std::reverse(f.rpo.begin(), f.rpo.end());
}

static std::set<u32> computeLiveOut(BasicBlock &bb) {
  std::set<u32> res;
  for (BasicBlock *succ : bb.succs) {
    for (u32 uevar : succ->ue_var) {
      res.insert(uevar);
    }
    for (u32 lo : succ->live_out) {
      if (!succ->var_kill.contains(lo)) {
        res.insert(lo);
      }
    }
  }
  return res;
}

void LiveVariables::compute(Function &f) {
  // Gathering initial information
  for (auto &bb : f.bbs) {
    bb.ue_var.clear();
    bb.live_out.clear();
    bb.var_kill.clear();

    for (auto &instr : bb.instructions) {
      auto reads = instr.getReadRegs();
      auto writes = instr.getWriteRegs();
      for (auto &read : reads) {
        if (read.isVirtual() && !bb.var_kill.contains(read.vid)) {
          bb.ue_var.insert(read.vid);
        }
      }
      for (auto &write : writes) {
        if (write.isVirtual()) {
          bb.var_kill.insert(write.vid);
        }
      }
    }
  }

  // Solving the equation

  bool changed = true;
  while (changed) {
    changed = false;
    for (BasicBlock &bb : f.bbs) {
      std::set<u32> new_live_out = computeLiveOut(bb);
      if (new_live_out != bb.live_out) {
        bb.live_out = new_live_out;
        changed = true;
      }
    }
  }
}

void LiveVariables::computeCFG(Function &f) {
  if (f.bbs.empty()) {
    return;
  }

  for (BasicBlock &cur : f.bbs) {
    for (auto &instr : cur.instructions) {
      if (instr.isJump()) {
        Label label = std::get<Label>(instr.op_infos[0].op);
        label.bb.preds.insert(&cur);
        cur.succs.insert(&label.bb);
      }
    }
  }
}

void LiveVariables::computeCFG(std::unique_ptr<Module> mod) {
  for (const auto &f : mod->functions) {
    computeCFG(*f);
    compute(*f);
    reorderBBs(*f);
    reenumerate(*f);
  }
}

} // namespace X86