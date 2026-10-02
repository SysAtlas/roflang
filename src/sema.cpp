#include <sema.hpp>
#include <ast.hpp>


bool Sema::checkReturnType(const FunctionAST &FNode) {
}

bool Sema::analyzeFunction(const FunctionAST &FNode) {
  // TODO:
  // Analyze no statements are after last return
  // Analyze no returns are inconsistent with declared return value 
}

bool Sema::analyzeModule(const ModuleAST& MNode) {
  bool Res = true;
  for (const auto& TLI : MNode.TopLevelItems) {
    Res &= std::visit<bool>(overloaded{
      [this](const FunctionAST& Arg) { return analyzeFunction(Arg);},
      [](const auto& ) { return true; }
    }, TLI);
  }
}

Sema::Sema(const ModuleAST &AST) : AST{AST} {}

bool Sema::analyze(const ModuleAST &AST) {
  Sema S{AST};
  S.analyze(AST);
}