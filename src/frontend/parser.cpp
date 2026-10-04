#include <frontend/ast.hpp>
#include <cassert>
#include <common.hpp>
#include <frontend/lexer.hpp>
#include <memory>
#include <frontend/parser.hpp>
#include <variant>

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// Consume current token and get the next one
const Token *Parser::getNextToken() {
  return CurTok = Lexer_.getTok();
}

const Token *Parser::peek() {
  return Lexer_.peek();
}

/// LogError* - These are little helper functions for error handling.
void Parser::logError(std::string_view Str) {
  SourceLocation Loc = CurTok->Loc;
  auto FullMessage = std::format("Parser error: {}\nAt {}:{}\n{}\n{}",
                                 Str,
                                 ModulePath,
                                 Loc.Line,
                                 Lexer_.getProgramLines()[Loc.Line - 1],
                                 std::string(Loc.Col - 2, ' ') + "^");
  std::cerr << FullMessage << '\n';
  exit(1);
}

/// numberexpr ::= number
AST::Expr Parser::parseNumberExpr() {
  return std::make_unique<AST::NumberExpr>(
    consumeToken<NumberToken>()->NumVal_);
}

/// parenexpr ::= '(' expression ')'
AST::Expr Parser::parseParenExpr() {
  consumeToken<LParToken>();
  auto V = parseExpression();
  consumeToken<RParToken>();
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
AST::Expr Parser::parseIdExpr() {
  std::string IdName{consumeToken<IdentifierToken>()->Name_};

  // Simple variable ref.
  if (!CurTok->is<LParToken>()) {
    return std::make_unique<AST::VariableExpr>(IdName);
  }

  consumeToken<LParToken>();
  std::vector<AST::Expr> Args;
  if (!CurTok->is<RParToken>()) {
    while (true) {
      auto Arg = parseExpression();
      Args.push_back(std::move(Arg));

      if (CurTok->is<RParToken>()) {
        break;
      }

      if (!CurTok->is<CommaToken>()) {
        logError("Expected ')' or ',' in argument list");
      }
      getNextToken();
    }
  }

  consumeToken<RParToken>();

  return std::make_unique<AST::CallExpr>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
AST::Expr Parser::parsePrimary() {
  if (CurTok->is<IdentifierToken>()) {
    return parseIdExpr();
  } else if (CurTok->is<NumberToken>()) {
    return parseNumberExpr();
  } else if (CurTok->is<LParToken>()) {
    return parseParenExpr();
  }
  logError("unknown token when expecting an expression");
  std::abort();
}

/// binoprhs
///   ::= ('+' primary)*
AST::Expr Parser::parseBinOpRHS(u32 PrevPrecedence, AST::Expr &LHS) {
  while (true) {
    if (!CurTok->is<BinOpToken>()) {
      return std::move(LHS);
    }

    const BinOpInfo *Info = CurTok->get<BinOpToken>()->Info;
    u32 Precedence = Info->Precedence;

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (Precedence < PrevPrecedence) {
      return std::move(LHS);
    }

    // We can now consume
    consumeToken<BinOpToken>();

    // Parse the primary expression after the binary operator.
    AST::Expr RHS = parsePrimary();

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    if (auto NextBinOp = CurTok->getIf<BinOpToken>()) {
      u32 NextPrec = NextBinOp->Info->Precedence;
      if (Precedence < NextPrec) {
        RHS = parseBinOpRHS(Precedence + 1, RHS);
      }
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<AST::BinaryExpr>(
      Info->Op, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
AST::Expr Parser::parseExpression() {
  AST::Expr LHS = parsePrimary();
  if (CurTok->is<BinOpToken>()) {
    return parseBinOpRHS(0, LHS);
  }
  return LHS;
}

AST::FunctionArgument Parser::parseFunctionArgument() {
  const std::string &Name = consumeToken<IdentifierToken>()->Name_;
  consumeToken<ColonToken>();
  const RLTypeInfo *TypeInfo = consumeToken<RLTypeToken>()->Info;
  return AST::FunctionArgument{Name, TypeInfo};
}
/// prototype
///   ::= id '(' id* ') -> returntype'
std::unique_ptr<AST::Prototype> Parser::parsePrototype() {
  const IdentifierToken *NameToken = consumeToken<IdentifierToken>();
  std::string FnName{NameToken->Name_};

  consumeToken<LParToken>();

  std::vector<AST::FunctionArgument> Args;
  while (CurTok->is<IdentifierToken>()) {
    Args.push_back(parseFunctionArgument());
    if (!CurTok->is<RParToken>()) {
      consumeToken<CommaToken>();
    }
  }

  consumeToken<RParToken>();
  consumeToken<ArrowToken>();

  const RLTypeInfo *ReturnTypeInfo = consumeToken<RLTypeToken>()->Info;

  return std::make_unique<AST::Prototype>(
    FnName, std::move(Args), ReturnTypeInfo);
}

/// ifstatement
/// if (expr) { statementsequence }
std::unique_ptr<AST::IfStatement> Parser::parseIfStmt() {
  consumeToken<KeywordIfToken>();
  consumeToken<LParToken>();
  AST::Expr Cond = parseExpression();
  consumeToken<RParToken>();
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> Body = parseStatementSequence();
  auto Res =
    std::make_unique<AST::IfStatement>(std::move(Cond), std::move(Body));
  consumeToken<RCurlyBraceToken>();
  return Res;
}

/// returnstatement
/// return expr?;
std::unique_ptr<AST::ReturnStatement> Parser::parseReturnStmt() {
  consumeToken<KeywordReturnToken>();
  std::optional<AST::Expr> Value{};
  if (!CurTok->is<SemicolonToken>()) {
    // return without return value
    Value = parseExpression();
  }
  return std::make_unique<AST::ReturnStatement>(std::move(Value));
}

/// localdeclstmt
/// id: type = expr;
[[nodiscard]] std::unique_ptr<AST::LocalVarDecl>
Parser::parseLocalDefStatement() {
  std::string_view Name = consumeToken<IdentifierToken>()->Name_;
  consumeToken<ColonToken>();
  const RLTypeInfo *Type = consumeToken<RLTypeToken>()->Info;
  consumeToken<EqualsToken>();
  AST::Expr Value = parseExpression();
  return std::make_unique<AST::LocalVarDecl>(Name, Type, std::move(Value));
}

/// assignment
/// id = expr;
[[nodiscard]] std::unique_ptr<AST::AssignmentStatement>
Parser::parseAssignmentStmt() {
  std::string_view Name = consumeToken<IdentifierToken>()->Name_;
  consumeToken<EqualsToken>();
  AST::Expr Value = parseExpression();
  return std::make_unique<AST::AssignmentStatement>(Name, std::move(Value));
}

[[nodiscard]] std::unique_ptr<AST::WhileStatement>
Parser::parseWhileStmt() {
  consumeToken<KeywordWhileToken>();
  consumeToken<LParToken>();
  AST::Expr Cond = parseExpression();
  consumeToken<RParToken>();
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> Body = parseStatementSequence();
  consumeToken<RCurlyBraceToken>();
  return std::make_unique<AST::WhileStatement>(std::move(Cond), std::move(Body));
}

/// statement ::= expr;
AST::Statement Parser::parseStatement() {
  return std::visit<AST::Statement>(
    overloaded{[this](const KeywordIfToken &) -> AST::Statement {
                 return parseIfStmt();
               },
               [this](const KeywordWhileToken&) -> AST::Statement {
                return parseWhileStmt();
               },
               [this](const KeywordReturnToken &) -> AST::Statement {
                 AST::Statement Res = parseReturnStmt();
                 consumeToken<SemicolonToken>();
                 return Res;
               },
               [this](const auto &) -> AST::Statement {
                 if (peek()->is<ColonToken>()) {
                   auto Res = parseLocalDefStatement();
                   consumeToken<SemicolonToken>();
                   return Res;
                 } else if (peek()->is<EqualsToken>()) {
                   auto Res = parseAssignmentStmt();
                   consumeToken<SemicolonToken>();
                   return Res;
                 } else {
                   AST::Statement Res =
                     std::make_unique<AST::Expr>(parseExpression());
                   consumeToken<SemicolonToken>();

                   return Res;
                 }
               }},
    CurTok->getValue());
}

std::vector<AST::Statement> Parser::parseStatementSequence() {
  std::vector<AST::Statement> StatementSequence;
  while (!CurTok->is<RCurlyBraceToken>()) {
    StatementSequence.emplace_back(parseStatement());
  }
  return StatementSequence;
}

/// definition ::= 'def' prototype { statementsequence }
std::unique_ptr<AST::Function> Parser::parseDefinition() {
  consumeToken<KeywordDefToken>();
  auto Proto = parsePrototype();
  if (!Proto) {
    abort();
  }
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> SS = parseStatementSequence();
  consumeToken<RCurlyBraceToken>();

  return std::make_unique<AST::Function>(std::move(Proto), std::move(SS));
}

/// external ::= 'extern' prototype
std::unique_ptr<AST::Prototype> Parser::parseExtern() {
  consumeToken<KeywordExternToken>();
  std::unique_ptr<AST::Prototype> Res = parsePrototype();
  consumeToken<SemicolonToken>();
  return Res;
}

/// top ::= definition | external
std::unique_ptr<AST::Module> Parser::parseModule() {
  std::vector<AST::Module::TopLevelItem> TLIs;

  while (!CurTok->is<EOFToken>()) {
    std::optional<AST::Module::TopLevelItem> ParsedTLI =
      std::visit<std::optional<AST::Module::TopLevelItem>>(
        overloaded{
          [this](const KeywordDefToken &) { return parseDefinition(); },
          [this](const KeywordExternToken &) { return parseExtern(); },
          [this](const SemicolonToken &) {
            consumeToken<SemicolonToken>();
            return std::nullopt;
          },
          [](const auto &) {
            std::abort();
            return std::nullopt;
          },
        },
        CurTok->getValue());
    if (ParsedTLI) {
      TLIs.push_back(std::move(*ParsedTLI));
    }
  }

  return std::make_unique<AST::Module>(std::move(TLIs));
}

Parser::Parser(const char *ModulePath)
    : Lexer_{ModulePath}, CurTok{Lexer_.getTok()}, ModulePath{ModulePath} {}

std::unique_ptr<AST::Module> Parser::parse(const char *ModulePath) {
  Parser P{ModulePath};
  std::unique_ptr<AST::Module> AST = P.parseModule();
  return AST;
}