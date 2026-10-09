#include <common.hpp>

static constexpr std::array BinOpInfoTable = {
  BinOpInfo{BinOp::ADD, 20, "+"},
  BinOpInfo{BinOp::SUB, 20, "-"},
  BinOpInfo{BinOp::MUL, 40, "*"},
  BinOpInfo{BinOp::DIV, 40, "/"},
  BinOpInfo{BinOp::MOD, 40, "%"},
  BinOpInfo{BinOp::LT, 10, "<"},
  BinOpInfo{BinOp::LEQ, 10, "<="},
  BinOpInfo{BinOp::GT, 10, ">"},
  BinOpInfo{BinOp::GEQ, 10, ">="},
  BinOpInfo{BinOp::EQ, 10, "=="},
  BinOpInfo{BinOp::NEQ, 10, "!="},
};

static constexpr std::array RLTypeInfoTable = {
  RLTypeInfo{RLType::VOID, "void", 0},
  RLTypeInfo{RLType::I64, "i64", 64},
  RLTypeInfo{RLType::I32, "i32", 32},
  RLTypeInfo{RLType::I16, "i16", 16},
  RLTypeInfo{RLType::I8, "i8", 8},
  RLTypeInfo{RLType::U64, "u64", 64},
  RLTypeInfo{RLType::U32, "u32", 32},
  RLTypeInfo{RLType::U16, "u16", 16},
  RLTypeInfo{RLType::U8, "u8", 8},
};

const BinOpInfo *searchBinOpInfoTable(BinOp bin_op) {
  if (auto element = std::ranges::find_if(
        BinOpInfoTable,
        [bin_op](const BinOpInfo &entry) { return entry.op == bin_op; });
      element != BinOpInfoTable.end()) {
    return element;
  }
  return nullptr;
}

const BinOpInfo *searchBinOpInfoTable(const std::string &SV) {
  if (auto element = std::ranges::find_if(
        BinOpInfoTable,
        [SV](const auto &entry) { return entry.repr == SV; });
      element != BinOpInfoTable.end()) {
    return element;
  }
  return nullptr;
}

const RLTypeInfo *searchRLTypeInfoTable(RLType type) {
  if (auto element = std::ranges::find_if(
        RLTypeInfoTable,
        [type](const RLTypeInfo &entry) { return entry.type == type; });
      element != RLTypeInfoTable.end()) {
    return element;
  }
  return nullptr;
}

const RLTypeInfo *searchRLTypeInfoTable(const std::string &sv) {
  if (auto element = std::ranges::find_if(
        RLTypeInfoTable,
        [sv](const RLTypeInfo &entry) { return entry.repr == sv; });
      element != RLTypeInfoTable.end()) {
    return element;
  }
  return nullptr;
}