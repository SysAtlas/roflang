#include <fstream>
#include <helper.hpp>
#include <iostream>
#include <lexer.hpp>

static constexpr std::array BinOpInfoTable = {
  BinOpInfo{BinOpType::Add, 20, "+"},
  BinOpInfo{BinOpType::Sub, 20, "-"},
  BinOpInfo{BinOpType::Mul, 40, "*"},
  BinOpInfo{BinOpType::Div, 40, "/"},
  BinOpInfo{BinOpType::Lt, 10, "<"},
  BinOpInfo{BinOpType::Leq, 10, "<="},
  BinOpInfo{BinOpType::Gt, 10, ">"},
  BinOpInfo{BinOpType::Geq, 10, ">="},
  BinOpInfo{BinOpType::Eq, 10, "=="},
  BinOpInfo{BinOpType::Neq, 10, "!="},
};

static constexpr std::array RLTypeInfoTable = {
  RLTypeInfo{RLType::Void, "void"},
  RLTypeInfo{RLType::I64, "i64"},
};

const BinOpInfo *searchBinOpInfoTable(BinOpType BinOp) {
  if (auto Element = std::ranges::find_if(
        BinOpInfoTable,
        [BinOp](const BinOpInfo &Entry) { return Entry.Op == BinOp; });
      Element != BinOpInfoTable.end()) {
    return Element;
  }
  return nullptr;
}

const BinOpInfo *searchBinOpInfoTable(const std::string &SV) {
  if (auto Element = std::ranges::find_if(
        BinOpInfoTable,
        [SV](const auto &Entry) { return Entry.PrettyRepr_ == SV; });
      Element != BinOpInfoTable.end()) {
    return Element;
  }
  return nullptr;
}

const RLTypeInfo *searchRLTypeInfoTable(RLType Type) {
  if (auto Element = std::ranges::find_if(
        RLTypeInfoTable,
        [Type](const RLTypeInfo &Entry) { return Entry.Type == Type; });
      Element != RLTypeInfoTable.end()) {
    return Element;
  }
  return nullptr;
}

const RLTypeInfo *searchRLTypeInfoTable(const std::string &SV) {
  if (auto Element = std::ranges::find_if(
        RLTypeInfoTable,
        [SV](const RLTypeInfo &Entry) { return Entry.PrettyRepr_ == SV; });
      Element != RLTypeInfoTable.end()) {
    return Element;
  }
  return nullptr;
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

    // TODO: Make centralized tables.. this is too much effort
    if (IdentifierStr == "def") {
      return KeywordDefToken{};
    }
    if (IdentifierStr == "extern") {
      return KeywordExternToken{};
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
    if (const RLTypeInfo *Type = searchRLTypeInfoTable(IdentifierStr)) {
      return RLTypeToken{Type->Type};
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
    return BinOpToken{BinOpType::Sub};
  } else if (const BinOpInfo *T =
               searchBinOpInfoTable(std::string(1, ToMatch))) {
    return BinOpToken{T->Op};
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
    overloaded{[](const BinOpToken &Arg) { return Arg.Info->PrettyRepr_; },
               [](const RLTypeToken &Arg) -> const std::string & { return Arg.Info->PrettyRepr_; },
               [](const auto &Value) -> const std::string & {
                 return Value.PrettyRepr_;
               }},
    Value);
}

void Lexer::Token::dump() const {
  std::cerr << print() << std::endl;
}
