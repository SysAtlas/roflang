#include <llvm/Support/CommandLine.h>
#include <driver.hpp>

using namespace llvm;

static cl::opt<std::string>
  InputFilename(cl::Positional, cl::desc("input file"), cl::init("-"));

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  LLVMDriver Driver{InputFilename, OutputFilename};
  Driver.compile();
}
