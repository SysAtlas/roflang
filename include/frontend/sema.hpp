#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <variant>

class Sema {
private:
  const AST::Module &module;

  Sema(const AST::Module &module) : module{module} {}

  template <typename T, typename S>
  std::vector<const T *> getAll(const S &ast_element) {
    std::vector<AST::ASTNodeView> collect_all{};
    AST::getSubtree(ast_element, collect_all);

    std::vector<const T *> result;
    for (const auto& c : collect_all) {
      if (std::holds_alternative<const T*>(c)) {
        result.push_back(std::get<const T*>(c));
      }
    }
    return result;
  }

  void reportSemanticAnalysisError(std::string_view msg) {
    std::cerr << msg << '\n';
    abort();
  }
  
  bool returnAnalysis(const AST::Function &F) {
    bool valid = true;
    for (const AST::ReturnStatement *return_stmt : getAll<AST::ReturnStatement>(F)) {
      bool current = return_stmt->value.has_value() != (F.proto->return_type_info->type == RLType::VOID);
      if (!current) {
        reportSemanticAnalysisError(std::format("Invalid return type in function {}", F.proto->name));
      }
      valid &= current;
    }
    return valid;
  }
  // --------

public:
  static bool analyze(const AST::Module &M) {
    Sema sema{M};
    for (const auto& tli : M.top_level_items) {
      if (const auto* f = std::get_if<std::unique_ptr<AST::Function>>(&tli)) {
        sema.returnAnalysis(**f);
      }
    }
    return false;
  }
};