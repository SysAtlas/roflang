#pragma once

#include <ast.hpp>
#include <lexer.hpp>
#include <memory>

class Parser {
private:
  using Token = Lexer::Token;

  std::unique_ptr<Lexer> Lexer_;
  Token CurTok;

  [[nodiscard]] std::unique_ptr<PrototypeAST> parseExtern();
  [[nodiscard]] std::unique_ptr<FunctionAST> parseTopLevelExpr();
  [[nodiscard]] std::unique_ptr<FunctionAST> parseDefinition();
  [[nodiscard]] std::unique_ptr<PrototypeAST> parsePrototype();
  [[nodiscard]] std::unique_ptr<ExprASTWrapper> parseExpression();
  [[nodiscard]] std::unique_ptr<ExprASTWrapper>
  parseBinOpRHS(i32 ExprPrec, std::unique_ptr<ExprASTWrapper> LHS);
  [[nodiscard]] std::unique_ptr<ExprASTWrapper> parsePrimary();
  [[nodiscard]] std::unique_ptr<ExprASTWrapper> parseIdExpr();
  [[nodiscard]] std::unique_ptr<ExprASTWrapper> parseParenExpr();
  [[nodiscard]] std::unique_ptr<ExprASTWrapper> parseNumberExpr();
  [[nodiscard]] std::unique_ptr<ModuleAST> parseModule();

  // Logging
  std::unique_ptr<PrototypeAST> logErrorP(const char *Str);
  std::unique_ptr<ExprASTWrapper> logError(const char *Str);

  Token getNextToken();

  template <typename T> T consumeToken(const char *ErrorMsg) {
    std::optional<T> Tmp = CurTok.getIf<T>();
    if (!Tmp) {
      logError(ErrorMsg);
      abort();
    }
    T Res = *Tmp;
    getNextToken();
    return Res;
  }

  template <typename T> T consumeToken() {
    std::optional<T> Tmp = CurTok.getIf<T>();
    if (!Tmp) {
      abort();
    }
    T Res = *Tmp;
    getNextToken();
    return Res;
  }

  i32 getTokPrecedence();

  Parser(const char *Module);

public:
  static std::unique_ptr<ModuleAST> parse(const char *ModulePath);
};