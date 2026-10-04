#pragma once

#include <common.hpp>

namespace X86 {

struct Register {
  Register() = default;
  Register(const Register &) = delete;
  const std::string Name;
  Register(const std::string& Name) : Name{Name} {}
};

template <u32 SizeP> struct GPRegister : public Register {
  static constexpr u32 Size = SizeP;

  GPRegister(const std::string &Name) : Register(Name) {}
};

struct RAX : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 0;
  RAX() : GPRegister("rax") {}
};

struct RBX : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 1;
  RBX() : GPRegister("rbx") {}
};

struct RCX : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 2;
  RCX() : GPRegister("rcx") {}
};

struct RDX : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 3;
  RDX() : GPRegister("rdx") {}
};

struct R8 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 4;
  R8() : GPRegister("r8") {}
};
struct R9 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 5;
  R9() : GPRegister("r9") {}
};
struct R10 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 6;
  R10() : GPRegister("r10") {}
};
struct R11 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 7;
  R11() : GPRegister("r11") {}
};
struct R12 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 8;
  R12() : GPRegister("r12") {}
};
struct R13 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 9;
  R13() : GPRegister("r13") {}
};
struct R14 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 10;
  R14() : GPRegister("r14") {}
};
struct R15 : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 11;
  R15() : GPRegister("r15") {}
};
struct RDI : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 12;
  RDI() : GPRegister("rdi") {}
};
struct RSI : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 13;
  RSI() : GPRegister("rsi") {}
};
struct RSP : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 14;
  RSP() : GPRegister("rsp") {}
};
struct RBP : public GPRegister<64> {
  static constexpr u8 RegisterFilePosition = 15;
  RBP() : GPRegister("rbp") {}
};

struct EAX : public GPRegister<32> {};
struct EBX : public GPRegister<32> {};
struct ECX : public GPRegister<32> {};
struct EDX : public GPRegister<32> {};
struct R8D : public GPRegister<32> {};
struct R9D : public GPRegister<32> {};
struct R10D : public GPRegister<32> {};
struct R11D : public GPRegister<32> {};
struct R12D : public GPRegister<32> {};
struct R13D : public GPRegister<32> {};
struct R14D : public GPRegister<32> {};
struct R15D : public GPRegister<32> {};
struct EDI : public GPRegister<32> {};
struct ESI : public GPRegister<32> {};
struct ESP : public GPRegister<32> {};
struct EBP : public GPRegister<32> {};

struct AX : public GPRegister<16> {};
struct BX : public GPRegister<16> {};
struct CX : public GPRegister<16> {};
struct DX : public GPRegister<16> {};
struct R8W : public GPRegister<16> {};
struct R9W : public GPRegister<16> {};
struct R10W : public GPRegister<16> {};
struct R11W : public GPRegister<16> {};
struct R12W : public GPRegister<16> {};
struct R13W : public GPRegister<16> {};
struct R14W : public GPRegister<16> {};
struct R15W : public GPRegister<16> {};
struct DI : public GPRegister<16> {};
struct SI : public GPRegister<16> {};
struct SP : public GPRegister<16> {};
struct BP : public GPRegister<16> {};

struct AL : public GPRegister<8> {};
struct BL : public GPRegister<8> {};
struct CL : public GPRegister<8> {};
struct DL : public GPRegister<8> {};
struct R8B : public GPRegister<8> {};
struct R9B : public GPRegister<8> {};
struct R10B : public GPRegister<8> {};
struct R11B : public GPRegister<8> {};
struct R12B : public GPRegister<8> {};
struct R13B : public GPRegister<8> {};
struct R14B : public GPRegister<8> {};
struct R15B : public GPRegister<8> {};
struct DIL : public GPRegister<8> {};
struct SIL : public GPRegister<8> {};
struct SPL : public GPRegister<8> {};
struct BPL : public GPRegister<8> {};

struct AH : public GPRegister<8> {};
struct BH : public GPRegister<8> {};
struct CH : public GPRegister<8> {};
struct DH : public GPRegister<8> {};

struct RF {
  static const std::array<const Register *, 16> Registers;

  template <typename T> static const T& get() {
    return static_cast<const T&>(*Registers[T::RegisterFilePosition]);
  }

  // Encode call convention here.
  static const RAX &getReturnRegister();
};

} // namespace X86