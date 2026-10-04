#include <llvm/Support/CommandLine.h>
#include <driver.hpp>
#include <common.hpp>

using namespace llvm;

static cl::opt<std::string>
  InputFilename(cl::Positional, cl::desc("input file"), cl::init("-"));

static cl::opt<bool>
  EmitLLVMIR("emit-llvm", cl::desc("Emit LLVM IR instead of X86 assembly"), cl::init(false));

#ifndef NDEBUG
static cl::opt<bool>
  EnableDbgMessages("debug", cl::desc("Enable output of information useful for debugging"), cl::init(false));
bool PrintDebugMessages = false;
#endif

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  #ifndef NDEBUG
  PrintDebugMessages = EnableDbgMessages;
  #endif

  std::unique_ptr<IDriver> Driver = nullptr;
  if (EmitLLVMIR) {
    Driver = std::make_unique<LLVMDriver>(InputFilename, OutputFilename);
  } else {
    Driver = std::make_unique<X86Driver>(InputFilename, OutputFilename);
  }
  Driver->compile();
}
