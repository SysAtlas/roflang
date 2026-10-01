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
  char Op = CurTok.get<Lexer::BinOpToken>().Op;

  std::optional<u32> Precedence = getBinOpPrededenece(Op);
  assert(Precedence && "Unknown binary operator!");
  return *Precedence;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprASTWrapper> Parser::logError(const char *Str) {
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
std::unique_ptr<ExprASTWrapper> Parser::parseNumberExpr() {
  auto NumberToken = consumeToken<Lexer::NumberToken>();
  auto Result = std::make_unique<NumberExprAST>(NumberToken.NumVal);
  return ExprASTWrapper::from(std::move(Result));
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprASTWrapper> Parser::parseParenExpr() {
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
std::unique_ptr<ExprASTWrapper> Parser::parseIdExpr() {
  auto IdToken = consumeToken<Lexer::IdentifierToken>();
  std::string IdName{IdToken.Name};

  // Simple variable ref.
  if (!CurTok.is<Lexer::LParToken>()) {
    return ExprASTWrapper::from(std::make_unique<VariableExprAST>(IdName));
  }

  consumeToken<Lexer::LParToken>();
  std::vector<std::unique_ptr<ExprASTWrapper>> Args;
  if (!CurTok.is<Lexer::RParToken>()) {
    while (true) {
      if (auto Arg = parseExpression()) {
        Args.push_back(std::move(Arg));
      } else {
        return nullptr;
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

  return ExprASTWrapper::from(
    std::make_unique<CallExprAST>(IdName, std::move(Args)));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<ExprASTWrapper> Parser::parsePrimary() {
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
std::unique_ptr<ExprASTWrapper>
Parser::parseBinOpRHS(int ExprPrec, std::unique_ptr<ExprASTWrapper> LHS) {
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
    char BinOp = consumeToken<Lexer::BinOpToken>().Op;

    // Parse the primary expression after the binary operator.
    auto RHS = parsePrimary();
    if (!RHS) {
      return nullptr;
    }

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
    LHS = ExprASTWrapper::from(
      std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS)));
  }
}

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprASTWrapper> Parser::parseExpression() {
  auto LHS = parsePrimary();
  if (!LHS)
    return nullptr;

  return parseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> Parser::parsePrototype() {
  Lexer::IdentifierToken NameToken =
    consumeToken<Lexer::IdentifierToken>("Expected function name in prototype");
  std::string FnName{NameToken.Name};

  consumeToken<Lexer::LParToken>("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (CurTok.is<Lexer::IdentifierToken>()) {
    Lexer::IdentifierToken IdToken = consumeToken<Lexer::IdentifierToken>();
    ArgNames.emplace_back(IdToken.Name);
  }

  consumeToken<Lexer::RParToken>("Expected ')' in prototype");
  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> Parser::parseDefinition() {
  consumeToken<Lexer::DefToken>();
  auto Proto = parsePrototype();
  if (!Proto) {
    abort();
  }

  if (auto E = parseExpression()) {
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  abort();
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> Parser::parseExtern() {
  consumeToken<Lexer::ExternToken>();
  return parsePrototype();
}

/// top ::= definition | external
std::unique_ptr<ModuleAST> Parser::parseModule() {
  std::vector<TopLevelItem> TLIs;

  while (!CurTok.is<Lexer::EOFToken>()) {
    std::optional<TopLevelItem> ParsedTLI =
      std::visit<std::optional<TopLevelItem>>(
        overloaded{
          [this](const Lexer::DefToken &Arg) { return parseDefinition(); },
          [this](const Lexer::ExternToken &Arg) { return parseExtern(); },
          [this](const Lexer::SemicolonToken &Arg) { return std::nullopt; },
          [this](const auto &Arg) {
            std::abort();
            return std::nullopt;
          },
        },
        CurTok.getValue());
    if (ParsedTLI) {
      TLIs.push_back(std::move(*ParsedTLI));
    }
    getNextToken();
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