#include <ast.hpp>
#include <cassert>
#include <helper.hpp>
#include <lexer.hpp>
#include <memory>
#include <parser.hpp>
#include <variant>

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// Consume current token and get the next one
Lexer::Token Parser::getNextToken() {
  return CurTok = Lexer_->getTok();
}

/// LogError* - These are little helper functions for error handling.
void Parser::logError(const char *Str) {
  Lexer::LocInfo Info = Lexer_->getLocInfo();
  fprintf(stderr, "Error on line %d, col %d: %s\n", Info.Line, Info.Col, Str);
  abort();
}

void Parser::logErrorP(const char *Str) {
  logError(Str);
  abort();
}

/// numberexpr ::= number
AST::Expr Parser::parseNumberExpr() {
  auto NumberToken = consumeToken<Lexer::NumberToken>();
  auto Result = std::make_unique<AST::NumberExpr>(NumberToken.NumVal_);
  return Result;
}

/// parenexpr ::= '(' expression ')'
AST::Expr Parser::parseParenExpr() {
  consumeToken<Lexer::LParToken>();
  auto V = parseExpression();
  consumeToken<Lexer::RParToken>();
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
AST::Expr Parser::parseIdExpr() {
  auto IdToken = consumeToken<Lexer::IdentifierToken>();
  std::string IdName{IdToken.Name_};

  // Simple variable ref.
  if (!CurTok.is<Lexer::LParToken>()) {
    return std::make_unique<AST::VariableExpr>(IdName);
  }

  consumeToken<Lexer::LParToken>();
  std::vector<AST::Expr> Args;
  if (!CurTok.is<Lexer::RParToken>()) {
    while (true) {
      auto Arg = parseExpression();
      Args.push_back(std::move(Arg));

      if (CurTok.is<Lexer::RParToken>()) {
        break;
      }

      if (!CurTok.is<Lexer::CommaToken>()) {
        logError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }

  consumeToken<Lexer::RParToken>();

  return std::make_unique<AST::CallExpr>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
AST::Expr Parser::parsePrimary() {
  if (CurTok.is<Lexer::IdentifierToken>()) {
    return parseIdExpr();
  } else if (CurTok.is<Lexer::NumberToken>()) {
    return parseNumberExpr();
  } else if (CurTok.is<Lexer::LParToken>()) {
    return parseParenExpr();
  }
  logError("unknown token when expecting an expression");
  std::abort();
}

/// binoprhs
///   ::= ('+' primary)*
AST::Expr Parser::parseBinOpRHS(u32 PrevPrecedence, AST::Expr &LHS) {
  while (true) {
    if (!CurTok.is<Lexer::BinOpToken>()) {
      return std::move(LHS);
    }

    const BinOpInfo *Info = CurTok.get<Lexer::BinOpToken>().Info;
    u32 Precedence = Info->Precedence;

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (Precedence < PrevPrecedence) {
      return std::move(LHS);
    }

    // We can now consume
    consumeToken<Lexer::BinOpToken>();

    // Parse the primary expression after the binary operator.
    AST::Expr RHS = parsePrimary();

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    if (auto NextBinOp = CurTok.getIf<Lexer::BinOpToken>()) {
      u32 NextPrec = NextBinOp->Info->Precedence;
      if (Precedence < NextPrec) {
        RHS = parseBinOpRHS(Precedence + 1, RHS);
      }
    }

    // Merge LHS/RHS.
    LHS =
      std::make_unique<AST::BinaryExpr>(Info->Op, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
AST::Expr Parser::parseExpression() {
  AST::Expr LHS = parsePrimary();
  if (CurTok.is<Lexer::BinOpToken>()) {
    return parseBinOpRHS(0, LHS);
  }
  return LHS;
}

/// prototype
///   ::= id '(' id* ') -> returntype'
std::unique_ptr<AST::Prototype> Parser::parsePrototype() {
  Lexer::IdentifierToken NameToken =
    consumeToken<Lexer::IdentifierToken>("Expected function name in prototype");
  std::string FnName{NameToken.Name_};

  consumeToken<Lexer::LParToken>("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (CurTok.is<Lexer::IdentifierToken>()) {
    Lexer::IdentifierToken IdToken = consumeToken<Lexer::IdentifierToken>();
    ArgNames.emplace_back(IdToken.Name_);
    if (!CurTok.is<Lexer::RParToken>()) {
      consumeToken<Lexer::CommaToken>();
    }
  }

  consumeToken<Lexer::RParToken>("Expected ')' in prototype");
  consumeToken<Lexer::ArrowToken>();

  RLType ReturnType = consumeToken<Lexer::RLTypeToken>("Expected return type").Info->Type;

  return std::make_unique<AST::Prototype>(FnName, std::move(ArgNames), ReturnType);
}

/// ifstatement
/// if (expr) { statementsequence }
std::unique_ptr<AST::IfStatement> Parser::parseIfStatement() {
  consumeToken<Lexer::KeywordIfToken>();
  consumeToken<Lexer::LParToken>();
  AST::Expr Cond = parseExpression();
  consumeToken<Lexer::RParToken>();
  consumeToken<Lexer::LCurlyBraceToken>();
  std::vector<AST::Statement> Body = parseStatementSequence();
  auto Res =
    std::make_unique<AST::IfStatement>(std::move(Cond), std::move(Body));
  consumeToken<Lexer::RCurlyBraceToken>();
  return Res;
}

/// returnstatement
/// return expr?;
std::unique_ptr<AST::ReturnStatement> Parser::parseReturnStatement() {
  consumeToken<Lexer::KeywordReturnToken>();
  std::optional<AST::Expr> Value{};
  if (!CurTok.is<Lexer::SemicolonToken>()) {
    // return without return value
    Value = parseExpression();
  }
  return std::make_unique<AST::ReturnStatement>(std::move(Value));
}

/// statement ::= expr;
AST::Statement Parser::parseStatement() {
  return std::visit<AST::Statement>(
    overloaded{[this](const Lexer::KeywordIfToken &) -> AST::Statement {
                 return parseIfStatement();
               },
               [this](const Lexer::KeywordReturnToken &) -> AST::Statement {
                 AST::Statement Res = parseReturnStatement();
                 consumeToken<Lexer::SemicolonToken>();
                 return Res;
               },
               [this](const auto &) -> AST::Statement {
                 AST::Statement Res =
                   std::make_unique<AST::Expr>(parseExpression());
                 consumeToken<Lexer::SemicolonToken>();
                 return Res;
               }},
    CurTok.getValue());
}

std::vector<AST::Statement> Parser::parseStatementSequence() {
  std::vector<AST::Statement> StatementSequence;
  while (!CurTok.is<Lexer::RCurlyBraceToken>()) {
    StatementSequence.emplace_back(parseStatement());
  }
  return StatementSequence;
}

/// definition ::= 'def' prototype { statementsequence }
std::unique_ptr<AST::Function> Parser::parseDefinition() {
  consumeToken<Lexer::KeywordDefToken>();
  auto Proto = parsePrototype();
  if (!Proto) {
    abort();
  }
  consumeToken<Lexer::LCurlyBraceToken>();
  std::vector<AST::Statement> SS = parseStatementSequence();
  consumeToken<Lexer::RCurlyBraceToken>();

  return std::make_unique<AST::Function>(std::move(Proto), std::move(SS));
}

/// external ::= 'extern' prototype
std::unique_ptr<AST::Prototype> Parser::parseExtern() {
  consumeToken<Lexer::KeywordExternToken>();
  std::unique_ptr<AST::Prototype> Res = parsePrototype();
  consumeToken<Lexer::SemicolonToken>();
  return Res;
}

/// top ::= definition | external
std::unique_ptr<AST::Module> Parser::parseModule() {
  std::vector<AST::Module::TopLevelItem> TLIs;

  while (!CurTok.is<Lexer::EOFToken>()) {
    std::optional<AST::Module::TopLevelItem> ParsedTLI =
      std::visit<std::optional<AST::Module::TopLevelItem>>(
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

  return std::make_unique<AST::Module>(std::move(TLIs));
}

Parser::Parser(const char *ModulePath)
    : Lexer_{std::make_unique<Lexer>(ModulePath)}, CurTok{Lexer_->getTok()} {}

std::unique_ptr<AST::Module> Parser::parse(const char *ModulePath) {
  Parser P{ModulePath};
  std::unique_ptr<AST::Module> AST = P.parseModule();
  return AST;
}