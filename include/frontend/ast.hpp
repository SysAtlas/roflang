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
struct Function;
struct Module;

using Expr = std::variant<UP<NumberExpr>, UP<VariableExpr>, UP<BinaryExpr>,
                          UP<CallExpr>>;

using Statement = std::variant<UP<Expr>, UP<IfStatement>, UP<ReturnStatement>,
                               UP<LocalVarDeclStmt>, UP<AssignmentStatement>,
                               UP<WhileStatement>>;

// Not used in AST itself, but used by other consumers.
using ASTNodeView =
    std::variant<const NumberExpr *, const VariableExpr *, const BinaryExpr *,
                 const CallExpr *, const IfStatement *, const ReturnStatement *,
                 const LocalVarDeclStmt *, const AssignmentStatement *,
                 const WhileStatement *, const Function *, const Module *>;

struct Node {
public:
  SourceLocation source_location;

protected:
  Node(SourceLocation source_location) : source_location{source_location} {}
};

/// NumberExprAST - Expression struct for numeric literals like "1.0".
struct NumberExpr : public Node {
  i64 value;

  NumberExpr(i64 value, SourceLocation sloc) : Node(sloc), value(value) {}
};

/// VariableExprAST - Expression struct for referencing a variable, like "a".
struct VariableExpr : public Node {
  std::string name;

  VariableExpr(const std::string &name, SourceLocation sloc)
      : Node(sloc), name(name) {}
};

/// BinaryExprAST - Expression struct for a binary operator.
struct BinaryExpr : public Node {
  BinOp op;
  Expr lhs, rhs;

  BinaryExpr(BinOp op, Expr &&lhs, Expr &&rhs, SourceLocation sloc);
};

/// CallExprAST - Expression struct for function calls.
struct CallExpr : public Node {
  std::string callee_name;
  std::vector<Expr> args;

  CallExpr(const std::string &callee, std::vector<Expr> &&args,
           SourceLocation sloc);
};

SourceLocation getExprSourceLocation(const Expr &expr);

// if (expr) { statementsequence }
struct IfStatement : public Node {
  Expr condition;
  using IfBodyType = std::vector<Statement>;
  IfBodyType body;

  IfStatement(Expr &&condition, IfBodyType body, SourceLocation sloc);
};

// return expr;
struct ReturnStatement : public Node {
  std::optional<Expr> value;

  ReturnStatement(std::optional<Expr> &&value, SourceLocation sloc);
};

struct AssignmentStatement : public Node {
  std::string name;
  Expr value;

  AssignmentStatement(std::string_view name, Expr &&value, SourceLocation sloc)
      : Node(sloc), name{name}, value{std::move(value)} {}
};

struct LocalVarDeclStmt : public Node {
  std::string name;
  const RLTypeInfo *type_info;
  Expr value;

  LocalVarDeclStmt(std::string_view name, const RLTypeInfo *type_info,
                   Expr &&value, SourceLocation sloc)
      : Node(sloc), name{name}, type_info{type_info}, value{std::move(value)} {}
};

struct WhileStatement : public Node {
  Expr cond;
  std::vector<Statement> body;

  WhileStatement(Expr &&cond, std::vector<Statement> &&body,
                 SourceLocation sloc)
      : Node(sloc), cond{std::move(cond)}, body{std::move(body)} {}
};

/// Function - This struct represents a function definition itself.
struct Function : public Node {
  struct Argument : Node {
    // Function argument names are optional and must not be specified in an
    // external function.
    std::optional<std::string> name;
    const RLTypeInfo *type_info;

    Argument(std::optional<std::string> name, const RLTypeInfo *type_info,
             SourceLocation sloc)
        : Node(sloc), name{name}, type_info{type_info} {}
  };

  /// Signature - This struct represents the signature of a function,
  struct Signature {
    std::string name;
    std::vector<Argument> args;
    const RLTypeInfo *return_type_info;
    bool is_static;

    Signature(const std::string &name, std::vector<Argument> &&args,
              const RLTypeInfo *return_type_info, bool is_static);
  };

  Signature signature;
  std::vector<Statement> body;
  bool is_decl;

  Function(Signature &&signature, std::vector<Statement> &&body, bool is_decl,
           SourceLocation sloc)
      : Node(sloc), signature{std::move(signature)}, body{std::move(body)},
        is_decl{is_decl} {}
};

// Represents the whole program as an abstract syntax tree
struct Module {
  using TopLevelItem = std::variant<Function>;
  std::vector<TopLevelItem> top_level_items;
  std::string file_path;
  std::vector<std::string> program_lines;

  Module(std::vector<TopLevelItem> top_level_items, std::string_view file_name,
         std::vector<std::string> program_lines)
      : top_level_items{std::move(top_level_items)}, file_path{file_name},
        program_lines{program_lines} {}
};

} // namespace AST