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
  static constexpr std::string TOKEN_NAME = "EOF";
};

// Reserved keywords
struct KeywordFnToken {
  static constexpr std::string_view TOKEN_NAME = "def";
};
struct KeywordIfToken {
  static constexpr std::string_view TOKEN_NAME = "if";
};
struct KeywordElseToken {
  static constexpr std::string_view TOKEN_NAME = "else";
};
struct KeywordReturnToken {
  static constexpr std::string_view TOKEN_NAME = "return";
};
struct KeywordWhileToken {
  static constexpr std::string_view TOKEN_NAME = "while";
};

// Types
struct RLTypeToken {
  const RLTypeInfo *info;
  static constexpr std::string_view TOKEN_NAME = "type";

  RLTypeToken(const RLType &type) : info(searchRLTypeInfoTable(type)) {}
};

// -----------------

struct IdentifierToken {
  std::string name;
  static constexpr std::string_view TOKEN_NAME = "identifier";

  IdentifierToken(std::string_view name) : name{name} {}
};

struct NumberToken {
  i64 num_val;
  static constexpr std::string_view TOKEN_NAME = "number";

  NumberToken(i64 num_val) : num_val{num_val} {}
};

struct BinOpToken {
  const BinOpInfo *info;
  static constexpr std::string_view TOKEN_NAME = "binary operator";

  BinOpToken(BinOp op) : info{searchBinOpInfoTable(op)} {}
};

// Punctuation
struct LParToken {
  static constexpr std::string_view TOKEN_NAME = "(";
};
struct RParToken {
  static constexpr std::string_view TOKEN_NAME = ")";
};
struct LCurlyBraceToken {
  static constexpr std::string_view TOKEN_NAME = "{";
};
struct RCurlyBraceToken {
  static constexpr std::string_view TOKEN_NAME = "}";
};
struct CommaToken {
  static constexpr std::string_view TOKEN_NAME = ",";
};
struct SemicolonToken {
  static constexpr std::string_view TOKEN_NAME = ";";
};
struct ArrowToken {
  static constexpr std::string_view TOKEN_NAME = "->";
};
struct ColonToken {
  static constexpr std::string_view TOKEN_NAME = ":";
};
struct EqualsToken {
  static constexpr std::string_view TOKEN_NAME = "=";
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

  TokenType value;

public:
  SourceLocation source_location{};

  const TokenType &getValue() const {
    return value;
  }

  template <typename T> bool is() const {
    return std::holds_alternative<T>(value);
  }

  template <typename T> const T *get() const {
    return &std::get<T>(value);
  }

  template <typename T> const T *getIf() const {
    auto *res = get_if<T>(&value);
    if (!res) {
      return nullptr;
    }
    return res;
  }

  std::string_view print() const;

  void dump() const;

  template <typename T> Token(T value) : value(value) {}
};

class Lexer {
private:
  std::vector<Token> tokens;
  const Token *cur_tok = nullptr;
  const char *module_path;

  u32 line = 1;
  u32 col = 0;

  SourceLocation getLocInfo();
  SourceLocation last_source_loc;

  i32 last_char;

  std::ifstream program_stream;
  std::vector<std::string> program_lines;

  void updateLineCounter();

  // Set LastChar to next char of ProgramStream and return LastChar
  i32 nextChar();
  i32 peekChar();

  Token consumeTok();

  void lex();

  void logError(std::string_view str) {
    auto full_message = std::format("Lexer error: {}\nAt {}:{}\n{}\n{}",
                                   str,
                                   module_path,
                                   line,
                                   program_lines[line - 1],
                                   std::string(col - 1, ' ') + "^");
    std::cerr << full_message << '\n';
  }

public:
  // For error reporting
  const std::vector<std::string> &getProgramLines();

  // Get current token and advance
  const Token *getTok();

  // Return next token, but don't advance
  const Token *peek();

  Lexer(const char *module_path);
};
