#include <ast.hpp>

namespace AST {
BinaryExpr::BinaryExpr(BinOpType Op, Expr &&LHS, Expr &&RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

CallExpr::CallExpr(const std::string &Callee, std::vector<Expr> &&Args)
    : Callee(Callee), Args(std::move(Args)) {}

IfStatement::IfStatement(Expr &&Condition, IfBodyType Body)
    : Condition{std::move(Condition)}, Body{std::move(Body)} {}

ReturnStatement::ReturnStatement(std::optional<Expr> &&Value)
    : Value{std::move(Value)} {}

Prototype::Prototype(const std::string &Name,
                     std::vector<std::string> Args,
                     RLType ReturnType)
    : Name(Name), Args(std::move(Args)), ReturnType{ReturnType} {}

void getSubtree(const NumberExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
}
void getSubtree(const VariableExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
}
void getSubtree(const BinaryExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
  getSubtree(E.LHS, Acc);
  getSubtree(E.RHS, Acc);
}
void getSubtree(const CallExpr &E, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&E);
  for (const auto &ST : E.Args) {
    std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }},
               ST);
  }
}

void getSubtree(const Expr &E, std::vector<ASTNodeView> &Acc) {
  std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }}, E);
}

void getSubtree(const IfStatement &S, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&S);
  getSubtree(S.Condition, Acc);
  for (const auto& BodyS : S.Body) {
    getSubtree(BodyS, Acc);
  }
}

void getSubtree(const LocalDefStatement& S, std::vector<ASTNodeView> &Acc) {

}

void getSubtree(const ReturnStatement &S, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&S);
  if (S.Value) {
    getSubtree(*S.Value, Acc);
  }
}

void getSubtree(const Statement &S, std::vector<ASTNodeView> &Acc) {
  std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }}, S);
}

void getSubtree(const Prototype &P, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&P);
}

void getSubtree(const Function &F, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&F);
  getSubtree(*F.Proto, Acc);
  for (const auto& S : F.Body) {
    getSubtree(S, Acc);
  }
}
void getSubtree(const Module &M, std::vector<ASTNodeView> &Acc) {
  Acc.emplace_back(&M);
  for (const auto& TLI : M.TopLevelItems) {
    std::visit(overloaded{[&Acc](const auto &Arg) { getSubtree(*Arg, Acc); }}, TLI);
  }
}

} // namespace AST