
#include "NewPlacement.hpp"
#include "Grammar.hpp"
#include "Strings.hpp"

using namespace tp;

static ModuleManifest* sModuleDependencies[] = { &gModuleStrings, nullptr };
ModuleManifest tp::gModuleLanguage = ModuleManifest("CommandLine", nullptr, nullptr, sModuleDependencies);