#pragma once

#include <common.hpp>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

struct EOFToken {
  static constexpr std::string TokenName = "EOF";
};

// Reserved keywords
struct KeywordFnToken {
  static constexpr std::string_view TokenName = "def";
};
struct KeywordIfToken {
  static constexpr std::string_view TokenName = "if";
};
struct KeywordElseToken {
  static constexpr std::string_view TokenName = "else";
};
struct KeywordReturnToken {
  static constexpr std::string_view TokenName = "return";
};
struct KeywordWhileToken {
  static constexpr std::string_view TokenName = "while";
};

// Types
struct RLTypeToken {
  const RLTypeInfo *Info;
  static constexpr std::string_view TokenName = "type";

  RLTypeToken(const RLType &Type) : Info(searchRLTypeInfoTable(Type)) {}
};

// -----------------

struct IdentifierToken {
  std::string Name_;
  static constexpr std::string_view TokenName = "identifier";

  IdentifierToken(std::string_view Name) : Name_{Name} {}
};

struct NumberToken {
  i64 NumVal_;
  static constexpr std::string_view TokenName = "number";

  NumberToken(i64 NumVal) : NumVal_{NumVal} {}
};

struct BinOpToken {
  const BinOpInfo *Info;
  static constexpr std::string_view TokenName = "binary operator";

  BinOpToken(BinOp Op) : Info{searchBinOpInfoTable(Op)} {}
};

// Punctuation
struct LParToken {
  static constexpr std::string_view TokenName = "(";
};
struct RParToken {
  static constexpr std::string_view TokenName = ")";
};
struct LCurlyBraceToken {
  static constexpr std::string_view TokenName = "{";
};
struct RCurlyBraceToken {
  static constexpr std::string_view TokenName = "}";
};
struct CommaToken {
  static constexpr std::string_view TokenName = ",";
};
struct SemicolonToken {
  static constexpr std::string_view TokenName = ";";
};
struct ArrowToken {
  static constexpr std::string_view TokenName = "->";
};
struct ColonToken {
  static constexpr std::string_view TokenName = ":";
};
struct EqualsToken {
  static constexpr std::string_view TokenName = "=";
};

// New token types should always be added to this variant.
class Token {
  using TokenType = std::variant<EOFToken,
                                 KeywordFnToken,
                                 IdentifierToken,
                                 NumberToken,
                                 BinOpToken,
                                 LParToken,
                                 RParToken,
                                 CommaToken,
                                 SemicolonToken,
                                 LCurlyBraceToken,
                                 RCurlyBraceToken,
                                 KeywordIfToken,
                                 KeywordElseToken,
                                 ArrowToken,
                                 KeywordReturnToken,
                                 ColonToken,
                                 RLTypeToken,
                                 EqualsToken,
                                 KeywordWhileToken>;

  TokenType Value;

public:
  SourceLocation Loc{};

  const TokenType &getValue() const {
    return Value;
  }

  template <typename T> bool is() const {
    return std::holds_alternative<T>(Value);
  }

  template <typename T> const T *get() const {
    return &std::get<T>(Value);
  }

  template <typename T> const T *getIf() const {
    auto *Res = get_if<T>(&Value);
    if (!Res) {
      return nullptr;
    }
    return Res;
  }

  std::string_view print() const;

  void dump() const;

  template <typename T> Token(T Value) : Value(Value) {}
};

class Lexer {
private:
  std::vector<Token> Tokens;
  const Token *CurTok = nullptr;
  const char *ModulePath;

  u32 Line = 1;
  u32 Col = 1;

  SourceLocation getLocInfo();

  i32 LastChar;

  std::ifstream ProgramStream;
  std::vector<std::string> ProgramLines;

  void updateLineCounter();

  // Set LastChar to next char of ProgramStream and return LastChar
  char nextChar();

  Token consumeTok();

  void lex();

  void logError(std::string_view Str) {
    auto FullMessage = std::format("Lexer error: {}\nAt {}:{}\n{}\n{}",
                                   Str,
                                   ModulePath,
                                   Line,
                                   ProgramLines[Line - 1],
                                   std::string(Col - 2, ' ') + "^");
    std::cerr << FullMessage << '\n';
    exit(1);
  }

public:
  // For error reporting
  const std::vector<std::string> &getProgramLines();

  // Get current token and advance
  const Token *getTok();

  // Return next token, but don't advance
  const Token *peek();

  Lexer(const char *ModulePath);
};
