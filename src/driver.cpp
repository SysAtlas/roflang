#include "backend/x86/livevariables.hpp"
#include <frontend/sema.hpp>
#include <driver.hpp>

#include <backend/llvm/llvmcodegen.hpp>
#include <backend/x86/isel.hpp>
#include <backend/x86/lsra.hpp>
#include <backend/x86/module.hpp>

#include <llvm/IR/PassManager.h>
#include <frontend/parser.hpp>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

LLVMDriver::LLVMDriver(std::string_view input_filepath, std::string_view output_filepath) : input_filepath(input_filepath), output_filepath(output_filepath) {}

void LLVMDriver::compile() {
  std::unique_ptr<AST::Module> ast = Parser::parse(input_filepath.data());
  Sema::analyze(*ast);

  LLVMContext the_context{};
  std::unique_ptr<llvm::Module> M =
    LLVMCodeGen::generate(std::move(ast), the_context);
  
  if (output_filepath == "-") {
    M->print(llvm::errs(), nullptr);
  } else {
    std::error_code ec;
    llvm::raw_fd_stream result{output_filepath, ec};
    M->print(result, nullptr);
  }
}

X86Driver::X86Driver(std::string_view input_filepath, std::string_view output_filepath) : input_filepath(input_filepath), output_filepath(output_filepath) {}

void X86Driver::compile() {
  std::unique_ptr<AST::Module> ast = Parser::parse(input_filepath.data());
  Sema::analyze(*ast);

  std::unique_ptr<X86::Module> mod = X86::ISel::runPass(std::move(ast));
  if (print_after_isel) {
    std::cerr << "AFTER ISEL\n";
    std::cerr << "---------------------------\n";
    std::cerr << mod->toString();
    std::cerr << "---------------------------\n";
  }
  X86::LiveVariableAnalysis::runPass(mod.get());
  X86::LSRA::runPass(mod.get());

  std::string result = mod->toString();
  
  if (output_filepath == "-") {
    std::cout << result;
  } else {
    std::ofstream res{output_filepath};
    res << result;
  }
}