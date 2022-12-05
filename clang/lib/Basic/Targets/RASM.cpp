#include "RASM.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

const char *const RASMTargetInfo::GCCRegNames[] = {
  // Integer registers
  "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
  "8",  "9",  "10", "11", "12", "13", "14", "15",
};

const TargetInfo::GCCRegAlias GCCRegAliases[] = {
  {{"zero"}, "0"}, {{"ra"}, "1"},   {{"sp"}, "2"},    {{"gp"}, "3"},
  {{"tp"}, "4"},   {{"t0"}, "5"},   {{"t1"}, "6"},    {{"t2"}, "7"},
  {{"s0"}, "8"},   {{"s1"}, "9"},   {{"a0"}, "10"},   {{"a1"}, "11"},
  {{"a2"}, "12"},  {{"a3"}, "13"},  {{"a4"}, "14"},   {{"a5"}, "15"},
};

ArrayRef<const char *> RASMTargetInfo::getGCCRegNames() const {
  return llvm::makeArrayRef(GCCRegNames);
}

ArrayRef<TargetInfo::GCCRegAlias> RASMTargetInfo::getGCCRegAliases() const {
  return llvm::makeArrayRef(GCCRegAliases);
}

void RASMTargetInfo::getTargetDefines(const LangOptions &Opts, MacroBuilder &Builder) const {
  Builder.defineMacro("__RASM__");
}