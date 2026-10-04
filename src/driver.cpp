#include <frontend/sema.hpp>
#include <driver.hpp>

#include <codegen/llvmcodegen.hpp>
#include <codegen/x86codegen.hpp>
#include <llvm/IR/PassManager.h>
#include <frontend/parser.hpp>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

LLVMDriver::LLVMDriver(std::string_view InputFilePath, std::string_view OutputFilePath) : InputFilePath(InputFilePath), OutputFilePath(OutputFilePath) {}

void LLVMDriver::compile() {
  std::unique_ptr<AST::Module> AST = Parser::parse(InputFilePath.data());
  Sema::analyze(*AST);

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

X86Driver::X86Driver(std::string_view InputFilePath, std::string_view OutputFilePath) : InputFilePath(InputFilePath), OutputFilePath(OutputFilePath) {}

void X86Driver::compile() {
  std::unique_ptr<AST::Module> AST = Parser::parse(InputFilePath.data());
  Sema::analyze(*AST);

  std::string Result = X86::CodeGen::generateAsm(std::move(AST));
  
  if (OutputFilePath == "-") {
    std::cout << Result;
  } else {
    std::ofstream Res{OutputFilePath};
    Res << Result;
  }
}