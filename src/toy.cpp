#include <llvm/Support/CommandLine.h>
#include <driver.hpp>
#include <helper.hpp>

using namespace llvm;

static cl::opt<std::string>
  InputFilename(cl::Positional, cl::desc("input file"), cl::init("-"));

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

  LLVMDriver Driver{InputFilename, OutputFilename};
  Driver.compile();
}
