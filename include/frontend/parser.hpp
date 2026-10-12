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
  AST::Module* mod = nullptr;

  // Stmt
  [[nodiscard]] AST::Statement parseStatement();
  [[nodiscard]] std::vector<AST::Statement> parseStatementSequence();
  [[nodiscard]] UP<AST::ReturnStatement> parseReturnStmt();
  [[nodiscard]] UP<AST::IfStatement> parseIfStmt();
  [[nodiscard]] UP<AST::AssignmentStatement> parseAssignmentStmt();
  [[nodiscard]] UP<AST::WhileStatement> parseWhileStmt();
  [[nodiscard]] UP<AST::LocalVarDeclStmt> parseLocalDefStatement();

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
  [[nodiscard]] UP<AST::Module> parseModule();

  // Logging
  void logError(std::string_view str);
  
  // For storing source locations. Semantic analysis will refer to them
  // when performing checks
  std::unordered_map<AST::ASTNodeView, SourceLocation> locations;
  SourceLocation consumed_token_loc{};

  const Token *getNextToken();
  const Token *peek();

  template <typename T>
  const T *
  consumeToken() {
    const T *res = cur_tok->getIf<T>();
    if (!res) {
      logError(std::format("Expected {}", T::TOKEN_NAME));
    }
    DBGPRINT(std::format("Parser: consumed token {}", cur_tok->print()));
    consumed_token_loc = cur_tok->source_location;
    getNextToken();
    return res;
  }

  Parser(const char *module_path);

public:
  static UP<AST::Module> parse(const char *module_path);
};