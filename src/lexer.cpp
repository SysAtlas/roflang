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
  RLTypeInfo{RLType::Void, "void", 0},
  RLTypeInfo{RLType::I64, "i64", 64},
  RLTypeInfo{RLType::I32, "i32", 32},
  RLTypeInfo{RLType::I16, "i16", 16},
  RLTypeInfo{RLType::I8, "i8", 8},
  RLTypeInfo{RLType::U64, "u64", 64},
  RLTypeInfo{RLType::U32, "u32", 32},
  RLTypeInfo{RLType::U16, "u16", 16},
  RLTypeInfo{RLType::U8, "u8", 8},
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
        [SV](const auto &Entry) { return Entry.Repr == SV; });
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
        [SV](const RLTypeInfo &Entry) { return Entry.Repr == SV; });
      Element != RLTypeInfoTable.end()) {
    return Element;
  }
  return nullptr;
}

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
    if (IdentifierStr == "while") {
      return KeywordWhileToken{};
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
    return BinOpToken{BinOpType::Sub};
  } else if (const BinOpInfo* T = searchBinOpInfoTable(std::string{(char) ToMatch, (char) LastChar})) {
    // Bin ops of size 2
    nextChar();
    return BinOpToken{T->Op};
    // Bin ops of size 1
  } else if (const BinOpInfo *T = searchBinOpInfoTable(std::string(1, ToMatch))) {
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
  } else if (ToMatch == '=') {
    if (LastChar == '=') {
      nextChar();
      return BinOpToken{searchBinOpInfoTable("==")->Op};
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
    overloaded{[](const BinOpToken &Arg) -> std::string_view { return Arg.Info->Repr; },
               [](const RLTypeToken &Arg) -> std::string_view { return Arg.Info->Repr; },
               [](const auto &Value) -> std::string_view {
                 return Value.TokenName;
               }},
    Value);
}

void Token::dump() const {
  std::cerr << print() << std::endl;
}
