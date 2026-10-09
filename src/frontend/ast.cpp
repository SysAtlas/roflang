#include <common.hpp>
#include <frontend/ast.hpp>

namespace AST {
BinaryExpr::BinaryExpr(BinOp op, Expr &&lhs, Expr &&rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

CallExpr::CallExpr(const std::string &callee, std::vector<Expr> &&args)
    : callee_name(callee), args(std::move(args)) {}

IfStatement::IfStatement(Expr &&condition, IfBodyType body)
    : condition{std::move(condition)}, body{std::move(body)} {}

ReturnStatement::ReturnStatement(std::optional<Expr> &&value)
    : value{std::move(value)} {}

Signature::Signature(const std::string &name,
                     std::vector<FunctionArgument> &&args,
                     const RLTypeInfo *return_type_info, bool is_static)
    : name(name), args(std::move(args)), return_type_info{return_type_info},
      is_static{is_static} {}

void getSubtree(const NumberExpr &number_expr, std::vector<ASTNodeView> &acc) {
  acc.emplace_back(&number_expr);
}
void getSubtree(const VariableExpr &var_expr, std::vector<ASTNodeView> &acc) {
  acc.emplace_back(&var_expr);
}
void getSubtree(const BinaryExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
  getSubtree(E.lhs, Acc);
  getSubtree(E.rhs, Acc);
}
void getSubtree(const CallExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
  for (const auto &ST : E.args) {
    std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }},
               ST);
  }
}

void getSubtree(const Expr &E, std::vector<ASTNodeView> &Acc) {
  std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }}, E);
}

void getSubtree(const IfStatement &S, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&S);
  getSubtree(S.condition, Acc);
  for (const auto &BodyS : S.body) {
    getSubtree(BodyS, Acc);
  }
}

void getSubtree(const LocalVarDeclStmt &S, std::vector<ASTNodeView> &Acc) {}

void getSubtree(const AssignmentStatement &S, std::vector<ASTNodeView> &Acc) {}

void getSubtree(const WhileStatement &S, std::vector<ASTNodeView> &Acc) {}

void getSubtree(const ReturnStatement &S, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&S);
  if (S.value) {
    getSubtree(*S.value, Acc);
  }
}

void getSubtree(const Statement &S, std::vector<ASTNodeView> &Acc) {
  std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }}, S);
}

void getSubtree(const Signature &P, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&P);
}

void getSubtree(const Function &F, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&F);
  getSubtree(*F.proto, Acc);
  for (const auto &S : F.body) {
    getSubtree(S, Acc);
  }
}
void getSubtree(const Module &M, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&M);
  for (const auto &TLI : M.top_level_items) {
    std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }},
               TLI);
  }
}

} // namespace AST