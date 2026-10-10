#include <backend/x86/builder.hpp>

namespace X86 {

Function &Builder::addFunction(Function &&f) {
  assert(mod != nullptr);
  return mod->addFunction(std::make_unique<Function>(std::move(f)));
}
BasicBlock &Builder::addBasicBlock(BasicBlock &&bb, BasicBlock *insert_after) {
  assert(cur_function != nullptr && "builder not set to a function!");
  return cur_function->addBasicBlock(std::move(bb), insert_after);
}
BasicBlock &Builder::addBasicBlock(const std::string &name,
                                   BasicBlock *insert_after) {
  if (block_name_counter.find(name) == block_name_counter.end()) {
    block_name_counter[name] = 0;
  }
  return addBasicBlock(
      BasicBlock(std::format("{}_{}", name, ++block_name_counter[name])),
      insert_after);
}
BasicBlock &Builder::getBasicBlock(std::string_view name) {
  for (BasicBlock &bb : cur_function->bbs) {
    if (bb.name == name) {
      return bb;
    }
  }
  std::cerr << "No such basic block!" << '\n';
  abort();
}
Function &Builder::getFunction(std::string_view name) {
  for (const auto &f : mod->functions) {
    if (f->name == name) {
      return *f;
    }
  }
  std::cerr << "No such function in the module!" << '\n';
  abort();
}
BasicBlock &Builder::getEndBlock() { return getBasicBlock("end"); }
BasicBlock *Builder::getNextBlock() {
  auto res = std::ranges::find_if(
      cur_function->bbs, [this](BasicBlock &bb) { return &bb == cur_bb; });
  if (res == cur_function->bbs.end()) {
    return nullptr;
  }
  ++res;
  return (res != cur_function->bbs.end()) ? &*res : nullptr;
}
void Builder::setInsertionPoint(BasicBlock &bb) { cur_bb = &bb; }
void Builder::setBBInsertionPoint(Function &f) { cur_function = &f; }
Builder::Builder(Module *mod) : mod{mod} {}

} // namespace X86