#pragma once

#include "backend/x86/module.hpp"
#include "backend/x86/registers.hpp"

namespace X86 {

class CallPreservation {

std::unordered_map<Function*, std::set<Register>> preserve_before_call;
std::unordered_map<Function*, std::set<Register>> should_preserve;

void patchFunctions();
void initClobberMap();
void patchCallSites();

Module* mod;
std::unique_ptr<Builder> builder;

CallPreservation(Module *mod);

public:
static void runPass(Module* mod);

};

}