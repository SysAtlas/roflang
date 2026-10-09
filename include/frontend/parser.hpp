#pragma once

#include <common.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <memory>
#include <source_location>

class Parser {
private:
  Lexer Lexer_;
  const Token *CurTok;
  const char* ModulePath;

  [[nodiscard]] std::unique_ptr<AST::Signature> parseExtern();
  [[nodiscard]] std::unique_ptr<AST::Function> parseTopLevelExpr();
  [[nodiscard]] std::unique_ptr<AST::Function> parseFunction();
  [[nodiscard]] std::unique_ptr<AST::Signature> parseSignature();
  [[nodiscard]] AST::Expr parseExpression();
  [[nodiscard]] AST::Expr parseBinOpRHS(u32 expr_prec, AST::Expr &lhs);
  [[nodiscard]] AST::Statement parseStatement();
  [[nodiscard]] std::vector<AST::Statement> parseStatementSequence();
  [[nodiscard]] std::unique_ptr<AST::ReturnStatement> parseReturnStmt();
  [[nodiscard]] std::unique_ptr<AST::IfStatement> parseIfStmt();
  [[nodiscard]] std::unique_ptr<AST::LocalVarDeclStmt>
  parseLocalDefStatement();
  [[nodiscard]] AST::Expr parsePrimary();
  [[nodiscard]] AST::Expr parseIdExpr();
  [[nodiscard]] AST::Expr parseParenExpr();
  [[nodiscard]] AST::Expr parseNumberExpr();
  [[nodiscard]] std::unique_ptr<AST::Module> parseModule();
  [[nodiscard]] AST::FunctionArgument parseFunctionArgument();
  [[nodiscard]] std::unique_ptr<AST::AssignmentStatement> parseAssignmentStmt();
  [[nodiscard]] std::unique_ptr<AST::WhileStatement> parseWhileStmt();


  // Logging
  void logError(std::string_view str, std::source_location parser_loc);

  const Token *getNextToken();
  const Token *peek();

  template <typename T> const T *consumeToken(std::source_location loc = std::source_location::current()) {
    const T *res = CurTok->getIf<T>();
    if (!res) {
      logError(std::format("Expected {}", T::TokenName), loc);
    }
    DBGPRINT(std::format("Parser: consumed token {}", CurTok->print()));
    getNextToken();
    return res;
  }

  Parser(const char* module_path);

public:
  static std::unique_ptr<AST::Module> parse(const char* module_path);
};