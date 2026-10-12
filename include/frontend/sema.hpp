#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <unordered_map>

class Sema {
private:
  const AST::Module* mod;
  void reportSemanticAnalysisError(std::string_view msg, SourceLocation sloc);

  std::unordered_map<std::string_view, const RLTypeInfo *> current_scope;
  std::unordered_map<std::string_view, const AST::Function *> name_to_func;
  const AST::Function *cur_fn = nullptr;

  std::pair<const RLTypeInfo *, SourceLocation> analyzeExpression(const AST::Expr &expr);
  void analyzeReturnStatement(const AST::ReturnStatement &return_stmt);
  void analyzeAssignmentStatement(const AST::AssignmentStatement &a_stmt);
  void analyzeIfStmt(const AST::IfStatement &if_stmt);
  void analyzeWhileStmt(const AST::WhileStatement &while_stmt);
  std::string_view
  analyzeLocalVarDeclStmt(const AST::LocalVarDeclStmt &lvd_stmt);
  std::optional<std::string_view> analyzeStmt(const AST::Statement &statement);
  void analyzeFunction(const AST::Function &function);
  void analyzeModule(const AST::Module &mod);

  Sema(const AST::Module *mod);

public:
  static void analyze(const AST::Module &m);
};