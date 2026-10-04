#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <variant>

class Sema {
private:
  const AST::Module &M;

  Sema(const AST::Module &M) : M{M} {}

  template <typename T, typename S>
  std::vector<const T *> getAll(const S &ASTElement) {
    std::vector<AST::ASTNodeView> CollectAll{};
    AST::getSubtree(ASTElement, CollectAll);

    std::vector<const T *> Result;
    for (const auto& C : CollectAll) {
      if (std::holds_alternative<const T*>(C)) {
        Result.push_back(std::get<const T*>(C));
      }
    }
    return Result;
  }

  void reportSemanticAnalysisError(std::string_view Msg) {
    std::cerr << Msg << '\n';
    abort();
  }
  
  bool returnAnalysis(const AST::Function &F) {
    bool Valid = true;
    for (const AST::ReturnStatement *RS : getAll<AST::ReturnStatement>(F)) {
      bool Current = RS->Value.has_value() != (F.Proto->ReturnTypeInfo->Type == RLType::Void);
      if (!Current) {
        reportSemanticAnalysisError(std::format("Invalid return type in function {}", F.Proto->Name));
      }
      Valid &= Current;
    }
    return Valid;
  }
  // --------

public:
  static bool analyze(const AST::Module &M) {
    Sema S{M};
    for (const auto& TLI : M.TopLevelItems) {
      if (const auto* F = std::get_if<std::unique_ptr<AST::Function>>(&TLI)) {
        S.returnAnalysis(**F);
      }
    }
    return false;
  }
};