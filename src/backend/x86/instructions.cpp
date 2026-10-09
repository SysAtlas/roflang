#include <backend/x86/basicblock.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/operands.hpp>

namespace X86 {

struct OpcodeInfo {
  std::string name;
  std::vector<Register> clobber_set;
};

static const std::unordered_map<Opcode, OpcodeInfo> OPCODE_TABLE = {{
    {Opcode::MOV, {"mov", {}}},
    {Opcode::PUSH, {"push", {}}},
    {Opcode::POP, {"pop", {}}},
    {Opcode::XCHG, {"xchg", {}}},
    {Opcode::ADD, {"add", {}}},
    {Opcode::SUB, {"sub", {}}},
    {Opcode::IMUL, {"imul", {}}},
    {Opcode::IDIV, {"idiv", {RAX(), RDX()}}},
    {Opcode::MOVSX, {"movsx", {}}},
    {Opcode::CQO, {"cqo", {RDX()}}},
    {Opcode::XOR, {"xor", {}}},
    {Opcode::CMP, {"cmp", {}}},
    {Opcode::SETZ, {"setz", {}}},
    {Opcode::SETNE, {"setne", {}}},
    {Opcode::SETLE, {"setle", {}}},
    {Opcode::SETL, {"setl", {}}},
    {Opcode::SETGE, {"setge", {}}},
    {Opcode::SETG, {"setg", {}}},
    {Opcode::JMP, {"jmp", {}}},
    {Opcode::CALL,
     {"call", {RAX(), RCX(), RDX(), RSI(), RDI(), R8(), R9(), R10(), R11()}}},
    {Opcode::RET, {"ret", {}}},
    {Opcode::JZ, {"jz", {}}},
    {Opcode::JG, {"jg", {}}},
    {Opcode::JGE, {"jge", {}}},
    {Opcode::JL, {"jl", {}}},
    {Opcode::JLE, {"jle", {}}},
    {Opcode::JNE, {"jne", {}}},
}};

std::vector<Register> Instruction::getClobberSet() const {
  return OPCODE_TABLE.at(opcode).clobber_set;
}

std::string Instruction::toString() const {
  std::string res = OPCODE_TABLE.at(opcode).name;
  for (const auto &op_info : op_infos) {
    res += ' ' + op_info.toString() + ',';
  }
  if (!op_infos.empty()) {
    res.pop_back();
  }
  return res;
}

bool Instruction::isJump() const {
  switch (opcode) {
  case Opcode::JMP:
  case Opcode::JZ:
  case Opcode::JNE:
  case Opcode::JL:
  case Opcode::JLE:
  case Opcode::JG:
  case Opcode::JGE:
    return true;
  default:
    return false;
  }
}

std::vector<Register> Instruction::getReadRegs() {
  std::vector<Register> res;
  for (auto op_info : op_infos) {
    if (op_info.reads()) { 
      // TODO: check type
      res.push_back(std::get<Register>(op_info.op));
    }
  }
  return res;
}

std::vector<Register> Instruction::getWriteRegs() {
  std::vector<Register> res;
  for (auto op_info : op_infos) {
    if (op_info.writes()) { 
      // TODO: check type
      res.push_back(std::get<Register>(op_info.op));
    }
  }
  return res;
}

Jge::Jge(BasicBlock &dst)
    : Instruction{Opcode::JGE, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jg::Jg(BasicBlock &dst)
    : Instruction{Opcode::JG, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jle::Jle(BasicBlock &dst)
    : Instruction{Opcode::JLE, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jl::Jl(BasicBlock &dst)
    : Instruction{Opcode::JL, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jne::Jne(BasicBlock &dst)
    : Instruction{Opcode::JNE, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jz::Jz(BasicBlock &dst)
    : Instruction{Opcode::JZ, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Jmp::Jmp(BasicBlock &dst)
    : Instruction{Opcode::JMP, {OpInfo{Label{dst}, AccessType::SRC}}} {}
Call::Call(Function &dst)
    : Instruction{Opcode::CALL, {OpInfo{FunctionRef{dst}, AccessType::SRC}}} {}
Ret::Ret() : Instruction{Opcode::RET, {}} {}
SetGE::SetGE(const Register &dst)
    : Instruction{Opcode::SETGE, {OpInfo{dst, AccessType::DST}}} {}
SetG::SetG(const Register &dst)
    : Instruction{Opcode::SETG, {OpInfo{dst, AccessType::DST}}} {}
SetLE::SetLE(const Register &dst)
    : Instruction{Opcode::SETLE, {OpInfo{dst, AccessType::DST}}} {}
SetL::SetL(const Register &dst)
    : Instruction{Opcode::SETL, {OpInfo{dst, AccessType::DST}}} {}
SetNE::SetNE(const Register &dst)
    : Instruction{Opcode::SETNE, {OpInfo{dst, AccessType::DST}}} {}
SetZ::SetZ(const Register &dst)
    : Instruction{Opcode::SETZ, {OpInfo{dst, AccessType::DST}}} {}
} // namespace X86