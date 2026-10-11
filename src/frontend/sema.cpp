#include "frontend/ast.hpp"
#include <frontend/sema.hpp>

using namespace AST;

u8 Sema::getExpressionSize(const Expr &expr) {
  return std::visit<u8>(
      overloaded{
          [](const std::unique_ptr<NumberExpr> &) -> u8 { return 32; },
          [this](const std::unique_ptr<VariableExpr> &a) -> u8 {
            if (function_level_vars.find(a->name) ==
                function_level_vars.end()) {
              reportSemanticAnalysisError(
                  std::format("Variable use before def for {}", a->name));
            }
            return function_level_vars.at(a->name);
          },
          [this](const std::unique_ptr<BinaryExpr> &bin_expr) -> u8 {
            u8 lhs_size = getExpressionSize(bin_expr->lhs);
            u8 rhs_size = getExpressionSize(bin_expr->rhs);
            if (lhs_size != rhs_size) {
              reportSemanticAnalysisError(
                  std::format("Mismatching argument types for expression! "
                              "Can't convert type {} to {}",
                              lhs_size, rhs_size));
            }
            return lhs_size;
          },
          [this](const std::unique_ptr<CallExpr> &call_expr) -> u8 {
            if (name_to_func.find(call_expr->callee_name) ==
                name_to_func.end()) {
              reportSemanticAnalysisError(std::format(
                  "Function use before def for {}", call_expr->callee_name));
            }
            return name_to_func.at(call_expr->callee_name)
                ->signature.return_type_info->size_in_bits;
          },
      },
      expr);
}

void Sema::assignmentAnalysis(const Function &function) {
  std::vector<const LocalVarDeclStmt *> ld_stmts =
      getAll<LocalVarDeclStmt>(function);
  for (const auto *ld_stmt : ld_stmts) {
    if (function_level_vars.find(ld_stmt->name) != function_level_vars.end()) {
      reportSemanticAnalysisError(
          std::format("Variable {} has already been defined!", ld_stmt->name));
    }
    u8 rhs_size = getExpressionSize(ld_stmt->value);
    if (ld_stmt->type_info->size_in_bits != rhs_size) {
      reportSemanticAnalysisError(std::format(
          "Trying to assign expression of size {} to variable {} of size {}",
          rhs_size, ld_stmt->name, ld_stmt->type_info->size_in_bits));
    }
    function_level_vars[ld_stmt->name] = rhs_size;
  }
  std::vector<const AssignmentStatement *> a_stmts =
      getAll<AssignmentStatement>(function);
  for (const auto *a_stmt : a_stmts) {
    if (function_level_vars.find(a_stmt->name) == function_level_vars.end()) {
      reportSemanticAnalysisError(std::format(
          "Unknown variable {} has not been defined yet!", a_stmt->name));
    }
    u8 lhs_size = function_level_vars.at(a_stmt->name);
    u8 rhs_size = getExpressionSize(a_stmt->value);
    if (lhs_size != rhs_size) {
      reportSemanticAnalysisError(std::format(
          "Trying to assign expression of size {} to variable {} of size {}",
          rhs_size, a_stmt->name, lhs_size));
    }
  }
}

void Sema::analyzeFunction(const AST::Function &function) {
  function_level_vars.clear();
  for (const Function::Argument &arg : function.signature.args) {
    if (arg.name) {
      function_level_vars.insert({*arg.name, arg.type_info->size_in_bits});
    }
  }

  for (const AST::Statement &statement : function.body) {
    if (const auto *local_var_def =
            std::get_if<std::unique_ptr<LocalVarDeclStmt>>(&statement)) {
    }
  }
}

void Sema::analyzeModule(const AST::Module &mod) {
  // In the future - global variable map
  for (auto &tli : mod.top_level_items) {
    if (const Function *function = std::get_if<AST::Function>(&tli)) {
      analyzeFunction(*function);
    }
  }
}
