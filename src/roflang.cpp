#include <llvm/Support/CommandLine.h>
#include <driver.hpp>
#include <common.hpp>

using namespace llvm;

static cl::opt<std::string>
  input_filename(cl::Positional, cl::desc("input file"), cl::init("-"));

static cl::opt<bool>
  emit_llvm_ir("emit-llvm", cl::desc("Emit LLVM IR instead of X86 assembly"), cl::init(false));

#ifndef NDEBUG
static cl::opt<bool>
  enable_dbg_messages("debug", cl::desc("Enable output of information useful for debugging"), cl::init(false));
bool print_debug_messages = false;
#endif

static cl::opt<std::string> output_filename("o",
                                           cl::desc("Output filename"),
                                           cl::value_desc("filename"),
                                           cl::init("-"));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  #ifndef NDEBUG
  print_debug_messages = enable_dbg_messages;
  #endif

  std::unique_ptr<IDriver> driver = nullptr;
  if (emit_llvm_ir) {
    driver = std::make_unique<LLVMDriver>(input_filename, output_filename);
  } else {
    driver = std::make_unique<X86Driver>(input_filename, output_filename);
  }
  driver->compile();
}
