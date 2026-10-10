#pragma once

#include <backend/x86/registers.hpp>
#include <common.hpp>
#include <unordered_map>
#include <variant>

namespace X86 {

class BasicBlock;
class Function;

class Immediate {
public:
  i32 value;
  explicit Immediate(i32 value) : value{value} {}
};

class Label {
public:
  BasicBlock &bb;

  explicit Label(BasicBlock &bb) : bb{bb} {}
};

class FunctionRef {
public:
  const Function &fn;

  explicit FunctionRef(const Function &fn) : fn{fn} {}
};

std::string toString(const Register &reg);
std::string toString(const Immediate &imm);
std::string toString(const Label &label);
std::string toString(const FunctionRef &fn_ref);

enum class AccessType { DST, SRC, DSTSRC };

// TODO: T is either register or immediate
struct OpInfo {
  using Operand = std::variant<Register, Immediate, Label, FunctionRef>;
  Operand op;
  AccessType access_type;

  std::string toString() const {
    return std::visit<std::string>(
        overloaded{
            [](const auto &arg) -> std::string { return X86::toString(arg); }},
        op);
  }

  bool writes() {
    return access_type == AccessType::DST || access_type == AccessType::DSTSRC;
  }

  bool reads() {
    return access_type == AccessType::SRC || access_type == AccessType::DSTSRC;
  }

  template <typename T> bool is() {
    return std::holds_alternative<Register>(op);
  }

  template <typename T> T get() {
    return std::get<T>(op);
  }

  template <typename T> T getIf() const {
    auto *res = get_if<T>(&op);
    if (!res) {
      return nullptr;
    }
    return *res;
  }

  template <typename T>
  OpInfo(const T &value, AccessType access_type)
      : op{value}, access_type(access_type) {}
  OpInfo(Label &&value, AccessType access_type)
      : op{value}, access_type(access_type) {}
};

} // namespace X86