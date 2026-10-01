#include <codegen.hpp>
#include <parser.hpp>

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

static cl::opt<std::string>
  InputFilename(cl::Positional, cl::desc("input file"), cl::init("-"));

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  std::unique_ptr<ModuleAST> AST = Parser::parse(InputFilename.data());

  LLVMContext Context{};
  std::unique_ptr<llvm::Module> M =
    CodeGenerator::generate(std::move(AST), Context);

  if (OutputFilename == "-") {
    M->print(llvm::errs(), nullptr);
  } else {
    std::error_code EC;
    llvm::raw_fd_stream Result{OutputFilename, EC};
    M->print(Result, nullptr);
  }

  return 0;
}
