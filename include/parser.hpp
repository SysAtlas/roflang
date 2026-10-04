#pragma once

#include "helper.hpp"
#include <ast.hpp>
#include <lexer.hpp>
#include <memory>

class Parser {
private:
  Lexer Lexer_;
  const Token *CurTok;
  const char* ModulePath;

  [[nodiscard]] std::unique_ptr<AST::Prototype> parseExtern();
  [[nodiscard]] std::unique_ptr<AST::Function> parseTopLevelExpr();
  [[nodiscard]] std::unique_ptr<AST::Function> parseDefinition();
  [[nodiscard]] std::unique_ptr<AST::Prototype> parsePrototype();
  [[nodiscard]] AST::Expr parseExpression();
  [[nodiscard]] AST::Expr parseBinOpRHS(u32 ExprPrec, AST::Expr &LHS);
  [[nodiscard]] AST::Statement parseStatement();
  [[nodiscard]] std::vector<AST::Statement> parseStatementSequence();
  [[nodiscard]] std::unique_ptr<AST::ReturnStatement> parseReturnStmt();
  [[nodiscard]] std::unique_ptr<AST::IfStatement> parseIfStmt();
  [[nodiscard]] std::unique_ptr<AST::LocalDefStatement>
  parseLocalDefStatement();
  [[nodiscard]] AST::Expr parsePrimary();
  [[nodiscard]] AST::Expr parseIdExpr();
  [[nodiscard]] AST::Expr parseParenExpr();
  [[nodiscard]] AST::Expr parseNumberExpr();
  [[nodiscard]] std::unique_ptr<AST::Module> parseModule();

  // Logging
  void logError(std::string_view Str);

  const Token *getNextToken();
  const Token *peek();

  template <typename T> const T *consumeToken() {
    const T *Res = CurTok->getIf<T>();
    if (!Res) {
      logError(std::format("Expected {}", T::TokenName));
    }
    DBGPRINT(std::format("Parser: consumed token {}", CurTok->print()));
    getNextToken();
    return Res;
  }

  Parser(const char* ModulePath);

public:
  static std::unique_ptr<AST::Module> parse(const char* ModulePath);
};