#include <cassert>
#include <common.hpp>
#include <frontend/ast.hpp>
#include <frontend/lexer.hpp>
#include <frontend/parser.hpp>
#include <memory>
#include <variant>

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// Consume current token and get the next one
const Token *Parser::getNextToken() { return cur_tok = lexer.getTok(); }

const Token *Parser::peek() { return lexer.peek(); }

/// LogError* - These are little helper functions for error handling.
void Parser::logError(
    std::string_view str,
    std::source_location parser_loc = std::source_location::current()) {
  SourceLocation loc = cur_tok->Loc;
  auto full_message =
      std::format("Parser error at {}:{}: {}\nAt {}:{}\n{}\n{}",
                  parser_loc.file_name(), parser_loc.line(), str, module_path,
                  loc.line, lexer.getProgramLines()[loc.line - 1],
                  std::string(loc.col - 2, ' ') + "^");
  std::cerr << full_message << '\n';
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
  if (!cur_tok->is<LParToken>()) {
    return std::make_unique<AST::VariableExpr>(IdName);
  }

  consumeToken<LParToken>();
  std::vector<AST::Expr> Args;
  if (!cur_tok->is<RParToken>()) {
    while (true) {
      auto Arg = parseExpression();
      Args.push_back(std::move(Arg));

      if (cur_tok->is<RParToken>()) {
        break;
      }

      if (!cur_tok->is<CommaToken>()) {
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
  if (cur_tok->is<IdentifierToken>()) {
    return parseIdExpr();
  } else if (cur_tok->is<NumberToken>()) {
    return parseNumberExpr();
  } else if (cur_tok->is<LParToken>()) {
    return parseParenExpr();
  }
  logError("unknown token when expecting an expression");
  std::abort();
}

/// binoprhs
///   ::= ('+' primary)*
AST::Expr Parser::parseBinOpRHS(u32 PrevPrecedence, AST::Expr &LHS) {
  while (true) {
    if (!cur_tok->is<BinOpToken>()) {
      return std::move(LHS);
    }

    const BinOpInfo *Info = cur_tok->get<BinOpToken>()->Info;
    u32 Precedence = Info->precedence;

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
    if (auto NextBinOp = cur_tok->getIf<BinOpToken>()) {
      u32 NextPrec = NextBinOp->Info->precedence;
      if (Precedence < NextPrec) {
        RHS = parseBinOpRHS(Precedence + 1, RHS);
      }
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<AST::BinaryExpr>(Info->op, std::move(LHS),
                                            std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
AST::Expr Parser::parseExpression() {
  AST::Expr LHS = parsePrimary();
  if (cur_tok->is<BinOpToken>()) {
    return parseBinOpRHS(0, LHS);
  }
  return LHS;
}

AST::Function::Argument Parser::parseFunctionArgument() {
  std::optional<std::string> Name;
  if (cur_tok->is<IdentifierToken>()) {
    Name = consumeToken<IdentifierToken>()->Name_;
    consumeToken<ColonToken>();
  }
  const RLTypeInfo *TypeInfo = consumeToken<RLTypeToken>()->Info;
  return AST::Function::Argument{Name, TypeInfo};
}
/// signature
///   ::= id '(' id* ') -> returntype'
AST::Function::Signature Parser::parseSignature() {
  const IdentifierToken *name_token = consumeToken<IdentifierToken>();
  std::string fn_name{name_token->Name_};

  consumeToken<LParToken>();

  std::vector<AST::Function::Argument> args;
  while (cur_tok->is<IdentifierToken>() || cur_tok->is<RLTypeToken>()) {
    args.push_back(parseFunctionArgument());
    if (!cur_tok->is<RParToken>()) {
      consumeToken<CommaToken>();
    }
  }

  consumeToken<RParToken>();
  consumeToken<ArrowToken>();

  const RLTypeInfo *return_type_info = consumeToken<RLTypeToken>()->Info;

  // TODO: add static keyword
  return AST::Function::Signature(fn_name, std::move(args), return_type_info, false);
}

/// ifstatement
/// if (expr) { statementsequence }
std::unique_ptr<AST::IfStatement> Parser::parseIfStmt() {
  consumeToken<KeywordIfToken>();
  consumeToken<LParToken>();
  AST::Expr cond = parseExpression();
  consumeToken<RParToken>();
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> body = parseStatementSequence();
  auto res =
      std::make_unique<AST::IfStatement>(std::move(cond), std::move(body));
  consumeToken<RCurlyBraceToken>();
  return res;
}

/// returnstatement
/// return expr?;
std::unique_ptr<AST::ReturnStatement> Parser::parseReturnStmt() {
  consumeToken<KeywordReturnToken>();
  std::optional<AST::Expr> value{};
  if (!cur_tok->is<SemicolonToken>()) {
    // return without return value
    value = parseExpression();
  }
  return std::make_unique<AST::ReturnStatement>(std::move(value));
}

/// localdeclstmt
/// id: type = expr;
[[nodiscard]] std::unique_ptr<AST::LocalVarDeclStmt>
Parser::parseLocalDefStatement() {
  std::string_view Name = consumeToken<IdentifierToken>()->Name_;
  consumeToken<ColonToken>();
  const RLTypeInfo *Type = consumeToken<RLTypeToken>()->Info;
  consumeToken<EqualsToken>();
  AST::Expr Value = parseExpression();
  return std::make_unique<AST::LocalVarDeclStmt>(Name, Type, std::move(Value));
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

[[nodiscard]] std::unique_ptr<AST::WhileStatement> Parser::parseWhileStmt() {
  consumeToken<KeywordWhileToken>();
  consumeToken<LParToken>();
  AST::Expr Cond = parseExpression();
  consumeToken<RParToken>();
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> Body = parseStatementSequence();
  consumeToken<RCurlyBraceToken>();
  return std::make_unique<AST::WhileStatement>(std::move(Cond),
                                               std::move(Body));
}

/// statement ::= expr;
AST::Statement Parser::parseStatement() {
  return std::visit<AST::Statement>(
      overloaded{[this](const KeywordIfToken &) -> AST::Statement {
                   return parseIfStmt();
                 },
                 [this](const KeywordWhileToken &) -> AST::Statement {
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
      cur_tok->getValue());
}

std::vector<AST::Statement> Parser::parseStatementSequence() {
  std::vector<AST::Statement> statement_sequence;
  while (!cur_tok->is<RCurlyBraceToken>()) {
    statement_sequence.emplace_back(parseStatement());
  }
  return statement_sequence;
}

/// definition ::= 'fn' signature { statementsequence }
AST::Function Parser::parseFunction() {
  consumeToken<KeywordFnToken>();
  auto signature = parseSignature();
  // check if its only a declaration
  if (cur_tok->is<SemicolonToken>()) {
    return AST::Function(std::move(signature), std::vector<AST::Statement>{}, true);
  }
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> ss = parseStatementSequence();
  consumeToken<RCurlyBraceToken>();

  return AST::Function(std::move(signature), std::move(ss), false);
}

/// top ::= definition | external
std::unique_ptr<AST::Module> Parser::parseModule() {
  std::vector<AST::Module::TopLevelItem> tlis;

  while (!cur_tok->is<EOFToken>()) {
    std::optional<AST::Module::TopLevelItem> parsed_tli =
        std::visit<std::optional<AST::Module::TopLevelItem>>(
            overloaded{
                [this](const KeywordFnToken &) { return parseFunction(); },
                [this](const SemicolonToken &) {
                  consumeToken<SemicolonToken>();
                  return std::nullopt;
                },
                [this](const auto &) {
                  logError("Unexpected!");
                  return std::nullopt;
                },
            },
            cur_tok->getValue());
    if (parsed_tli) {
      tlis.push_back(std::move(*parsed_tli));
    }
  }

  return std::make_unique<AST::Module>(std::move(tlis));
}

Parser::Parser(const char *module_path)
    : lexer{module_path}, cur_tok{lexer.getTok()}, module_path{module_path} {}

std::unique_ptr<AST::Module> Parser::parse(const char *module_path) {
  Parser P{module_path};
  std::unique_ptr<AST::Module> ast = P.parseModule();
  return ast;
}