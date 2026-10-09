#include <fstream>
#include <common.hpp>
#include <iostream>
#include <frontend/lexer.hpp>

char Lexer::nextChar() {
  LastChar = ProgramStream.get();
  if (LastChar == '\n') {
    ++Line;
    Col = 1;
  } else {
    ++Col;
  }
  return LastChar;
}

SourceLocation Lexer::getLocInfo() {
  return {Line, Col};
}

Token Lexer::consumeTok() {
  // Skip any whitespace.
  while (std::isspace(LastChar)) {
    nextChar();
  }

  // identifier: [a-zA-Z][a-zA-Z0-9]*
  if (std::isalpha(LastChar) || LastChar == '_') {
    std::string IdentifierStr;
    do {
      IdentifierStr += LastChar;
      nextChar();
    } while (std::isalnum(LastChar) || LastChar == '_');

    // TODO: Make centralized tables.. this is too much effort
    if (IdentifierStr == "fn") {
      return KeywordFnToken{};
    }
    if (IdentifierStr == "if") {
      return KeywordIfToken{};
    }
    if (IdentifierStr == "else") {
      return KeywordElseToken{};
    }
    if (IdentifierStr == "return") {
      return KeywordReturnToken{};
    }
    if (IdentifierStr == "while") {
      return KeywordWhileToken{};
    }
    if (const RLTypeInfo *TypeInfo = searchRLTypeInfoTable(IdentifierStr)) {
      return RLTypeToken{TypeInfo->type};
    }

    return IdentifierToken{IdentifierStr};
  }

  if (std::isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      nextChar();
    } while (isdigit(LastChar) || LastChar == '.');

    // TODO: ERROR HANDLING
    i64 NumVal = std::strtoll(NumStr.c_str(), nullptr, 10);

    return NumberToken{NumVal};
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do {
      nextChar();
    } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF) {
      return consumeTok();
    }
  }

  std::optional<Token> SingleCharToken;
  i32 ToMatch = LastChar;
  nextChar();
  if (ToMatch == ',') {
    return CommaToken{};
  } else if (ToMatch == '-') {
    if (LastChar == '>') {
      nextChar();
      return ArrowToken{};
    }
    return BinOpToken{BinOp::SUB};
  } else if (const BinOpInfo* T = searchBinOpInfoTable(std::string{(char) ToMatch, (char) LastChar})) {
    // Bin ops of size 2
    nextChar();
    return BinOpToken{T->op};
    // Bin ops of size 1
  } else if (const BinOpInfo *T = searchBinOpInfoTable(std::string(1, ToMatch))) {
    return BinOpToken{T->op};
  } else if (ToMatch == ';') {
    return SemicolonToken{};
  } else if (ToMatch == '(') {
    return LParToken{};
  } else if (ToMatch == ')') {
    return RParToken{};
  } else if (ToMatch == EOF) {
    return EOFToken{};
  } else if (ToMatch == '{') {
    return LCurlyBraceToken{};
  } else if (ToMatch == '}') {
    return RCurlyBraceToken{};
  } else if (ToMatch == ':') {
    return ColonToken{};
  } else if (ToMatch == '=') {
    if (LastChar == '=') {
      nextChar();
      return BinOpToken{searchBinOpInfoTable("==")->op};
    }
    return EqualsToken{};
  }

  // Should be unreachable
  logError("Unknown token type!");
  abort();
}

void Lexer::lex() {
  do {
    SourceLocation Loc = getLocInfo();
    Tokens.push_back(consumeTok());
    Tokens.back().Loc = Loc;
    DBGPRINT(Tokens.back().print());
  } while (!Tokens.back().is<EOFToken>());
  CurTok = Tokens.data();
}

const std::vector<std::string> &Lexer::getProgramLines() {
  return ProgramLines;
}
const Token *Lexer::getTok() {
  const Token* Res = CurTok;
  ++CurTok;
  return Res;
}

const Token* Lexer::peek() {
  return CurTok;
}

Lexer::Lexer(const char* ModulePath)
    : ModulePath(ModulePath), ProgramStream{std::ifstream{ModulePath}} {
  if (ProgramStream.fail()) {
    std::cerr << "Failure opening file " << ModulePath << std::endl;
    std::exit(1);
  }
  std::ifstream Tmp{ModulePath};
  std::string Line;
  while (std::getline(Tmp, Line)) { 
    ProgramLines.push_back(Line);
  }

  nextChar();
  lex();
}

std::string_view Token::print() const {
  return std::visit<std::string_view>(
    overloaded{[](const BinOpToken &Arg) -> std::string_view { return Arg.Info->repr; },
               [](const RLTypeToken &Arg) -> std::string_view { return Arg.Info->repr; },
               [](const auto &Value) -> std::string_view {
                 return Value.TokenName;
               }},
    Value);
}

void Token::dump() const {
  std::cerr << print() << std::endl;
}
