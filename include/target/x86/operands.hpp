#pragma once

#include <target/x86/registers.hpp>
#include <unordered_map>
#include <variant>

namespace X86 {

class Immediate {
private:
  explicit Immediate(i32 Value) : Value{Value} {}

public:
  i32 Value;

  static const Immediate &get(i32 Value);
};

extern std::unordered_map<i32, Immediate> ImmediatePool;

std::string toString(const Register &R);
std::string toString(const Immediate &Imm);

struct Operand {
  using OperandT = std::variant<const Register *, const Immediate *>;
  OperandT Value;

  std::string toString() const {
    return std::visit<std::string>(
      overloaded{[](const auto *Arg) { return X86::toString(*Arg); }}, Value);
  }

  template <typename T> Operand(const T &Value) : Value{&Value} {}
};

// TODO: T is either register or immediate
struct OpInfo {
  Operand Op;
  bool IsOut = false;
  bool IsIn = true;

  OpInfo(Operand Op, bool IsOut) : Op{Op}, IsOut{IsOut} {}
};

} // namespace X86