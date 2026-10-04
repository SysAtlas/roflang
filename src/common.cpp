#include <common.hpp>

static constexpr std::array BinOpInfoTable = {
  BinOpInfo{BinOpType::Add, 20, "+"},
  BinOpInfo{BinOpType::Sub, 20, "-"},
  BinOpInfo{BinOpType::Mul, 40, "*"},
  BinOpInfo{BinOpType::Div, 40, "/"},
  BinOpInfo{BinOpType::Mod, 40, "%"},
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