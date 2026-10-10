#include "frontend/ast.hpp"
#include <backend/x86/builder.hpp>
#include <backend/x86/instructions.hpp>
#include <backend/x86/isel.hpp>
#include <backend/x86/operands.hpp>
#include <backend/x86/registers.hpp>
#include <backend/x86/module.hpp>
#include <common.hpp>
#include <memory>
#include <optional>

namespace X86 {

class VirtualRegisterTracker {
  u32 reg_counter = 0;
  std::string function_name;

  std::unordered_map<u32, std::string> id_to_var_name;
  std::unordered_map<std::string, Register> var_name_to_id;

public:
  bool holdsVar(const Register &reg) {
    return id_to_var_name.find(reg.vid) != id_to_var_name.end();
  }

  Register findRegisterHoldingValue(const std::string &name) {
    return var_name_to_id.at(name);
  }

  Register getVirtualRegister(u32 size) {
    return Register{RegisterType::NONE, size, static_cast<i32>(reg_counter++)};
  }

  void addVar(const Register &reg, const std::string &name) {
    id_to_var_name[reg.vid] = name;
    var_name_to_id[name] = reg;
  }

  VirtualRegisterTracker(std::string_view function_name)
      : function_name{function_name} {}
};

void ISel::logError(std::string_view err_msg) {
  std::cerr << err_msg << '\n';
  exit(1);
}

Register ISel::select(const AST::NumberExpr &number_expr) {
  Register reg = tracker->getVirtualRegister(64);
  builder->addInstruction(Mov{reg, Immediate(number_expr.value)});
  return reg;
}

Register ISel::select(const AST::VariableExpr &var_expr) {
  Register res = tracker->findRegisterHoldingValue(var_expr.name);
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
    builder->addInstruction(Mov{RAX(), lhs});
    builder->addInstruction(Cqo{});
    builder->addInstruction(IDiv{rhs});
    auto res = tracker->getVirtualRegister(64);
    if (op == BinOp::MOD) {
      builder->addInstruction(Mov{res, RDX()});
    } else {
      builder->addInstruction(Mov{res, RAX()});
    }
    return res;
  }
  const Register &new_reg = tracker->getVirtualRegister(64);
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
  for (usize i = 0; i < call_expr.args.size(); ++i) {
    builder->addInstruction(Mov{argumentRegister(i), arg_vregs[i]});
  }
  builder->addInstruction(Call{builder->getFunction(call_expr.callee_name)});
  auto res = tracker->getVirtualRegister(64);
  builder->addInstruction(Mov{res, RAX()});
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
  builder->addInstruction(Cmp{cond_res, Immediate(0)});
  BasicBlock &if_body = builder->addBasicBlock(".L_ifbody", builder->cur_bb);
  BasicBlock &after_if = builder->addBasicBlock(".L_afterif", &if_body);
  builder->addInstruction(Jz{after_if});
  builder->addInstruction(Jmp{if_body});
  builder->setInsertionPoint(if_body);
  for (const auto &S : if_stmt.body) {
    select(S);
  }
  // note: in optimization pass - remove redundant jumps
  builder->addInstruction(Jmp{after_if});
  builder->setInsertionPoint(after_if);
}

void ISel::select(const AST::ReturnStatement &return_stmt) {
  if (return_stmt.value.has_value()) {
    Register to_return = select(*return_stmt.value);
    if (to_return != returnRegister()) {
      builder->addInstruction(Mov{returnRegister(), to_return});
    }
  }
  // note: in optimization pass - remove redundant jumps
  builder->addInstruction(Jmp{builder->getEndBlock()});
}

Register ISel::select(const AST::LocalVarDeclStmt &stmt) {
  Register expr_value = select(stmt.value);
  if (tracker->holdsVar(expr_value)) {
    Register res = tracker->getVirtualRegister(64);
    tracker->addVar(res, stmt.name);
    builder->addInstruction(Mov{res, expr_value});
    return res;
  }
  tracker->addVar(expr_value, stmt.name);
  return expr_value;
}

Register ISel::select(const AST::AssignmentStatement &assignment_stmt) {
  const auto &rhs = select(assignment_stmt.value);
  const auto &lhs = tracker->findRegisterHoldingValue(assignment_stmt.name);
  if (rhs == lhs) {
    return lhs;
  }
  builder->addInstruction(Mov{lhs, rhs});
  return lhs;
}

void ISel::select(const AST::WhileStatement &while_stmt) {
  BasicBlock &cond_block =
      builder->addBasicBlock(".L_while_cond", builder->cur_bb);
  builder->addInstruction(Jmp{cond_block});
  BasicBlock &while_body = builder->addBasicBlock(".L_while_body", &cond_block);
  BasicBlock &after_while =
      builder->addBasicBlock(".L_after_while", &while_body);

  builder->setInsertionPoint(cond_block);
  Register cond_res = select(while_stmt.cond);
  builder->addInstruction(Cmp{cond_res, Immediate(0)});
  builder->addInstruction(Jz{after_while});
  builder->addInstruction(Jmp{while_body});
  builder->setInsertionPoint(while_body);
  for (const auto &stmt : while_stmt.body) {
    select(stmt);
  }
  builder->addInstruction(Jmp{cond_block});
  builder->setInsertionPoint(after_while);
}

Function &ISel::select(const AST::Signature &signature, bool is_decl) {
  return builder->addFunction(Function(signature.name, signature.is_static
                                                           ? Linkage::INTERNAL
                                                           : Linkage::EXTERN, is_decl));
}

void ISel::select(const AST::Function &f_node) {
  Function &function = select(*f_node.proto, f_node.is_decl);

  if (f_node.is_decl) {
    return;
  }

  tracker = std::make_unique<VirtualRegisterTracker>(function.name);
  builder->setBBInsertionPoint(function);
  BasicBlock &start =
      builder->addBasicBlock(BasicBlock(builder->cur_function->name));
  function.entry_block = &start;
  builder->setInsertionPoint(start);
  end_bb = &builder->addBasicBlock(BasicBlock{"end"}, &start);

  if (f_node.proto->args.size() > 6) {
    std::cerr << "Arguments over stack are not supported (yet!)" << '\n';
    abort();
  }
  for (usize i = 0; i < f_node.proto->args.size(); ++i) {
    const auto &arg = f_node.proto->args[i];
    Register arg_vreg = tracker->getVirtualRegister(64);
    tracker->addVar(arg_vreg,
                    arg.name ? *arg.name : std::format("unnamed_arg_{}", i));
    builder->addInstruction(Mov{arg_vreg, argumentRegister(i)});
  }

  for (const auto &s : f_node.body) {
    select(s);
  }

  builder->setInsertionPoint(*end_bb);
  builder->addInstruction(Ret{}, end_bb);
}

void ISel::select() {
  mod = std::make_unique<Module>();
  builder = std::make_unique<Builder>(mod.get());

  for (const auto &tli : module_tree->top_level_items) {
    std::visit(overloaded{[this](const auto &arg) { select(*arg); }}, tli);
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