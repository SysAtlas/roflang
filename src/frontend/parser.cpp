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
  SourceLocation loc = cur_tok->source_location;
  auto full_message =
      std::format("Parser error at {}:{}: {}\nAt {}:{}\n{}\n{}",
                  parser_loc.file_name(), parser_loc.line(), str, module_path,
                  loc.line, lexer.getProgramLines()[loc.line - 1],
                  std::string(loc.col - 1, ' ') + "^");
  std::cerr << full_message << '\n';
  exit(1);
}

/// numberexpr ::= number
AST::Expr Parser::parseNumberExpr() {
  return std::make_unique<AST::NumberExpr>(
      consumeToken<NumberToken>()->num_val);
}

/// parenexpr ::= '(' expression ')'
AST::Expr Parser::parseParenExpr() {
  consumeToken<LParToken>();
  auto v = parseExpression();
  consumeToken<RParToken>();
  return v;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
AST::Expr Parser::parseIdExpr() {
  std::string id_name{consumeToken<IdentifierToken>()->name};

  // Simple variable ref.
  if (!cur_tok->is<LParToken>()) {
    return std::make_unique<AST::VariableExpr>(id_name);
  }

  consumeToken<LParToken>();
  std::vector<AST::Expr> args;
  if (!cur_tok->is<RParToken>()) {
    while (true) {
      auto arg = parseExpression();
      args.push_back(std::move(arg));

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

  return std::make_unique<AST::CallExpr>(id_name, std::move(args));
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
AST::Expr Parser::parseBinOpRHS(u32 prev_precedence, AST::Expr &lhs) {
  while (true) {
    if (!cur_tok->is<BinOpToken>()) {
      return std::move(lhs);
    }

    const BinOpInfo *info = cur_tok->get<BinOpToken>()->info;
    u32 precedence = info->precedence;

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (precedence < prev_precedence) {
      return std::move(lhs);
    }

    // We can now consume
    consumeToken<BinOpToken>();

    // Parse the primary expression after the binary operator.
    AST::Expr rhs = parsePrimary();

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    if (const BinOpToken *next_bin_op = cur_tok->getIf<BinOpToken>()) {
      u32 next_prec = next_bin_op->info->precedence;
      if (precedence < next_prec) {
        rhs = parseBinOpRHS(precedence + 1, rhs);
      }
    }

    // Merge LHS/RHS.
    lhs = std::make_unique<AST::BinaryExpr>(info->op, std::move(lhs),
                                            std::move(rhs));
  }
}

/// expression
///   ::= primary binoprhs
AST::Expr Parser::parseExpression() {
  AST::Expr lhs = parsePrimary();
  if (cur_tok->is<BinOpToken>()) {
    return parseBinOpRHS(0, lhs);
  }
  return lhs;
}

AST::Function::Argument Parser::parseFunctionArgument() {
  std::optional<std::string> name;
  if (cur_tok->is<IdentifierToken>()) {
    name = consumeToken<IdentifierToken>()->name;
    consumeToken<ColonToken>();
  }
  const RLTypeInfo *type_info = consumeToken<RLTypeToken>()->info;
  return AST::Function::Argument{name, type_info};
}
/// signature
///   ::= id '(' id* ') -> returntype'
AST::Function::Signature Parser::parseSignature() {
  const IdentifierToken *name_token = consumeToken<IdentifierToken>();
  std::string fn_name{name_token->name};

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

  const RLTypeInfo *return_type_info = consumeToken<RLTypeToken>()->info;

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
  std::string_view name = consumeToken<IdentifierToken>()->name;
  consumeToken<ColonToken>();
  const RLTypeInfo *type = consumeToken<RLTypeToken>()->info;
  consumeToken<EqualsToken>();
  AST::Expr value = parseExpression();
  return std::make_unique<AST::LocalVarDeclStmt>(name, type, std::move(value));
}

/// assignment
/// id = expr;
[[nodiscard]] std::unique_ptr<AST::AssignmentStatement>
Parser::parseAssignmentStmt() {
  std::string_view name = consumeToken<IdentifierToken>()->name;
  consumeToken<EqualsToken>();
  AST::Expr value = parseExpression();
  return std::make_unique<AST::AssignmentStatement>(name, std::move(value));
}

[[nodiscard]] std::unique_ptr<AST::WhileStatement> Parser::parseWhileStmt() {
  consumeToken<KeywordWhileToken>();
  consumeToken<LParToken>();
  AST::Expr cond = parseExpression();
  consumeToken<RParToken>();
  consumeToken<LCurlyBraceToken>();
  std::vector<AST::Statement> body = parseStatementSequence();
  consumeToken<RCurlyBraceToken>();
  return std::make_unique<AST::WhileStatement>(std::move(cond),
                                               std::move(body));
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
                   AST::Statement res = parseReturnStmt();
                   consumeToken<SemicolonToken>();
                   return res;
                 },
                 [this](const auto &) -> AST::Statement {
                   if (peek()->is<ColonToken>()) {
                     auto res = parseLocalDefStatement();
                     consumeToken<SemicolonToken>();
                     return res;
                   } else if (peek()->is<EqualsToken>()) {
                     auto res = parseAssignmentStmt();
                     consumeToken<SemicolonToken>();
                     return res;
                   } else {
                     AST::Statement res =
                         std::make_unique<AST::Expr>(parseExpression());
                     consumeToken<SemicolonToken>();

                     return res;
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
  Parser parser{module_path};
  std::unique_ptr<AST::Module> ast = parser.parseModule();
  return ast;
}