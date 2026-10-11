#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <unordered_map>
#include <variant>

// Current correct checks:

// Function use before def - functions can't be used before they're defined
// Function redefinition - functions can't be redefined
// Expression argument checks - all parts of the expression should be of the same type
// Type of local var declarations matches expression size of RHS 
// No assignments to undefined variables
// Variables are never assigned rvalues of mismatching size

class Sema {
private:
  const AST::Module &module;

  Sema(const AST::Module &module) : module{module} {}

  template <typename T, typename S>
  std::vector<const T *> getAll(const S &ast_element) {
    std::vector<AST::ASTNodeView> collect_all{};
    AST::getSubtree(ast_element, collect_all);

    std::vector<const T *> result;
    for (const auto &c : collect_all) {
      if (std::holds_alternative<const T *>(c)) {
        result.push_back(std::get<const T *>(c));
      }
    }
    return result;
  }

  void reportSemanticAnalysisError(std::string_view msg) {
    std::cerr << msg << '\n';
    abort();
  }

  std::unordered_map<std::string, u8> function_level_vars;
  std::unordered_map<std::string, const AST::Function *> name_to_func;

  u8 getExpressionSize(const AST::Expr &expr);

  void assignmentAnalysis(const AST::Function &function);

  void returnAnalysis(const AST::Function &f) {
    for (const AST::ReturnStatement *return_stmt :
         getAll<AST::ReturnStatement>(f)) {
      bool current = return_stmt->value.has_value() !=
                     (f.signature.return_type_info->type == RLType::VOID);
      if (!current) {
        reportSemanticAnalysisError(std::format(
            "Invalid return type in function {}", f.signature.name));
      }
    }
  }
  // --------
  void analyzeFunction(const AST::Function& function);

  void analyzeModule(const AST::Module &mod);

public:
  static bool analyze(const AST::Module &M) {
    Sema sema{M};
    for (const auto &tli : M.top_level_items) {
      sema.function_level_vars.clear();
      if (const AST::Function *f = std::get_if<AST::Function>(&tli)) {
        if (sema.name_to_func.find(f->signature.name) !=
            sema.name_to_func.end()) {
          sema.reportSemanticAnalysisError(
              std::format("Function redefinition for {}", f->signature.name));
        }
        sema.name_to_func[f->signature.name] = f;
        sema.returnAnalysis(*f);
        sema.assignmentAnalysis(*f);
      }
    }
    return false;
  }
};