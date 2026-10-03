#pragma once

#include "helper.hpp"
#include <ast.hpp>
#include <lexer.hpp>
#include <memory>

class Parser {
private:
  using Token = Lexer::Token;

  std::unique_ptr<Lexer> Lexer_;
  Token CurTok;

  [[nodiscard]] std::unique_ptr<AST::Prototype> parseExtern();
  [[nodiscard]] std::unique_ptr<AST::Function> parseTopLevelExpr();
  [[nodiscard]] std::unique_ptr<AST::Function> parseDefinition();
  [[nodiscard]] std::unique_ptr<AST::Prototype> parsePrototype();
  [[nodiscard]] AST::Expr parseExpression();
  [[nodiscard]] AST::Expr parseBinOpRHS(u32 ExprPrec, AST::Expr& LHS);
  [[nodiscard]] AST::Statement parseStatement();
  [[nodiscard]] std::vector<AST::Statement> parseStatementSequence();
  [[nodiscard]] std::unique_ptr<AST::ReturnStatement> parseReturnStatement();
  [[nodiscard]] std::unique_ptr<AST::IfStatement> parseIfStatement();
  [[nodiscard]] AST::Expr parsePrimary();
  [[nodiscard]] AST::Expr parseIdExpr();
  [[nodiscard]] AST::Expr parseParenExpr();
  [[nodiscard]] AST::Expr parseNumberExpr();
  [[nodiscard]] std::unique_ptr<AST::Module> parseModule();

  // Logging
  void logErrorP(const char *Str);
  void logError(const char *Str);

  Token getNextToken();

  template <typename T> T consumeToken(const char *ErrorMsg = nullptr) {
    std::optional<T> Tmp = CurTok.getIf<T>();
    if (!Tmp) {
      if (ErrorMsg) {
        logError(ErrorMsg);
      }
      abort();
    }
    T Res = *Tmp;
    DBGPRINT(std::format("Parser: consumed token {}", CurTok.print()));
    getNextToken();
    return Res;
  }

  Parser(const char *Module);

public:
  static std::unique_ptr<AST::Module> parse(const char *ModulePath);
};