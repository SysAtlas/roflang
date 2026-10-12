#include "frontend/ast.hpp"
#include <common.hpp>
#include <frontend/sema.hpp>
#include <variant>

using namespace AST;

void Sema::reportSemanticAnalysisError(std::string_view msg) {
  std::cerr << msg << '\n';
  abort();
}

const RLTypeInfo *Sema::analyzeExpression(const AST::Expr &expr) {
  if (std::holds_alternative<UP<NumberExpr>>(expr)) {
    return searchRLTypeInfoTable(RLType::I32);
  }
  if (auto bin_expr = std::get_if<UP<BinaryExpr>>(&expr)) {
    const RLTypeInfo *lhs_info = analyzeExpression((*bin_expr)->lhs);
    const RLTypeInfo *rhs_info = analyzeExpression((*bin_expr)->rhs);
    if (lhs_info != rhs_info) {
      reportSemanticAnalysisError(
          std::format("Mismatching argument types for expression! "
                      "Can't convert type {} to {}",
                      lhs_info->repr, rhs_info->repr));
    }
    return lhs_info;
  }
  if (auto var_expr_ptr = std::get_if<UP<VariableExpr>>(&expr)) {
    const VariableExpr &var_expr = **var_expr_ptr;
    if (current_scope.find(var_expr.name) == current_scope.end()) {
      reportSemanticAnalysisError(
          std::format("Variable use before def for {}", var_expr.name));
    }
    return current_scope.at(var_expr.name);
  }
  if (auto call_expr_ptr = std::get_if<UP<CallExpr>>(&expr)) {
    const CallExpr &call_expr = **call_expr_ptr;
    if (name_to_func.find(call_expr.callee_name) == name_to_func.end()) {
      reportSemanticAnalysisError(
          std::format("Function use before def for {}", call_expr.callee_name));
    }
    const Function *function = name_to_func.at(call_expr.callee_name);
    usize counter = 0;
    usize expected_arg_count = function->signature.args.size();

    for (auto &passed_arg : call_expr.args) {
      const RLTypeInfo *info = analyzeExpression(passed_arg);
      const auto &expected_arg = function->signature.args[counter++];
      if (expected_arg.type_info != info) {
        reportSemanticAnalysisError(
            std::format("Argument type mismatch for function call to {}",
                        function->signature.name));
      }
    }
    if (counter != expected_arg_count) {
      reportSemanticAnalysisError(
          std::format("Function {} expected {} arguments but got {}",
                      function->signature.name, expected_arg_count, counter));
    }

    return name_to_func.at(call_expr.callee_name)->signature.return_type_info;
  }
  abort();
}

void Sema::analyzeIfStmt(const AST::IfStatement &if_stmt) {
  analyzeExpression(if_stmt.condition);

  std::vector<std::string_view> added_to_local;
  for (const auto &stmt : if_stmt.body) {
    auto decl = analyzeStmt(stmt);
    if (decl) {
      added_to_local.push_back(*decl);
    }
  }

  // Remove everything added by if from scope
  for (const auto &decl : added_to_local) {
    current_scope.erase(decl);
  }
}

void Sema::analyzeWhileStmt(const WhileStatement &while_stmt) {
  analyzeExpression(while_stmt.cond);

  std::vector<std::string_view> added_to_local;
  for (const auto &stmt : while_stmt.body) {
    auto decl = analyzeStmt(stmt);
    if (decl) {
      added_to_local.push_back(*decl);
    }
  }

  // Remove everything added by if from scope
  for (const auto &decl : added_to_local) {
    current_scope.erase(decl);
  }
}

