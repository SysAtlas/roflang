#pragma once

#include <ast.hpp>

// Semantic analyzer
class Sema {
  private:
  const ModuleAST& AST;

  bool checkReturnType(const FunctionAST& FNode);
  bool analyzeFunction(const FunctionAST& FNode);
  bool analyzeModule(const ModuleAST& MNode);

  Sema(const ModuleAST& AST);

  public:
  static bool analyze(const ModuleAST& AST);
};