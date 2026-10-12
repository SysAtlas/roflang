#include <fstream>
#include <common.hpp>
#include <iostream>
#include <frontend/lexer.hpp>

i32 Lexer::nextChar() {
  if (last_char == '\n') {
    ++line;
    col = 1;
  } else {
    ++col;
  }
  last_char = program_stream.get();
  return last_char;
}

i32 Lexer::peekChar() {
  return program_stream.peek();
}

SourceLocation Lexer::getLocInfo() {
  return {line, col};
}

Token Lexer::consumeTok() {
  // Skip any whitespace.
  while (std::isspace(last_char)) {
    nextChar();
  }

  // identifier: [a-zA-Z][a-zA-Z0-9]*
  if (std::isalpha(last_char) || last_char == '_') {
    std::string identifier_str;
    do {
      identifier_str += static_cast<char>(last_char);
      nextChar();
    } while (std::isalnum(last_char) || last_char == '_');

    // TODO: Make centralized tables.. this is too much effort
    if (identifier_str == "fn") {
      return KeywordFnToken{};
    }
    if (identifier_str == "if") {
      return KeywordIfToken{};
    }
    if (identifier_str == "else") {
      return KeywordElseToken{};
    }
    if (identifier_str == "return") {
      return KeywordReturnToken{};
    }
    if (identifier_str == "while") {
      return KeywordWhileToken{};
    }
    if (const RLTypeInfo *type_info = searchRLTypeInfoTable(identifier_str)) {
      return RLTypeToken{type_info->type};
    }

    return IdentifierToken{identifier_str};
  }

  if (std::isdigit(last_char) || last_char == '.') { // Number: [0-9.]+
    std::string num_str;
    do {
      num_str += static_cast<char>(last_char);
      nextChar();
    } while (isdigit(last_char) || last_char == '.');

    // TODO: ERROR HANDLING
    i64 num_val = std::strtoll(num_str.c_str(), nullptr, 10);

    return NumberToken{num_val};
  }

  if (last_char == '/' && peekChar() == '/') {
    do {
      nextChar();
    } while (last_char != EOF && last_char != '\n' && last_char != '\r');

    if (last_char != EOF) {
      return consumeTok();
    }
  } 
  // TODO: Multi-line comments

  std::optional<Token> single_char_token;
  i32 to_match = last_char;
  nextChar();
  if (to_match == ',') {
    return CommaToken{};
  } else if (to_match == '-') {
    if (last_char == '>') {
      nextChar();
      return ArrowToken{};
    }
    return BinOpToken{BinOp::SUB};
  } else if (const BinOpInfo* t = searchBinOpInfoTable(std::string{(char) to_match, (char) last_char})) {
    // Bin ops of size 2
    nextChar();
    return BinOpToken{t->op};
    // Bin ops of size 1
  } else if (const BinOpInfo *t = searchBinOpInfoTable(std::string(1, static_cast<char>(to_match)))) {
    return BinOpToken{t->op};
  } else if (to_match == ';') {
    return SemicolonToken{};
  } else if (to_match == '(') {
    return LParToken{};
  } else if (to_match == ')') {
    return RParToken{};
  } else if (to_match == EOF) {
    return EOFToken{};
  } else if (to_match == '{') {
    return LCurlyBraceToken{};
  } else if (to_match == '}') {
    return RCurlyBraceToken{};
  } else if (to_match == ':') {
    return ColonToken{};
  } else if (to_match == '=') {
    if (last_char == '=') {
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
    SourceLocation source_location = getLocInfo();
    tokens.push_back(consumeTok());
    tokens.back().source_location = source_location;
    DBGPRINT(tokens.back().print());
  } while (!tokens.back().is<EOFToken>());
  cur_tok = tokens.data();
}

const std::vector<std::string> &Lexer::getProgramLines() {
  return program_lines;
}
const Token *Lexer::getTok() {
  const Token* res = cur_tok;
  ++cur_tok;
  return res;
}

const Token* Lexer::peek() {
  return cur_tok;
}

Lexer::Lexer(const char* module_path)
    : module_path(module_path), program_stream{std::ifstream{module_path}} {
  if (program_stream.fail()) {
    std::cerr << "Failure opening file " << module_path << std::endl;
    std::exit(1);
  }
  std::ifstream tmp{module_path};
  std::string line;
  while (std::getline(tmp, line)) { 
    program_lines.push_back(line);
  }

  nextChar();
  lex();
}

std::string_view Token::print() const {
  return std::visit<std::string_view>(
    overloaded{[](const BinOpToken &arg) -> std::string_view { return arg.info->repr; },
               [](const RLTypeToken &arg) -> std::string_view { return arg.info->repr; },
               [](const auto &value) -> std::string_view {
                 return value.TOKEN_NAME;
               }},
    value);
}

void Token::dump() const {
  std::cerr << print() << std::endl;
}
