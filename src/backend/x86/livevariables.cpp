#include <backend/x86/basicblock.hpp>
#include <backend/x86/function.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/livevariables.hpp>
#include <backend/x86/module.hpp>
#include <backend/x86/operands.hpp>
#include <backend/x86/registers.hpp>
#include <cassert>

namespace X86 {

// A variable's live range starts:
// at it's first access
// or if it's in live_in, at beginning of BB

// A variable's live range ends:
// at it's last access (write, since we need to allocate the variable anyway)
// or if it's in live_out, at end of BB

// note that we also need to check all live_outs of basic blocks.

// BB1: liveins: {}
// 1. v1 <- 5
// 2. v2 <- 6
// BB2: liveins: {v1, v2}
// 3. v3 <- v1 + v2
// 4. v4 <- v3 + 2
// 5. v3 <- v3 + 1
std::set<u32> LiveVariableAnalysis::compute_live_ins(BasicBlock *bb) {
  std::set<u32> live_ins;
  for (u32 used_before_def : bb->ue_var) {
    live_ins.insert(used_before_def);
  }
  for (u32 live_out : bb->live_out) {
    if (!bb->var_kill.contains(live_out)) {
      live_ins.insert(live_out);
    }
  }
  return live_ins;
}

// This is the info regalloc needs.
void LiveVariableAnalysis::computeLiveIntervals(Function &f) {
  f.live_int_map.clear();
  std::unordered_map<u32, LiveInterval>& live_int_map = f.live_int_map;

  for (BasicBlock *bb : f.rpo) {
    // compute set of live ins
    std::set<u32> live_ins = compute_live_ins(bb);

    if (bb->instructions.empty()) {
      continue;
    }
    u32 first_instr_number = bb->instructions.front()->number;
    u32 last_instr_number = bb->instructions.back()->number;
    for (auto &instr : bb->instructions) {
      for (OpInfo &op : instr->op_infos) {
        if (!op.is<Register>()) {
          continue;
        }
        Register access = op.get<Register>();

        // physical registers are already allocated
        if (!access.isVirtual()) {
          continue;
        }

        assert(access.vid != -1);

        // If we have no entry for this variable yet, create one
        if (!live_int_map.contains(access.vid)) {
          live_int_map.insert({access.vid, LiveInterval{static_cast<u32>(access.vid)}});
        }

        // If interval is in live-ins, it must live through whole block
        bool is_live_in = live_ins.contains(access.vid);
        u32 start = is_live_in ? first_instr_number : instr->number;
        u32 end = bb->live_out.contains(access.vid) ? last_instr_number
                                                    : instr->number;
        LiveInterval& li = live_int_map.at(access.vid);
        li.start = std::min(li.start, start);
        li.end = std::max(li.end, end);
        li.variable_id = access.vid;
      }
    }
    for (u32 live_out_id : bb->live_out) {
      if (!live_int_map.contains(live_out_id)) {
        live_int_map.insert({live_out_id, LiveInterval{live_out_id}});
      }
      LiveInterval& li = live_int_map.at(live_out_id);
      li.end = std::max(li.end, last_instr_number);
      li.variable_id = live_out_id;
    }
  }
}

void LiveVariableAnalysis::reenumerate(Function &f) {
  u32 index = 0;
  for (BasicBlock *bb : f.rpo) {
    for (auto &instr : bb->instructions) {
      instr->number = (index++) * ENUMERATION_GAP;

      auto clobber_set = instr->getClobberSet();
      for (auto& op : instr->op_infos) {
        if (op.is<Register>()) {
          Register reg = op.get<Register>();
          if (!reg.isVirtual()) {
            clobber_set.push_back(reg);
          }
        }
      }
      if (!clobber_set.empty()) {
        clobber_sets.emplace_back(instr->number, clobber_set);
      }
    }
  }
}

void LiveVariableAnalysis::dfs(Function &f, BasicBlock &cur,
                        std::set<BasicBlock *> &visited) {
  visited.insert(&cur);
  for (BasicBlock *succ : cur.succs) {
    if (!visited.contains(succ)) {
      dfs(f, *succ, visited);
    }
  }
  f.rpo.push_back(&cur);
}

void LiveVariableAnalysis::reorderBBs(Function &f) {
  f.rpo.clear();
  std::set<BasicBlock *> visited;
  assert(f.entry_block != nullptr && "Missing entry block!");
  dfs(f, *f.entry_block, visited);
  std::reverse(f.rpo.begin(), f.rpo.end());
}

std::set<u32> LiveVariableAnalysis::computeLiveOut(BasicBlock &bb) {
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

// Solves data flow equations
void LiveVariableAnalysis::computeLiveness(Function &f) {
  // Gathering initial information
  for (auto &bb : f.bbs) {
    bb.ue_var.clear();
    bb.live_out.clear();
    bb.var_kill.clear();

    for (auto &instr : bb.instructions) {
      auto reads = instr->getReadRegs();
      auto writes = instr->getWriteRegs();
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

void LiveVariableAnalysis::computeCFG(Function &f) {
  if (f.bbs.empty()) {
    return;
  }

  for (BasicBlock &cur : f.bbs) {
    assert(cur.instructions.back()->isJump() ||
           cur.instructions.back()->isRet() &&
               "Basic blocks should always end with a jmp or ret for control "
               "flow computation!");
    for (auto &instr : cur.instructions) {
      if (instr->isJump()) {
        Label label = std::get<Label>(instr->op_infos[0].op);
        label.bb.preds.insert(&cur);
        cur.succs.insert(&label.bb);
      }
    }
  }
}

void LiveVariableAnalysis::computeClobberSets(Function &f) {
  for (auto& li : f.live_int_map) {
    LiveInterval& interval = li.second;
    for (auto& cs : clobber_sets) {
      u32 cs_instr_number = cs.first;
      if (cs_instr_number >= interval.start && cs_instr_number <= interval.end) {
        interval.clobber_set.insert(cs.second.begin(), cs.second.end());
      }
    }
  }
}

void LiveVariableAnalysis::runPass(Module *mod) {
  LiveVariableAnalysis analysis;
  for (const auto &f : mod->getFunctionDefs()) {
    analysis.computeCFG(*f);
    analysis.computeLiveness(*f);
    analysis.reorderBBs(*f);
    analysis.reenumerate(*f);
    analysis.computeLiveIntervals(*f);
    analysis.computeClobberSets(*f);
  }
}

std::string LiveInterval::toString() const {
  std::string res = std::format("[{};{}] <= v{}", start, end, variable_id);
  res += " clobber set: [";
  for (auto& clobber : clobber_set) {
    res += std::format("{}, ", X86::toString(clobber));
  }
  if (!clobber_set.empty()) {
    res.pop_back();
    res.pop_back();
  }
  res += "]";
  if (alloc_reg) {
    res += std::format(" => {}", variable_id, X86::toString(*alloc_reg));
  }
  return res;
}
} // namespace X86