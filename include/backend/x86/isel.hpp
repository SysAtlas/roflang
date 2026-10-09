#pragma once

#include "backend/x86/registers.hpp"
#include <backend/x86/x86.hpp>
#include <frontend/ast.hpp>
#include <memory>

namespace X86 {

class VirtualRegisterTracker;
class ISel {
private:
  // Helper functions
  void compareSetHelper(BinOp bin_op, const Register &dst);
  void binOpHelper(BinOp bin_op, const Register &lhs, const Register &rhs);

  // Main functions
  Register select(const AST::NumberExpr &number_expr);
  Register select(const AST::VariableExpr &var_expr);
  Register select(const AST::BinaryExpr &bin_expr);
  Register select(const AST::CallExpr &call_expr);
  Register select(const AST::Expr &expr);
  void select(const AST::Statement &stmt);
  void select(const AST::IfStatement &if_stmt);
  void select(const AST::ReturnStatement &return_stmt);
  Register select(const AST::LocalVarDeclStmt &stmt);
  Register select(const AST::AssignmentStatement &assignment_stmt);
  void select(const AST::WhileStatement &while_stmt);
  Function& select(const AST::Signature& signature);
  void select(const AST::Function &f_node);
  void select();

  [[noreturn]] void logError(std::string_view err_msg);
  std::unique_ptr<VirtualRegisterTracker> tracker;
  std::unique_ptr<Module> mod;
  std::unique_ptr<AST::Module> module_tree;
  std::unique_ptr<Builder> builder;

  ISel(std::unique_ptr<AST::Module> module_tree);
  BasicBlock *end_bb = nullptr;

  std::vector<std::unique_ptr<ISelInfo>> isel_infos;
public:
  using Result = std::pair<std::unique_ptr<Module>, std::vector<std::unique_ptr<ISelInfo>>>;
  static Result select(std::unique_ptr<AST::Module> ast);
};

} // namespace X86