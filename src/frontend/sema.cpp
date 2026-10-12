#include "frontend/ast.hpp"
#include <common.hpp>
#include <frontend/sema.hpp>
#include <variant>

using namespace AST;

void Sema::reportSemanticAnalysisError(std::string_view msg,
                                       SourceLocation sloc) {
  auto full_message = std::format(
      "Semantic analysis error: {}\nAt {}:{}:{}\n{}\n{}", msg, mod->file_path, sloc.line, sloc.col,
      mod->program_lines[sloc.line - 1], std::string(sloc.col - 1, ' ') + "^");
  std::cerr << full_message << '\n';
  exit(1);
}

std::pair<const RLTypeInfo *, SourceLocation>
Sema::analyzeExpression(const AST::Expr &expr) {
  if (auto num_expr = std::get_if<UP<NumberExpr>>(&expr)) {
    return {searchRLTypeInfoTable(RLType::I32), (*num_expr)->source_location};
  }
  if (auto bin_expr = std::get_if<UP<BinaryExpr>>(&expr)) {
    const auto [lhs_info, sloc] = analyzeExpression((*bin_expr)->lhs);
    const auto [rhs_info, _] = analyzeExpression((*bin_expr)->rhs);
    if (lhs_info != rhs_info) {
      reportSemanticAnalysisError(
          std::format("Mismatching argument types for expression! "
                      "Can't convert type {} to {}",
                      lhs_info->repr, rhs_info->repr),
          (*bin_expr)->source_location);
    }
    return {lhs_info, sloc};
  }
  if (auto var_expr_ptr = std::get_if<UP<VariableExpr>>(&expr)) {
    const VariableExpr &var_expr = **var_expr_ptr;
    if (current_scope.find(var_expr.name) == current_scope.end()) {
      reportSemanticAnalysisError(
          std::format("Variable use before def for {}", var_expr.name),
          var_expr.source_location);
    }
    return {current_scope.at(var_expr.name), var_expr.source_location};
  }
  if (auto call_expr_ptr = std::get_if<UP<CallExpr>>(&expr)) {
    const CallExpr &call_expr = **call_expr_ptr;
    if (name_to_func.find(call_expr.callee_name) == name_to_func.end()) {
      reportSemanticAnalysisError(
          std::format("Function use before def for {}", call_expr.callee_name),
          call_expr.source_location);
    }
    const Function *function = name_to_func.at(call_expr.callee_name);
    usize counter = 0;
    usize expected_arg_count = function->signature.args.size();

    for (auto &passed_arg : call_expr.args) {
      const auto [INFO, _] = analyzeExpression(passed_arg);
      const auto &expected_arg = function->signature.args[counter++];
      if (expected_arg.type_info != INFO) {
        reportSemanticAnalysisError(
            std::format("Argument type mismatch for function call to {}",
                        function->signature.name),
            getExprSourceLocation(passed_arg));
      }
    }
    if (counter != expected_arg_count) {
      reportSemanticAnalysisError(
          std::format("Function {} expected {} arguments but got {}",
                      function->signature.name, expected_arg_count, counter),
          call_expr.source_location);
    }

    return {name_to_func.at(call_expr.callee_name)->signature.return_type_info,
            call_expr.source_location};
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
        std::format("Variable shadowing is not allowed", lvd_stmt.name),
        lvd_stmt.source_location);
  }
  const auto [RHS_INFO, SLOC] = analyzeExpression(lvd_stmt.value);
  if (lvd_stmt.type_info != RHS_INFO) {
    reportSemanticAnalysisError(
        std::format(
            "Trying to assign expression of type {} to variable {} of type {}",
            RHS_INFO->repr, lvd_stmt.name, lvd_stmt.type_info->repr),
        SLOC);
  }
  current_scope[lvd_stmt.name] = RHS_INFO;
  return lvd_stmt.name;
}

void Sema::analyzeAssignmentStatement(const AST::AssignmentStatement &a_stmt) {
  if (current_scope.find(a_stmt.name) == current_scope.end()) {
    reportSemanticAnalysisError(
        std::format("Unknown identifier {}", a_stmt.name),
        getExprSourceLocation(a_stmt.value));
  }
  const RLTypeInfo *lhs_info = current_scope.at(a_stmt.name);
  const auto [rhs_info, SLOC] = analyzeExpression(a_stmt.value);
  if (lhs_info != rhs_info) {
    reportSemanticAnalysisError(
        std::format(
            "Trying to assign expression of type {} to variable {} of type {}",
            rhs_info->repr, a_stmt.name, lhs_info->repr),
        SLOC);
  }
}

void Sema::analyzeReturnStatement(const ReturnStatement &return_stmt) {
  std::string_view fn_name = cur_fn->signature.name;
  const RLTypeInfo *ret_type_info = cur_fn->signature.return_type_info;

  if (return_stmt.value.has_value() && ret_type_info->type == RLType::VOID) {
    reportSemanticAnalysisError(
        std::format("No return value expected from function {}", fn_name),
        getExprSourceLocation(*return_stmt.value));
  } else if (!return_stmt.value && ret_type_info->type != RLType::VOID) {
    reportSemanticAnalysisError(
        std::format("Function {} expected a return value!", fn_name),
        return_stmt.source_location);
  } else if (!return_stmt.value && ret_type_info->type == RLType::VOID) {
    return;
  }

  const auto [rv_info, sloc] = analyzeExpression(*return_stmt.value);
  if (rv_info != ret_type_info) {
    reportSemanticAnalysisError(
        std::format(
            "Incorrect return type for function {}: expected {}, got {}",
            fn_name, ret_type_info->repr, rv_info->repr),
        sloc);
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

Sema::Sema(const Module *mod) : mod{mod} {}

void Sema::analyze(const AST::Module &mod) {
  Sema pass{&mod};
  pass.analyzeModule(mod);
}
