#include "ast.hpp"
#include "lexer.hpp"
#include <memory>
#include <my_types.hpp>
#include <parser.hpp>
#include <variant>

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// Consume current token and get the next one
Lexer::Token Parser::getNextToken() {
  return CurTok = Lexer_->getTok();
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
i32 Parser::getTokPrecedence() {
  assert(CurTok.is<Lexer::BinOpToken>() &&
         "BinOp predecende queried in wrong place!");
  char Op = CurTok.get<Lexer::BinOpToken>().Op_;

  std::optional<u32> Precedence = getBinOpPrededenece(Op);
  assert(Precedence && "Unknown binary operator!");
  return *Precedence;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<Expr> Parser::logError(const char *Str) {
  Lexer::LocInfo Info = Lexer_->getLocInfo();
  fprintf(stderr, "Error on line %d, col %d: %s\n", Info.Line, Info.Col, Str);
  abort();
  return nullptr;
}

std::unique_ptr<PrototypeAST> Parser::logErrorP(const char *Str) {
  logError(Str);
  return nullptr;
}

/// numberexpr ::= number
std::unique_ptr<Expr> Parser::parseNumberExpr() {
  auto NumberToken = consumeToken<Lexer::NumberToken>();
  auto Result = std::make_unique<NumberExpr>(NumberToken.NumVal_);
  return Expr::from(std::move(Result));
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<Expr> Parser::parseParenExpr() {
  consumeToken<Lexer::LParToken>();
  auto V = parseExpression();
  if (!V) {
    return nullptr;
  }

  consumeToken<Lexer::RParToken>();
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
std::unique_ptr<Expr> Parser::parseIdExpr() {
  auto IdToken = consumeToken<Lexer::IdentifierToken>();
  std::string IdName{IdToken.Name_};

  // Simple variable ref.
  if (!CurTok.is<Lexer::LParToken>()) {
    return Expr::from(std::make_unique<VariableExpr>(IdName));
  }

  consumeToken<Lexer::LParToken>();
  std::vector<std::unique_ptr<Expr>> Args;
  if (!CurTok.is<Lexer::RParToken>()) {
    while (true) {
      if (auto Arg = parseExpression()) {
        Args.push_back(std::move(Arg));
      }

      if (CurTok.is<Lexer::RParToken>()) {
        break;
      }

      if (!CurTok.is<Lexer::CommaToken>()) {
        return logError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }

  consumeToken<Lexer::RParToken>();

  return Expr::from(std::make_unique<CallExpr>(IdName, std::move(Args)));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<Expr> Parser::parsePrimary() {
  return std::visit(
    overloaded{
      [this](const Lexer::IdentifierToken &Arg) { return parseIdExpr(); },
      [this](const Lexer::NumberToken &Arg) { return parseNumberExpr(); },
      [this](const Lexer::LParToken &Arg) { return parseParenExpr(); },
      [this](const auto &Arg) {
        return logError("unknown token when expecting an expression");
      }},
    CurTok.getValue());
}

/// binoprhs
///   ::= ('+' primary)*
std::unique_ptr<Expr> Parser::parseBinOpRHS(int ExprPrec,
                                            std::unique_ptr<Expr> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    if (!CurTok.is<Lexer::BinOpToken>()) {
      return LHS;
    }

    i32 TokPrec = getTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec) {
      return LHS;
    }

    // Okay, we know this is a binop.
    char BinOp = consumeToken<Lexer::BinOpToken>().Op_;

    // Parse the primary expression after the binary operator.
    std::unique_ptr<Expr> RHS = parsePrimary();

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    if (CurTok.is<Lexer::BinOpToken>()) {
      i32 NextPrec = getTokPrecedence();
      if (TokPrec < NextPrec) {
        RHS = parseBinOpRHS(TokPrec + 1, std::move(RHS));
        if (!RHS) {
          return nullptr;
        }
      }
    }

    // Merge LHS/RHS.
    LHS = Expr::from(
      std::make_unique<BinaryExpr>(BinOp, std::move(LHS), std::move(RHS)));
  }
}

/// expression
///   ::= primary binoprhs
std::unique_ptr<Expr> Parser::parseExpression() {
  auto LHS = parsePrimary();
  return parseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ') -> returntype'
std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
  Lexer::IdentifierToken NameToken =
    consumeToken<Lexer::IdentifierToken>("Expected function name in prototype");
  std::string FnName{NameToken.Name_};

  consumeToken<Lexer::LParToken>("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (CurTok.is<Lexer::IdentifierToken>()) {
    Lexer::IdentifierToken IdToken = consumeToken<Lexer::IdentifierToken>();
    ArgNames.emplace_back(IdToken.Name_);
  }

  consumeToken<Lexer::RParToken>("Expected ')' in prototype");
  consumeToken<Lexer::ArrowToken>();

  // TODO: Make type system less ugly
  RLType ReturnType;

  static constexpr const char *BadRet = "Expected return type for function!";

  if (CurTok.is<Lexer::KeywordVoidToken>()) {
    ReturnType = RLType::Void;
    consumeToken<Lexer::KeywordVoidToken>(BadRet);
  } else if (CurTok.is<Lexer::KeywordI64Token>()) {
    ReturnType = RLType::I64;
    consumeToken<Lexer::KeywordI64Token>(BadRet);
  } else {
    abort();
  }

  return std::make_unique<PrototypeAST>(
    FnName, std::move(ArgNames), ReturnType);
}

/// ifstatement
/// if (expr) { statementsequence }
std::unique_ptr<IfStatement> Parser::parseIfStatement() {
  consumeToken<Lexer::KeywordIfToken>();
  consumeToken<Lexer::LParToken>();
  std::unique_ptr<Expr> Cond = parseExpression();
  consumeToken<Lexer::RParToken>();
  consumeToken<Lexer::LCurlyBraceToken>();
  std::vector<std::unique_ptr<Statement>> Body = parseStatementSequence();
  auto Res = std::make_unique<IfStatement>(std::move(Cond), std::move(Body));
  consumeToken<Lexer::RCurlyBraceToken>();
  return Res;
}

/// returnstatement
/// return expr?;
std::unique_ptr<ReturnStatement> Parser::parseReturnStatement() {
  consumeToken<Lexer::KeywordReturnToken>();
  std::unique_ptr<Expr> Value = nullptr;
  if (!CurTok.is<Lexer::SemicolonToken>()) {
    // return without return value
    Value = parseExpression();
  }
  return std::make_unique<ReturnStatement>(std::move(Value));
}

/// statement ::= expr;
std::unique_ptr<Statement> Parser::parseStatement() {
  std::unique_ptr<Statement> Res = std::visit<std::unique_ptr<Statement>>(
    overloaded{[this](const Lexer::KeywordIfToken &) {
                 return std::make_unique<Statement>(parseIfStatement());
               },
               [this](const Lexer::KeywordReturnToken &) {
                 auto Res = std::make_unique<Statement>(parseReturnStatement());
                 consumeToken<Lexer::SemicolonToken>();
                 return Res;
               },
               [this](const auto &) {
                 auto Res = std::make_unique<Statement>(parseExpression());
                 consumeToken<Lexer::SemicolonToken>();
                 return Res;
               }},
    CurTok.getValue());
  return Res;
}

std::vector<std::unique_ptr<Statement>> Parser::parseStatementSequence() {
  std::vector<std::unique_ptr<Statement>> StatementSequence;
  while (true) {
    if (std::unique_ptr<Statement> S = parseStatement()) {
      StatementSequence.emplace_back(std::move(S));
    }
    if (CurTok.is<Lexer::RCurlyBraceToken>()) {
      break;
    }
  }
  return StatementSequence;
}

/// definition ::= 'def' prototype { statementsequence }
std::unique_ptr<FunctionAST> Parser::parseDefinition() {
  consumeToken<Lexer::KeywordDefToken>();
  auto Proto = parsePrototype();
  if (!Proto) {
    abort();
  }
  consumeToken<Lexer::LCurlyBraceToken>();
  std::vector<std::unique_ptr<Statement>> SS = parseStatementSequence();
  consumeToken<Lexer::RCurlyBraceToken>();

  return std::make_unique<FunctionAST>(std::move(Proto), std::move(SS));
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> Parser::parseExtern() {
  consumeToken<Lexer::KeywordExternToken>();
  std::unique_ptr<PrototypeAST> Res = parsePrototype();
  consumeToken<Lexer::SemicolonToken>();
  return Res;
}

/// top ::= definition | external
std::unique_ptr<ModuleAST> Parser::parseModule() {
  std::vector<TopLevelItem> TLIs;

  while (!CurTok.is<Lexer::EOFToken>()) {
    std::optional<TopLevelItem> ParsedTLI =
      std::visit<std::optional<TopLevelItem>>(
        overloaded{
          [this](const Lexer::KeywordDefToken &Arg) {
            return parseDefinition();
          },
          [this](const Lexer::KeywordExternToken &Arg) {
            return parseExtern();
          },
          [this](const Lexer::SemicolonToken &Arg) {
            consumeToken<Lexer::SemicolonToken>();
            return std::nullopt;
          },
          [this](const auto &Arg) {
            std::abort();
            return std::nullopt;
          },
        },
        CurTok.getValue());
    if (ParsedTLI) {
      TLIs.push_back(std::move(*ParsedTLI));
    }
  }

  return std::make_unique<ModuleAST>(std::move(TLIs));
}

Parser::Parser(const char *ModulePath)
    : Lexer_{std::make_unique<Lexer>(ModulePath)}, CurTok{Lexer_->getTok()} {}

std::unique_ptr<ModuleAST> Parser::parse(const char *ModulePath) {
  Parser P{ModulePath};
  std::unique_ptr<ModuleAST> AST = P.parseModule();
  return AST;
}