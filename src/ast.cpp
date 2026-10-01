#include <ast.hpp>

BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprASTWrapper> LHS,
                             std::unique_ptr<ExprASTWrapper> RHS)
    : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprASTWrapper>> Args)
    : Callee(Callee), Args(std::move(Args)) {}