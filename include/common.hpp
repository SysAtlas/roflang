#pragma once

#include <cstdint>

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

using i64 = std::int64_t;
using i32 = std::int32_t;
using i16 = std::int16_t;
using i8 = std::int8_t;

using f64 = double;
using f32 = float;
using usize = std::size_t;

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

#ifndef NDEBUG

#include <iostream>

[[noreturn]] inline void TODO() {
  std::cerr << "TODO!" << '\n';
  std::exit(1);
}

extern bool print_debug_messages;

inline void debugPrint_(std::string_view msg) {
  if (print_debug_messages) {
    std::cerr << msg << '\n';
  }
}

#define DBGPRINT(x) debugPrint_(x)

#else

#define DBGPRINT(x) ((void)0)

#endif

enum class BinOp { ADD, SUB, MUL, DIV, LT, LEQ, GT, GEQ, EQ, NEQ, MOD };
enum class RLType { I64, I32, I16, I8, U64, U32, U16, U8, VOID };

struct BinOpInfo {
  const BinOp op;
  const u32 precedence;
  const std::string repr;
};

struct RLTypeInfo {
  const RLType type;
  const std::string repr;
  u32 size_in_bits;
};

struct SourceLocation {
  u32 line;
  u32 col;
};

const BinOpInfo *searchBinOpInfoTable(BinOp bin_op);
const BinOpInfo *searchBinOpInfoTable(const std::string &sv);

const RLTypeInfo *searchRLTypeInfoTable(RLType type);
const RLTypeInfo *searchRLTypeInfoTable(const std::string &sv);