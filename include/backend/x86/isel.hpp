#pragma once

#include <frontend/ast.hpp>
#include <memory>
#include <unordered_map>

namespace X86 {

class VirtualRegisterTracker;
class BasicBlock;
class Register;
class Function;
class Module;
class Builder;
struct ISelInfo;

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
  void select(const AST::LocalVarDeclStmt &stmt);
  void select(const AST::AssignmentStatement &assignment_stmt);
  void select(const AST::WhileStatement &while_stmt);
  Function& select(const AST::Function::Signature& signature, bool is_decl);
  void select(const AST::Function &f_node);
  void select();

  [[noreturn]] void logError(std::string_view err_msg);
  UP<VirtualRegisterTracker> tracker;
  UP<Module> mod;
  UP<AST::Module> module_tree;
  UP<Builder> builder;

  std::unordered_map<std::string, const AST::Function*> function_map;
  ISel(UP<AST::Module> module_tree);
  BasicBlock *end_bb = nullptr;
public:
  static UP<Module> runPass(UP<AST::Module> ast);
};

} // namespace X86