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

TEST(ReversePostOrder, BasicTest) {
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

  LiveVariables::runPipeline(builder.mod);
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

  LiveVariables::runPipeline(builder.mod);

  std::cerr << builder.cur_function->dbgString();
  ASSERT_EQ(builder.cur_function->live_intervals.at(0).toString(), "[0;24]");
}

// Value is clobbered along one of the paths. It should still be live after the
// join
TEST(LiveAnalysis, DiamondTest) {
  auto [builder, mod, function, start_bb] = getBuilder();
  VRegSpawner spawner;

  auto &bb_2 = builder.addBasicBlock();
  auto &bb_3 = builder.addBasicBlock();
  auto &bb_4 = builder.addBasicBlock();

  builder.addInstruction(Mov{spawner.spawnVReg(), Immediate(5)});
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
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.getVReg(0)});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(mod.get());

  std::cerr << builder.cur_function->dbgString();

  ASSERT_EQ(builder.cur_function->live_intervals.at(0).toString(),
            std::format("[0;{}]", (countInstructions(*function) - 2) *
                                      LiveVariables::ENUMERATION_GAP));
}

// A live interval of a variable that's in a loop should be the whole loop block
TEST(LiveAnalysis, LoopTest) {
  auto [builder, mod, function, cond] = getBuilder();
  VRegSpawner spawner;

  auto &body = builder.addBasicBlock("body");
  auto &after = builder.addBasicBlock("after");

  auto counter = spawner.spawnVReg();
  builder.addInstruction(Mov{counter, Immediate(5)});
  builder.addInstruction(Sub{counter, Immediate(1)});
  builder.addInstruction(Cmp{counter, Immediate(0)});
  builder.addInstruction(Jz{after});
  builder.addInstruction(Jmp{body});

  builder.setInsertionPoint(body);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  auto loop_start_idx = countInstructions(*function) - 1;
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  auto interesting_register = spawner.spawnVReg();
  builder.addInstruction(Mov{interesting_register, spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Jmp{*cond});
  auto loop_end_idx = countInstructions(*function) - 1;

  builder.setInsertionPoint(after);
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Mov{spawner.spawnVReg(), spawner.spawnVReg()});
  builder.addInstruction(Ret{});

  LiveVariables::runPipeline(mod.get());

  std::cerr << builder.cur_function->dbgString();

  ASSERT_EQ(
      builder.cur_function->live_intervals.at(interesting_register.vid)
          .toString(),
      std::format("[{};{}]", loop_start_idx * LiveVariables::ENUMERATION_GAP, loop_end_idx * LiveVariables::ENUMERATION_GAP));
}

} // namespace X86