std::string_view
Sema::analyzeLocalVarDeclStmt(const LocalVarDeclStmt &lvd_stmt) {
  if (current_scope.find(lvd_stmt.name) != current_scope.end()) {
    reportSemanticAnalysisError(
        std::format("Variable shadowing is not allowed", lvd_stmt.name));
  }
  const RLTypeInfo *rhs_info = analyzeExpression(lvd_stmt.value);
  if (lvd_stmt.type_info != rhs_info) {
    reportSemanticAnalysisError(std::format(
        "Trying to assign expression of type {} to variable {} of type {}",
        rhs_info->repr, lvd_stmt.name, lvd_stmt.type_info->repr));
  }
  current_scope[lvd_stmt.name] = rhs_info;
  return lvd_stmt.name;
}

void Sema::analyzeAssignmentStatement(const AST::AssignmentStatement &a_stmt) {
  if (current_scope.find(a_stmt.name) == current_scope.end()) {
    reportSemanticAnalysisError(
        std::format("Unknown identifier {}", a_stmt.name));
  }
  const RLTypeInfo *lhs_info = current_scope.at(a_stmt.name);
  const RLTypeInfo *rhs_info = analyzeExpression(a_stmt.value);
  if (lhs_info != rhs_info) {
    reportSemanticAnalysisError(std::format(
        "Trying to assign expression of type {} to variable {} of type {}",
        rhs_info->repr, a_stmt.name, lhs_info->repr));
  }
}

void Sema::analyzeReturnStatement(const ReturnStatement &return_stmt) {
  std::string_view fn_name = cur_fn->signature.name;
  const RLTypeInfo *ret_type_info = cur_fn->signature.return_type_info;

  if (return_stmt.value.has_value() && ret_type_info->type == RLType::VOID) {
    reportSemanticAnalysisError(
        std::format("No return value expected from function {}", fn_name));
  } else if (!return_stmt.value && ret_type_info->type != RLType::VOID) {
    reportSemanticAnalysisError(
        std::format("Function {} expeceted a return value!", fn_name));
  } else if (!return_stmt.value && ret_type_info->type == RLType::VOID) {
    return;
  }

  const RLTypeInfo *rv_info = analyzeExpression(*return_stmt.value);
  if (rv_info != ret_type_info) {
    reportSemanticAnalysisError(std::format(
        "Incorrect return type for function {}: expected {}, got {}", fn_name,
        ret_type_info->repr, rv_info->repr));
  }
}

std::optional<std::string_view>
Sema::analyzeStmt(const AST::Statement &statement) {
  if (auto lvd_stmt = std::get_if<UP<LocalVarDeclStmt>>(&statement)) {
    return analyzeLocalVarDeclStmt(**lvd_stmt);
  }
  if (auto if_stmt = std::get_if<UP<IfStatement>>(&statement)) {
    analyzeIfStmt(**if_stmt);
  } else if (auto a_stmt = std::get_if<UP<AssignmentStatement>>(&statement)) {
    analyzeAssignmentStatement(**a_stmt);
  } else if (auto return_stmt = std::get_if<UP<ReturnStatement>>(&statement)) {
    analyzeReturnStatement(**return_stmt);
  } else if (auto expr = std::get_if<UP<Expr>>(&statement)) {
    analyzeExpression(**expr);
  } else if (auto while_stmt = std::get_if<UP<WhileStatement>>(&statement)) {
    analyzeWhileStmt(**while_stmt);
  } else {
    std::cerr << "Unrecognized statement type" << '\n';
    abort();
  }
  return std::nullopt;
}

void Sema::analyzeFunction(const AST::Function &function) {
  current_scope.clear();
  for (const Function::Argument &arg : function.signature.args) {
    if (arg.name) {
      current_scope.insert({*arg.name, arg.type_info});
    }
  }

  for (const AST::Statement &statement : function.body) {
    analyzeStmt(statement);
  }
}

void Sema::analyzeModule(const AST::Module &mod) {
  // In the future - global variable map
  for (auto &tli : mod.top_level_items) {
    if (const Function *function = std::get_if<AST::Function>(&tli)) {
      cur_fn = function;
      name_to_func[function->signature.name] = function;
      analyzeFunction(*function);
    }
  }
}

void Sema::analyze(const AST::Module &mod) {
  Sema pass{};
  pass.analyzeModule(mod);
}
