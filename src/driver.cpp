#include "backend/x86/callpreservation.hpp"
#include "backend/x86/livevariables.hpp"
#include <driver.hpp>
#include <frontend/sema.hpp>

#include <backend/llvm/llvmcodegen.hpp>
#include <backend/x86/isel.hpp>
#include <backend/x86/lsra.hpp>
#include <backend/x86/module.hpp>

#include <frontend/parser.hpp>
#include <llvm/IR/PassManager.h>

#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

LLVMDriver::LLVMDriver(std::string_view input_filepath,
                       std::string_view output_filepath)
    : input_filepath(input_filepath), output_filepath(output_filepath) {}

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

X86Driver::X86Driver(std::string_view input_filepath,
                     std::string_view output_filepath)
    : input_filepath(input_filepath), output_filepath(output_filepath) {}

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
  if (print_after_li) {
    std::cerr << "AFTER LIVE VARIABLE ANALYSIS\n";
    std::cerr << "---------------------------\n";
    for (auto &f : mod->functions) {
      std::cerr << "Live intervals for function " << f->name << '\n';
      std::vector<X86::LiveInterval> lints;
      for (const auto &li : f->live_int_map) {
        lints.push_back(li.second);
      }
      std::sort(
          lints.begin(), lints.end(),
          [](const X86::LiveInterval &first, const X86::LiveInterval &second) {
            return first.start < second.start;
          });

      for (const auto &li : lints) {
        std::cerr << li.toString() << '\n';
      }
      std::cerr << '\n';
    }
    std::cerr << mod->dbgString();
    std::cerr << "---------------------------\n";
  }
  X86::LSRA::runPass(mod.get());
  X86::CallPreservation::runPass(mod.get());

  std::string result = mod->toString();

  if (output_filepath == "-") {
    std::cout << result;
  } else {
    std::ofstream res{output_filepath};
    res << result;
  }
}