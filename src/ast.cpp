#include <ast.hpp>

BinaryExpr::BinaryExpr(char Op, std::unique_ptr<Expr> LHS,
                             std::unique_ptr<Expr> RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

CallExpr::CallExpr(const std::string &Callee,
                         std::vector<std::unique_ptr<Expr>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

IfStatement::IfStatement(std::unique_ptr<Expr> Condition, IfBodyType Body)
    : Condition{std::move(Condition)}, Body{std::move(Body)} {}

ReturnStatement::ReturnStatement(std::unique_ptr<Expr> Value)
    : Value{std::move(Value)} {}

PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args,
                           RLType ReturnType)
    : Name(Name), Args(std::move(Args)), ReturnType{ReturnType} {}
