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
  [[nodiscard]] std::unique_ptr<Expr> parseExpression();
  [[nodiscard]] std::unique_ptr<Expr> parseBinOpRHS(i32 ExprPrec,
                                                    std::unique_ptr<Expr> LHS);
  [[nodiscard]] std::unique_ptr<Statement> parseStatement();
  [[nodiscard]] std::vector<std::unique_ptr<Statement>>
  parseStatementSequence();
  [[nodiscard]] std::unique_ptr<ReturnStatement> parseReturnStatement();
  [[nodiscard]] std::unique_ptr<IfStatement> parseIfStatement();
  [[nodiscard]] std::unique_ptr<Expr> parsePrimary();
  [[nodiscard]] std::unique_ptr<Expr> parseIdExpr();
  [[nodiscard]] std::unique_ptr<Expr> parseParenExpr();
  [[nodiscard]] std::unique_ptr<Expr> parseNumberExpr();
  [[nodiscard]] std::unique_ptr<ModuleAST> parseModule();

  // Logging
  std::unique_ptr<PrototypeAST> logErrorP(const char *Str);
  std::unique_ptr<Expr> logError(const char *Str);

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