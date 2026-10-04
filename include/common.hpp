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

extern bool PrintDebugMessages;

inline void debugPrint_(std::string_view Msg) {
  if (PrintDebugMessages)
    std::cerr << Msg << '\n';
}

#define DBGPRINT(x) debugPrint_(x)

#else

#define DBGPRINT(x) ((void)0)

#endif

enum class BinOpType { Add, Sub, Mul, Div, Lt, Leq, Gt, Geq, Eq, Neq, Mod };
enum class RLType { I64, I32, I16, I8, U64, U32, U16, U8, Void };

struct BinOpInfo {
  const BinOpType Op;
  const u32 Precedence;
  const std::string Repr;
};

struct RLTypeInfo {
  const RLType Type;
  const std::string Repr;
  u32 SizeInBits;
};

struct SourceLocation {
  u32 Line;
  u32 Col;
};

const BinOpInfo *searchBinOpInfoTable(BinOpType BinOp);
const BinOpInfo *searchBinOpInfoTable(const std::string &SV);

const RLTypeInfo *searchRLTypeInfoTable(RLType Type);
const RLTypeInfo *searchRLTypeInfoTable(const std::string &SV);