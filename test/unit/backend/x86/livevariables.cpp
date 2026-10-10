#include <testutil.hpp>

#include <backend/x86/builder.hpp>
#include <backend/x86/livevariables.hpp>

namespace X86 {

class VRegSpawner {
  u32 counter = 0;

  std::vector<Register> spawned_registers;

public:
  Register spawnVReg() {
    spawned_registers.push_back(
        Register{RegisterType::NONE, 64, static_cast<i32>(counter++)});
    return spawned_registers.back();
  }

  Register getVReg(u32 idx) {
    assert(idx < spawned_registers.size());
    return spawned_registers[idx];
  }
};

usize countInstructions(Function &f) {
  usize total = 0;
  for (const auto &bb : f.bbs) {
    total += bb.instructions.size();
  }
  return total;
}

std::tuple<Builder, std::unique_ptr<Module>, Function *, BasicBlock *>
getBuilder() {
  auto mod = std::make_unique<Module>();
  Builder builder(mod.get());
  Function &function = builder.addFunction(Function("test", Linkage::EXTERN));
  builder.setBBInsertionPoint(function);
  BasicBlock &bb = builder.addBasicBlock();
  function.entry_block = &bb;
  builder.setInsertionPoint(bb);
  return {builder, std::move(mod), &function, &bb};
}

TEST(CFGComputation, BasicTest) {
  auto [builder, mod, function, start_bb] = getBuilder();
  VRegSpawner spawner;

  auto &bb_2 = builder.addBasicBlock();
  auto &bb_3 = builder.addBasicBlock();

  builder.addInstruction(Mov{spawner.spawnVReg(), Immediate(5)});
  builder.addInstruction(Jmp{bb_2});
  builder.setInsertionPoint(bb_2);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Jmp{bb_3});
  builder.setInsertionPoint(bb_3);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.getVReg(0)});
  builder.addInstruction(Ret{});

  LiveVariables::computeCFG(*builder.cur_function);

  ASSERT_EQ(start_bb->succs, std::set<BasicBlock *>{&bb_2});
  ASSERT_EQ(bb_2.succs, std::set<BasicBlock *>{&bb_3});
  ASSERT_EQ(bb_3.succs, std::set<BasicBlock *>{});
  ASSERT_EQ(start_bb->preds, std::set<BasicBlock *>{});
  ASSERT_EQ(bb_2.preds, std::set<BasicBlock *>{start_bb});
  ASSERT_EQ(bb_3.preds, std::set<BasicBlock *>{&bb_2});
}

TEST(LiveAnalysis, BasicTest) {
  auto [builder, mod, function, start_bb] = getBuilder();
  VRegSpawner spawner;

  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.getVReg(0)});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(builder.mod);

  ASSERT_EQ(builder.cur_function->live_intervals.at(0).toString(), "[0;16]");
  ASSERT_EQ(builder.cur_function->live_intervals.at(3).toString(), "[0;4]");
  ASSERT_EQ(builder.cur_function->live_intervals.at(4).toString(), "[8;8]");
}

TEST(LiveAnalysis, ExtendsThroughBlocks) {
  auto [builder, mod, function, start_bb] = getBuilder();
  VRegSpawner spawner;

  auto &bb_2 = builder.addBasicBlock();
  auto &bb_3 = builder.addBasicBlock();

  auto& live = builder.addInstruction(Mov{spawner.spawnVReg(), Immediate(5)});
  builder.addInstruction(Jmp{bb_2});
  builder.setInsertionPoint(bb_2);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Jmp{bb_3});
  builder.setInsertionPoint(bb_3);
  auto& dead = builder.addInstruction(Mov{spawner.spawnVReg(), spawner.getVReg(0)});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(builder.mod);

  std::cerr << builder.cur_function->dbgString();

  LiveInterval expected{live.number, dead.number};
  ASSERT_EQ(builder.cur_function->live_intervals.at(0).toString(), expected.toString());
}

// Value is clobbered along one of the paths. It should still be live after the
// join
TEST(LiveAnalysis, DiamondTest) {
  auto [builder, mod, function, start_bb] = getBuilder();
  VRegSpawner spawner;

  auto &bb_2 = builder.addBasicBlock();
  auto &bb_3 = builder.addBasicBlock();
  auto &bb_4 = builder.addBasicBlock();

  auto& live = builder.addInstruction(Mov{spawner.spawnVReg(), Immediate(5)});
  builder.addInstruction(Jz{bb_2});
  builder.addInstruction(Jmp{bb_3});

  builder.setInsertionPoint(bb_2);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Jmp{bb_4});

  builder.setInsertionPoint(bb_3);
  builder.addInstruction(Mov{spawner.getVReg(0), Immediate(0)});
  builder.addInstruction(Jmp{bb_4});

  builder.setInsertionPoint(bb_4);
  auto& dead = builder.addInstruction(Mov{spawner.spawnVReg(), spawner.getVReg(0)});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(mod.get());

  std::cerr << builder.cur_function->dbgString();

  LiveInterval expected{live.number, dead.number};
  ASSERT_EQ(builder.cur_function->live_intervals.at(0).toString(), expected.toString());
}

// A live interval of a variable that's in a loop should extend to the end of the loop 
TEST(LiveAnalysis, LoopTest) {
  auto [builder, mod, function, start] = getBuilder();
  VRegSpawner spawner;

  auto &cond = builder.addBasicBlock("cond");
  auto &body = builder.addBasicBlock("body");
  auto &after = builder.addBasicBlock("after");

  auto counter = spawner.spawnVReg();
  auto &live = builder.addInstruction(Mov{counter, Immediate(5)});
  builder.addInstruction(Jmp{cond});
  builder.setInsertionPoint(cond);
  builder.addInstruction(Sub{counter, Immediate(1)});
  builder.addInstruction(Cmp{counter, Immediate(0)});
  builder.addInstruction(Jz{after});
  builder.addInstruction(Jmp{body});

  builder.setInsertionPoint(body);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), counter});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  auto &dead = builder.addInstruction(Jmp{cond});

  builder.setInsertionPoint(after);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(mod.get());

  std::cerr << builder.cur_function->dbgString();

  LiveInterval expected{live.number, dead.number};
  ASSERT_EQ(builder.cur_function->live_intervals.at(counter.vid).toString(), expected.toString());
}

} // namespace X86