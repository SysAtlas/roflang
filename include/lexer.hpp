#pragma once

#include "my_types.hpp"
#include <fstream>
#include <string>
#include <variant>
#include <optional>

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
  struct EOFToken {};

  // Reserved keywords
  struct DefToken {};
  struct ExternToken {};
  // -----------------

  struct IdentifierToken {
    std::string Name;
  };

  struct NumberToken {
    f64 NumVal;
  };

  struct BinOpToken {
    char Op;
  };

  struct LParToken {};
  struct RParToken {};
  struct CommaToken {};
  struct SemicolonToken{};

  // New token types should always be added to this variant.
  class Token {
    using TokenType =
      std::variant<EOFToken, DefToken, ExternToken, IdentifierToken, NumberToken, BinOpToken, LParToken, RParToken, CommaToken, SemicolonToken>;
    TokenType Value;

    public:

    const TokenType& getValue() {
      return Value;
    }

    template <typename T>
    bool is() {
      return std::holds_alternative<T>(Value);
    }

    template <typename T>
    T get() {
      return std::get<T>(Value);
    }

    template <typename T>
    std::optional<T> getIf() {

      T* Res = get_if<T>(&Value);
      if (!Res) {
        return std::nullopt;
      }
      return *Res;
    }

    template <typename T>
    Token(T Value) : Value(Value) {}
  };

  struct LocInfo {
    u32 Line;
    u32 Col;
  };

  LocInfo getLocInfo();
  Token getTok();

  Lexer(const char *ModulePath);
};
