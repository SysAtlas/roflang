#include <common.hpp>
#include <frontend/ast.hpp>

namespace AST {
BinaryExpr::BinaryExpr(BinOp op, Expr &&lhs, Expr &&rhs, SourceLocation sloc)
    : Node(sloc), op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

CallExpr::CallExpr(const std::string &callee, std::vector<Expr> &&args,
                   SourceLocation sloc)
    : Node(sloc), callee_name(callee), args(std::move(args)) {}

IfStatement::IfStatement(Expr &&condition, IfBodyType body, SourceLocation sloc)
    : Node(sloc), condition{std::move(condition)}, body{std::move(body)} {}

ReturnStatement::ReturnStatement(std::optional<Expr> &&value,
                                 SourceLocation sloc)
    : Node(sloc), value{std::move(value)} {}

Function::Signature::Signature(const std::string &name,
                               std::vector<Argument> &&args,
                               const RLTypeInfo *return_type_info,
                               bool is_static)
    : name(name), args(std::move(args)), return_type_info{return_type_info},
      is_static{is_static} {}

SourceLocation getExprSourceLocation(const Expr &expr) {
  return std::visit(
      overloaded{[](const auto &arg) { return arg->source_location; }}, expr);
}
} // namespace AST