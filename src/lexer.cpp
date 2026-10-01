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

    if (IdentifierStr == "def") {
      return DefToken{};
    }
    if (IdentifierStr == "extern") {
      return ExternToken{};
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
    f64 NumVal = strtod(NumStr.c_str(), nullptr);

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
  }

  // Should be unreachable
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
