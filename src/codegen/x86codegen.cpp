#include <codegen/x86codegen.hpp>

void X86CodeGen::codegen(const AST::NumberExpr &NNode) {
}

void X86CodeGen::codegen(const AST::VariableExpr &VNode) {
}

void X86CodeGen::codegen(const AST::BinaryExpr &BNode) {
}

void X86CodeGen::codegen(const AST::CallExpr &CNode) {
}

void X86CodeGen::codegen(const AST::Expr &ENode) {
}

void X86CodeGen::codegen(const AST::Statement &SNode) {
}

void X86CodeGen::codegen(const AST::IfStatement &SNode) {
}

void X86CodeGen::codegen(const AST::Prototype &PNode) {
}

void X86CodeGen::codegen(const AST::Function &FNode) {
}

void X86CodeGen::codegen(const AST::Module& MNode) {
}

std::string X86CodeGen::generate(std::unique_ptr<AST::Module> AST) {
  X86CodeGen G;
  G.codegen(*AST);
  return G.ProgramText;
}