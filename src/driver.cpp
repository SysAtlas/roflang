#include <driver.hpp>

#include <llvmcodegen.hpp>
#include <llvm/IR/PassManager.h>
#include <parser.hpp>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

LLVMDriver::LLVMDriver(std::string_view InputFilePath, std::string_view OutputFilePath) : InputFilePath(InputFilePath), OutputFilePath(OutputFilePath) {}

void LLVMDriver::compile() {
  std::unique_ptr<ModuleAST> AST = Parser::parse(InputFilePath.data());

  LLVMContext TheContext{};
  std::unique_ptr<llvm::Module> M =
    LLVMCodeGen::generate(std::move(AST), TheContext);
  
  if (OutputFilePath == "-") {
    M->print(llvm::errs(), nullptr);
  } else {
    std::error_code EC;
    llvm::raw_fd_stream Result{OutputFilePath, EC};
    M->print(Result, nullptr);
  }
}