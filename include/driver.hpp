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
  std::string input_filepath;
  std::string output_filepath;

  public:
  void compile() override;
  LLVMDriver(std::string_view input_filepath, std::string_view output_filepath);
};

class X86Driver : public IDriver {
  private:
  std::string input_filepath;
  std::string output_filepath;

  public:
  void compile() override;
  X86Driver(std::string_view input_filepath, std::string_view output_filepath);
};