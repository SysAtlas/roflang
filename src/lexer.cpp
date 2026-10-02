#include <fstream>
#include <iostream>
#include <lexer.hpp>
#include <my_types.hpp>

std::optional<u32> getBinOpPrededenece(char BinOp) {
  switch (BinOp) {
  case '<': {
    return 10;
  }
  case '+': {
    return 20;
  }
  case '-': {
    return 20;
  }
  case '*': {
    return 40;
  }
  case '/': {
    return 40;
  }
  default: {
    break;
  }
  }
  return std::nullopt;
}

char Lexer::nextChar() {
  PrevChar = LastChar;
  LastChar = ProgramStream.get();
  ++Col;
  if (PrevChar == '\n') {
    ++Line;
    Col = 1;
  }
  return LastChar;
}

Lexer::LocInfo Lexer::getLocInfo() {
  return {Line, Col};
}

Lexer::Token Lexer::getTok() {
  // Skip any whitespace.
  while (std::isspace(LastChar)) {
    nextChar();
  }

  // identifier: [a-zA-Z][a-zA-Z0-9]*
  if (std::isalpha(LastChar)) {
    std::string IdentifierStr;
    do {
      IdentifierStr += LastChar;
      nextChar();
    } while (std::isalnum(LastChar));

    // TODO: Make a centralized tables.. this is too much effort
    if (IdentifierStr == "def") {
      return KeywordDefToken{};
    }
    if (IdentifierStr == "extern") {
      return KeywordExternToken{};
    }
    if (IdentifierStr == "if") {
      return KeywordIfToken{};
    }
    if (IdentifierStr == "i64") {
      return KeywordI64Token{};
    }
    if (IdentifierStr == "else") {
      return KeywordElseToken{};
    }
    if (IdentifierStr == "void") {
      return KeywordVoidToken{};
    }
    if (IdentifierStr == "return") {
      return KeywordReturnToken{};
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
      return getTok();
    }
  }

  std::optional<Token> SingleCharToken;
  char ToMatch = LastChar;
  nextChar();
  if (ToMatch == ',') {
    return CommaToken{};
  } else if (ToMatch == '-') {
    if (LastChar == '>') {
      nextChar();
      return ArrowToken{};
    }
    return BinOpToken{'-'};
  } else if (getBinOpPrededenece(ToMatch)) {
    return BinOpToken{ToMatch};
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
  }

  // Should be unreachable
  std::cerr << "Unknown token type!" << std::endl;
  abort();
}

Lexer::Lexer(const char *ModulePath)
    : ProgramStream{std::ifstream{ModulePath}} {
  if (ProgramStream.fail()) {
    std::cerr << "Failure opening file " << ModulePath << std::endl;
    std::exit(1);
  }
  nextChar();
}

std::string_view Lexer::Token::print() const {
  return std::visit<std::string_view>(
    overloaded{[](const auto &Value) -> const std::string& { return Value.PrettyRepr_; }}, Value);
}

void Lexer::Token::dump() const {
  std::cerr << print() << std::endl;
}
