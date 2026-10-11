#pragma once

#include "backend/x86/module.hpp"
#include "backend/x86/registers.hpp"

namespace X86 {

class PrologEpilog {

Module* mod;
std::unique_ptr<Builder> builder;

PrologEpilog(Module *mod);

public:
static void runPass(Module* mod);

};

}