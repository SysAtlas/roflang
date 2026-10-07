#include <target/x86/instructions.hpp>
#include <target/x86/x86.hpp>
#include <unordered_map>

namespace X86 {

std::unordered_map<i32, Immediate> ImmediatePool;

const Immediate &Immediate::get(i32 Value) {
  if (auto Found = ImmediatePool.find(Value); Found != ImmediatePool.end()) {
    return Found->second;
  }
  ImmediatePool.insert({Value, Immediate{Value}});
  return ImmediatePool.at(Value);
}

std::string Module::emitHeader() {
  std::string Res = ".intel_syntax noprefix\n\n.text\n";

  if (!Functions.empty()) {
    Res += ".global";
  }
  for (const auto &Fn : Functions) {
    if (Fn->Linkage == LinkageType::Extern) {
      Res += std::format(" {},", Fn->Name);
    }
  }
  if (!Functions.empty()) {
    Res.pop_back();
    Res += '\n';
  }
  Res += '\n';

  return Res;
}
std::string Module::toString() {
  std::string Result = emitHeader();
  for (const auto &Fn : Functions) {
    Result += Fn->toString();
  }
  return Result;
}
Function &Module::addFunction(std::unique_ptr<Function> Fn) {
  Functions.emplace_back(std::move(Fn));
  return *Functions.back();
}
std::string BasicBlock::toString() const {
  std::string Res = std::format("{}:\n", Name);
  for (const auto &Instruction : Instructions) {
    Res += "  " + Instruction->toString() + '\n';
  }
  return Res;
}
BasicBlock::BasicBlock(std::string_view Name) : Name{Name} {}

std::string Function::toString() const {
  std::string Res;
  for (const BasicBlock &BB : BBs) {
    Res += BB.toString() + '\n';
  }
  return Res;
}
Function::Function(std::string_view Name, LinkageType Linkage)
    : Name{Name}, Linkage{Linkage} {}

BasicBlock &Function::addBasicBlock(BasicBlock &&BB, BasicBlock *InsertAfter) {
  if (InsertAfter == nullptr) {
    BBs.push_front(std::move(BB));
    return BBs.front();
  }
  return *BBs.insert(
    ++std::ranges::find_if(
      BBs,
      [InsertAfter](const BasicBlock &Arg) { return &Arg == InsertAfter; }),
    std::move(BB));
}

} // namespace X86