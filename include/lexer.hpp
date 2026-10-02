#pragma once

#include "my_types.hpp"
#include <fstream>
#include <optional>
#include <string>
#include <variant>

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

std::optional<u32> getBinOpPrededenece(char BinOp);

class Lexer {
private:
  u32 Line = 1;
  u32 Col = 1;

  i32 LastChar;
  i32 PrevChar = ' ';

  std::ifstream ProgramStream;

  void updateLineCounter();

  // Set LastChar to next char of ProgramStream and return LastChar
  char nextChar();

public:
  struct EOFToken {
    std::string PrettyRepr_ = "EOF";
  };

  // Reserved keywords
  struct KeywordDefToken {
    std::string PrettyRepr_ = "def";
  };
  struct KeywordExternToken {
    std::string PrettyRepr_ = "extern";
  };
  struct KeywordIfToken {
    std::string PrettyRepr_ = "if";
  };
  struct KeywordElseToken {
    std::string PrettyRepr_ = "else";
  };
  struct KeywordReturnToken {
    std::string PrettyRepr_ = "return";
  };
  // Types
  struct KeywordVoidToken {
    std::string PrettyRepr_ = "void";
  };
  struct KeywordI64Token {
    std::string PrettyRepr_ = "i64";
  };

  // -----------------

  struct IdentifierToken {
    std::string Name_;
    std::string PrettyRepr_;

    IdentifierToken(std::string_view Name) : Name_{Name}, PrettyRepr_{Name} {}
  };

  struct NumberToken {
    i64 NumVal_;
    std::string PrettyRepr_;

    NumberToken(i64 NumVal)
        : NumVal_{NumVal}, PrettyRepr_{std::to_string(NumVal_)} {}
  };

  struct BinOpToken {
    char Op_;
    std::string PrettyRepr_;

    BinOpToken(char Op) : Op_{Op}, PrettyRepr_(1, Op) {}
  };

  // Punctuation
  struct LParToken {
    std::string PrettyRepr_ = "(";
  };
  struct RParToken {
    std::string PrettyRepr_ = ")";
  };
  struct LCurlyBraceToken {
    std::string PrettyRepr_ = "{";
  };
  struct RCurlyBraceToken {
    std::string PrettyRepr_ = "}";
  };
  struct CommaToken {
    std::string PrettyRepr_ = ",";
  };
  struct SemicolonToken {
    std::string PrettyRepr_ = ";";
  };
  struct ArrowToken {
    std::string PrettyRepr_ = "->";
  };

  // New token types should always be added to this variant.
  class Token {
    using TokenType = std::variant<EOFToken,
                                   KeywordDefToken,
                                   KeywordExternToken,
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
                                   KeywordVoidToken,
                                   KeywordReturnToken, KeywordI64Token>;
    TokenType Value;

  public:
    const TokenType &getValue() {
      return Value;
    }

    template <typename T> bool is() const {
      return std::holds_alternative<T>(Value);
    }

    template <typename T> T get() {
      return std::get<T>(Value);
    }

    template <typename T> std::optional<T> getIf() {

      T *Res = get_if<T>(&Value);
      if (!Res) {
        return std::nullopt;
      }
      return *Res;
    }

    std::string_view print() const;

    void dump() const;

    template <typename T> Token(T Value) : Value(Value) {}
  };

  struct LocInfo {
    u32 Line;
    u32 Col;
  };

  LocInfo getLocInfo();
  Token getTok();

  Lexer(const char *ModulePath);
};
