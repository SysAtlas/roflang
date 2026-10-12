#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <memory>
#include <source_location>
#include <unordered_map>

class Parser {
private:
  Lexer lexer;
  const Token *cur_tok;
  const char *module_path;

  // Stmt
  [[nodiscard]] AST::Statement parseStatement();
  [[nodiscard]] std::vector<AST::Statement> parseStatementSequence();
  [[nodiscard]] std::unique_ptr<AST::ReturnStatement> parseReturnStmt();
  [[nodiscard]] std::unique_ptr<AST::IfStatement> parseIfStmt();
  [[nodiscard]] std::unique_ptr<AST::AssignmentStatement> parseAssignmentStmt();
  [[nodiscard]] std::unique_ptr<AST::WhileStatement> parseWhileStmt();
  [[nodiscard]] std::unique_ptr<AST::LocalVarDeclStmt> parseLocalDefStatement();

  // Expr
  [[nodiscard]] AST::Expr parseExpression();
  [[nodiscard]] AST::Expr parseBinOpRHS(u32 expr_prec, AST::Expr &lhs);
  [[nodiscard]] AST::Expr parsePrimary();
  [[nodiscard]] AST::Expr parseIdExpr();
  [[nodiscard]] AST::Expr parseParenExpr();
  [[nodiscard]] AST::Expr parseNumberExpr();

  // Function
  [[nodiscard]] AST::Function::Argument parseFunctionArgument();
  [[nodiscard]] AST::Function::Signature parseSignature();
  [[nodiscard]] AST::Function parseFunction();

  // Module
  [[nodiscard]] std::unique_ptr<AST::Module> parseModule();

  // Logging
  void logError(std::string_view str, std::source_location parser_loc);
  
  // For storing source locations. Semantic analysis will refer to them
  // when performing checks
  std::unordered_map<AST::ASTNodeView, SourceLocation> locations;

  const Token *getNextToken();
  const Token *peek();

  template <typename T>
  const T *
  consumeToken(std::source_location loc = std::source_location::current()) {
    const T *res = cur_tok->getIf<T>();
    if (!res) {
      logError(std::format("Expected {}", T::TokenName), loc);
    }
    DBGPRINT(std::format("Parser: consumed token {}", cur_tok->print()));
    getNextToken();
    return res;
  }

  Parser(const char *module_path);

public:
  static std::unique_ptr<AST::Module> parse(const char *module_path);
};