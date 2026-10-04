#pragma once

#include <string>

class IDriver {
  public:
  virtual void compile() = 0;
  virtual ~IDriver() = default;
};

/// Calls frontend and LLVM-based backend
class LLVMDriver : public IDriver {
  private:
  std::string InputFilePath;
  std::string OutputFilePath;

  public:
  void compile() override;
  LLVMDriver(std::string_view InputFilePath, std::string_view OutputFilePath);
};

class X86Driver : public IDriver {
  private:
  std::string InputFilePath;
  std::string OutputFilePath;

  public:
  void compile() override;
  X86Driver(std::string_view InputFilePath, std::string_view OutputFilePath);
};