#pragma once

#include <common.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace AST {

struct NumberExpr;
struct VariableExpr;
struct BinaryExpr;
struct CallExpr;
struct IfStatement;
struct ReturnStatement;
struct WhileStatement;
struct LocalVarDeclStmt;
struct AssignmentStatement;
struct FunctionArgument;
struct Signature;
struct Function;
struct Module;

using Expr = std::variant<std::unique_ptr<NumberExpr>,
                          std::unique_ptr<VariableExpr>,
                          std::unique_ptr<BinaryExpr>,
                          std::unique_ptr<CallExpr>>;

using Statement = std::variant<std::unique_ptr<Expr>,
                               std::unique_ptr<IfStatement>,
                               std::unique_ptr<ReturnStatement>,
                               std::unique_ptr<LocalVarDeclStmt>,
                               std::unique_ptr<AssignmentStatement>,
                               std::unique_ptr<WhileStatement>>;

// Not used in AST itself, but used by other consumers.
using ASTNodeView = std::variant<const NumberExpr *,
                                 const VariableExpr *,
                                 const BinaryExpr *,
                                 const CallExpr *,
                                 const IfStatement *,
                                 const ReturnStatement *,
                                 const LocalVarDeclStmt *,
                                 const AssignmentStatement *,
                                 const WhileStatement *,
                                 const Signature *,
                                 const FunctionArgument *,
                                 const Function *,
                                 const Module *>;

/// NumberExprAST - Expression struct for numeric literals like "1.0".
struct NumberExpr {
  i64 value;

  explicit NumberExpr(i64 value) : value(value) {}
};

/// VariableExprAST - Expression struct for referencing a variable, like "a".
struct VariableExpr {
  std::string name;

  explicit VariableExpr(const std::string &name) : name(name) {}
};

/// BinaryExprAST - Expression struct for a binary operator.
struct BinaryExpr {
  BinOp op;
  Expr lhs, rhs;

  BinaryExpr(BinOp op, Expr &&lhs, Expr &&rhs);
};

/// CallExprAST - Expression struct for function calls.
struct CallExpr {
  std::string callee_name;
  std::vector<Expr> args;

  CallExpr(const std::string &callee, std::vector<Expr> &&args);
};

// if (expr) { statementsequence }
struct IfStatement {
  Expr condition;
  using IfBodyType = std::vector<Statement>;
  IfBodyType body;

  IfStatement(Expr &&condition, IfBodyType body);
};

// return expr;
struct ReturnStatement {
  std::optional<Expr> value;

  explicit ReturnStatement(std::optional<Expr> &&value);
};

struct AssignmentStatement {
  std::string name;
  Expr value;

  AssignmentStatement(std::string_view name, Expr &&value)
      : name{name}, value{std::move(value)} {}
};

struct LocalVarDeclStmt {
  std::string name;
  const RLTypeInfo *type_info;
  Expr value;

  LocalVarDeclStmt(std::string_view name, const RLTypeInfo *type_info, Expr &&value)
      : name{name}, type_info{type_info}, value{std::move(value)} {}
};

struct WhileStatement {
  Expr cond;
  std::vector<Statement> body;

  WhileStatement(Expr &&cond, std::vector<Statement> &&body)
      : cond{std::move(cond)}, body{std::move(body)} {}
};

struct FunctionArgument {
  // Function argument names are optional and must not be specified in an external function.
  std::optional<std::string> name;
  const RLTypeInfo *type_info;

  FunctionArgument(std::optional<std::string> name, const RLTypeInfo *type_info)
      : name{name}, type_info{type_info} {}
};

/// Signature - This struct represents the signature of a function,
struct Signature {
  std::string name;
  std::vector<FunctionArgument> args;
  const RLTypeInfo *return_type_info;
  bool is_static;

  Signature(const std::string &name,
            std::vector<FunctionArgument> &&args,
            const RLTypeInfo *return_type_info, bool is_static);
};

/// Function - This struct represents a function definition itself.
struct Function {
  std::unique_ptr<Signature> proto;
  std::vector<Statement> body;
  bool is_decl;

  Function(std::unique_ptr<Signature> proto, std::vector<Statement> &&body, bool is_decl)
      : proto{std::move(proto)}, body{std::move(body)}, is_decl{is_decl} {}
};

// Represents the whole program as an abstract syntax tree
struct Module {
  using TopLevelItem = std::variant<std::unique_ptr<Function>>;
  std::vector<TopLevelItem> top_level_items;

  Module(std::vector<TopLevelItem> top_level_items)
      : top_level_items{std::move(top_level_items)} {}
};

void getSubtree(const NumberExpr &number_expr, std::vector<ASTNodeView> &acc);
void getSubtree(const VariableExpr &var_expr, std::vector<ASTNodeView> &acc);
void getSubtree(const BinaryExpr &bin_expr, std::vector<ASTNodeView> &acc);

void getSubtree(const CallExpr &E, std::vector<ASTNodeView> &acc);

void getSubtree(const Expr &E, std::vector<ASTNodeView> &acc);

void getSubtree(const IfStatement &S, std::vector<ASTNodeView> &acc);
void getSubtree(const ReturnStatement &S, std::vector<ASTNodeView> &acc);
void getSubtree(const LocalVarDeclStmt &S, std::vector<ASTNodeView> &acc);
void getSubtree(const AssignmentStatement &S, std::vector<ASTNodeView> &acc);
void getSubtree(const WhileStatement &S, std::vector<ASTNodeView> &acc);
void getSubtree(const Statement &S, std::vector<ASTNodeView> &acc);

void getSubtree(const Signature &P, std::vector<ASTNodeView> &acc);
void getSubtree(const Function &F, std::vector<ASTNodeView> &acc);
void getSubtree(const Module &M, std::vector<ASTNodeView> &acc);

} // namespace AST