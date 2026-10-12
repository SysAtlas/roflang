#include "frontend/ast.hpp"
#include <backend/x86/builder.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/isel.hpp>
#include <backend/x86/module.hpp>
#include <backend/x86/operands.hpp>
#include <backend/x86/registers.hpp>
#include <common.hpp>
#include <memory>
#include <optional>

namespace X86 {

class VirtualRegisterTracker {
  u32 reg_counter = 0;
  Function *function;

  std::unordered_map<i32, std::string> offset_to_name;
  std::unordered_map<std::string, i32> name_to_offset;
  std::unordered_map<std::string, u8> name_to_size;

public:
  i32 getDisplacement(const std::string &name) {
    return name_to_offset.at(name);
  }

  u8 getSize(const std::string &name) { return name_to_size.at(name); }

  Register getVirtualRegister(u32 size) {
    return Register{RegisterType::NONE, size, static_cast<i32>(reg_counter++)};
  }

  i32 addVar(const std::string &name, u8 size) {
    ++function->variable_slots;
    i32 new_offset = -function->variable_slots * 8;
    offset_to_name[new_offset] = name;
    name_to_offset[name] = new_offset;
    name_to_size[name] = size;
    return new_offset;
  }

  VirtualRegisterTracker(Function *function) : function{function} {}
};

void ISel::logError(std::string_view err_msg) {
  std::cerr << err_msg << '\n';
  exit(1);
}

Register ISel::select(const AST::NumberExpr &number_expr) {
  Register reg = tracker->getVirtualRegister(32);
  builder->addInstruction(Mov{reg, Immediate(number_expr.value, 64)});
  return reg;
}

Register ISel::select(const AST::VariableExpr &var_expr) {
  Register res = tracker->getVirtualRegister(tracker->getSize(var_expr.name));
  builder->addInstruction(
      Mov{res, MemOp{RBP(),
                     Immediate(tracker->getDisplacement(var_expr.name), 64)}});
  return res;
}

void ISel::compareSetHelper(BinOp bin_op, const Register &dst) {
  switch (bin_op) {
  case BinOp::LT:
    builder->addInstruction(SetL{dst});
    break;
  case BinOp::LEQ:
    builder->addInstruction(SetLE{dst});
    break;
  case BinOp::GT:
    builder->addInstruction(SetG{dst});
    break;
  case BinOp::GEQ:
    builder->addInstruction(SetGE{dst});
    break;
  case BinOp::EQ:
    builder->addInstruction(SetZ{dst});
    break;
  case BinOp::NEQ:
    builder->addInstruction(SetNE{dst});
    break;
  default:
    std::cerr << "Not a comparison binop!" << '\n';
    abort();
  }
  return;
}

void ISel::binOpHelper(BinOp op, const Register &lhs, const Register &rhs) {
  static const std::array CMP_OPS = {BinOp::EQ, BinOp::NEQ, BinOp::LT,
                                     BinOp::GT, BinOp::GEQ, BinOp::LEQ};

  if (op == BinOp::ADD) {
    builder->addInstruction(Add{lhs, rhs});
  } else if (op == BinOp::SUB) {
    builder->addInstruction(Sub{lhs, rhs});
  } else if (op == BinOp::MUL) {
    builder->addInstruction(IMul{lhs, rhs});
  } else if (std::ranges::find(CMP_OPS, op) != CMP_OPS.end()) {
    builder->addInstruction(Cmp{lhs, rhs});
    compareSetHelper(op, lhs.getSubregister(8));
    builder->addInstruction(MovSX{lhs, lhs.getSubregister(8)});
  } else {
    logError("invalid binary operator");
    abort();
  }
}

Register ISel::select(const AST::BinaryExpr &bin_expr) {
  Register lhs = select(bin_expr.lhs);
  Register rhs = select(bin_expr.rhs);

  BinOp op = bin_expr.op;

  if (op == BinOp::DIV || op == BinOp::MOD) {
    builder->addInstruction(Mov{RAX().getSubregister(lhs.size), lhs});
    if (rhs.size == 64) {
      builder->addInstruction(Cqo{});
    } else if (rhs.size == 32) {
      builder->addInstruction(Cdq{});
    } else if (rhs.size <= 16) {
      builder->addInstruction(Cwd{});
    } else {
      abort();
    }
    builder->addInstruction(IDiv{rhs});
    auto res = tracker->getVirtualRegister(rhs.size);
    if (op == BinOp::MOD) {
      builder->addInstruction(Mov{res, RDX().getSubregister(res.size)});
    } else {
      builder->addInstruction(Mov{res, RAX().getSubregister(res.size)});
    }
    return res;
  }
  const Register &new_reg = tracker->getVirtualRegister(lhs.size);
  builder->addInstruction(Mov{new_reg, lhs});
  binOpHelper(bin_expr.op, new_reg, rhs);
  return new_reg;
}

// Start very simple.
Register ISel::select(const AST::CallExpr &call_expr) {
  if (call_expr.args.size() > 6) {
    std::cerr << "Arguments over stack are not supported (yet!)" << '\n';
    abort();
  }
  std::vector<Register> arg_vregs;
  arg_vregs.reserve(call_expr.args.size());
  for (usize i = 0; i < call_expr.args.size(); ++i) {
    const auto &arg = call_expr.args[i];
    arg_vregs.emplace_back(select(arg));
  }
  const AST::Function* callee = function_map.at(call_expr.callee_name);
  for (usize i = 0; i < call_expr.args.size(); ++i) {
    u32 arg_size = callee->signature.args[i].type_info->size_in_bits;
    builder->addInstruction(Mov{argumentRegister(i).getSubregister(arg_size), arg_vregs[i]});
  }
  builder->addInstruction(Call{builder->getFunction(call_expr.callee_name)});
  u8 callee_return_size = callee->signature.return_type_info->size_in_bits;
  auto res = tracker->getVirtualRegister(callee_return_size);
  builder->addInstruction(Mov{res, RAX().getSubregister(callee_return_size)});
  return res;
}

Register ISel::select(const AST::Expr &expr) {
  return std::visit<Register>(
      overloaded{[this](const auto &arg) -> Register { return select(*arg); }},
      expr);
}

void ISel::select(const AST::Statement &stmt) {
  std::visit(overloaded{[this](const auto &arg) { select(*arg); }}, stmt);
}

void ISel::select(const AST::IfStatement &if_stmt) {
  const auto &cond_res = select(if_stmt.condition);
  builder->addInstruction(Cmp{cond_res, Immediate(0, cond_res.size)});
  BasicBlock &if_body = builder->addBasicBlock(".L_ifbody", builder->cur_bb);
  BasicBlock &after_if = builder->addBasicBlock(".L_afterif", &if_body);
  builder->addInstruction(Jz{after_if});
  builder->addInstruction(Jmp{if_body});
  builder->setInsertionBB(if_body);
  for (const auto &S : if_stmt.body) {
    select(S);
  }
  // note: in optimization pass - remove redundant jumps
  builder->addInstruction(Jmp{after_if});
  builder->setInsertionBB(after_if);
}

void ISel::select(const AST::ReturnStatement &return_stmt) {
  if (return_stmt.value.has_value()) {
    Register to_return = select(*return_stmt.value);
    if (to_return != returnRegister()) {
      builder->addInstruction(Mov{returnRegister().getSubregister(to_return.size), to_return});
    }
  }
  // note: in optimization pass - remove redundant jumps
  builder->addInstruction(Jmp{builder->getEndBlock()});
}

void ISel::select(const AST::LocalVarDeclStmt &stmt) {
  i32 new_var_offset = tracker->addVar(stmt.name, stmt.type_info->size_in_bits);
  Register expr_value = select(stmt.value);
  builder->addInstruction(
      Mov{MemOp{RBP(), Immediate(new_var_offset, 64)}, expr_value});
}

void ISel::select(const AST::AssignmentStatement &assignment_stmt) {
  const auto &rhs = select(assignment_stmt.value);
  i32 offset = tracker->getDisplacement(assignment_stmt.name);
  builder->addInstruction(Mov{MemOp{RBP(), Immediate(offset, 64)}, rhs});
}

void ISel::select(const AST::WhileStatement &while_stmt) {
  BasicBlock &cond_block =
      builder->addBasicBlock(".L_while_cond", builder->cur_bb);
  builder->addInstruction(Jmp{cond_block});
  BasicBlock &while_body = builder->addBasicBlock(".L_while_body", &cond_block);
  BasicBlock &after_while =
      builder->addBasicBlock(".L_after_while", &while_body);

  builder->setInsertionBB(cond_block);
  Register cond_res = select(while_stmt.cond);
  builder->addInstruction(Cmp{cond_res, Immediate(0, 64)});
  builder->addInstruction(Jz{after_while});
  builder->addInstruction(Jmp{while_body});
  builder->setInsertionBB(while_body);
  for (const auto &stmt : while_stmt.body) {
    select(stmt);
  }
  builder->addInstruction(Jmp{cond_block});
  builder->setInsertionBB(after_while);
}

Function &ISel::select(const AST::Function::Signature &signature, bool is_decl) {
  return builder->addFunction(Function(
      signature.name, signature.is_static ? Linkage::INTERNAL : Linkage::EXTERN,
      is_decl));
}

void ISel::select(const AST::Function &f_node) {
  Function &function = select(f_node.signature, f_node.is_decl);
  assert(function_map.find(function.name) == function_map.end());
  function_map[function.name] = &f_node;

  if (f_node.is_decl) {
    return;
  }

  tracker = std::make_unique<VirtualRegisterTracker>(&function);
  builder->setFunction(function);
  BasicBlock &start =
      builder->addBasicBlock(BasicBlock(builder->cur_function->name));
  function.entry_block = &start;
  builder->setInsertionBB(start);
  end_bb = &builder->addBasicBlock(
      BasicBlock{
          std::format("end_{}", function.name),
      },
      &start);
  function.exit_block = end_bb;

  if (f_node.signature.args.size() > 6) {
    std::cerr << "Arguments over stack are not supported (yet!)" << '\n';
    abort();
  }
  for (usize i = 0; i < f_node.signature.args.size(); ++i) {
    const auto &arg = f_node.signature.args[i];
    i32 arg_offset =
        tracker->addVar(arg.name ? *arg.name : std::format("unnamed_arg_{}", i),
                        arg.type_info->size_in_bits);
    builder->addInstruction(
        Mov{MemOp{RBP(), Immediate(arg_offset, 64)},
            argumentRegister(i).getSubregister(arg.type_info->size_in_bits)});
  }

  for (const auto &s : f_node.body) {
    select(s);
  }

  builder->setInsertionBB(*end_bb);
  builder->addInstruction(Ret{}, end_bb);
}

void ISel::select() {
  mod = std::make_unique<Module>();
  builder = std::make_unique<Builder>(mod.get());

  for (const auto &tli : module_tree->top_level_items) {
    std::visit(overloaded{[this](const auto &arg) { select(arg); }}, tli);
  }
}

ISel::ISel(std::unique_ptr<AST::Module> module_tree)
    : tracker{nullptr}, module_tree{std::move(module_tree)} {}

std::unique_ptr<Module> ISel::runPass(std::unique_ptr<AST::Module> ast) {
  ISel isel{std::move(ast)};
  isel.select();
  return std::move(isel.mod);
}

} // namespace X86