#include <x86codegen.hpp>

void X86CodeGen::codegen(const NumberExpr &NNode) {
}

void X86CodeGen::codegen(const VariableExpr &VNode) {
}

void X86CodeGen::codegen(const BinaryExpr &BNode) {
}

void X86CodeGen::codegen(const CallExpr &CNode) {
}

void X86CodeGen::codegen(const Expr &ENode) {
}

void X86CodeGen::codegen(const Statement &SNode) {
}

void X86CodeGen::codegen(const IfStatement &SNode) {
}

void X86CodeGen::codegen(const PrototypeAST &PNode) {
}

void X86CodeGen::codegen(const FunctionAST &FNode) {
}

void X86CodeGen::codegen(const ModuleAST& MNode) {
}

std::string X86CodeGen::generate(std::unique_ptr<ModuleAST> AST) {
  X86CodeGen G;
  G.codegen(*AST);
  return G.ProgramText;
}