#include "RASM.h"
#include "CommonArgs.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/ArgList.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

RASMToolChain::RASMToolChain(const Driver &D, const llvm::Triple &Triple,
                               const ArgList &Args)
    : ToolChain(D, Triple, Args) {
  // ProgramPaths are found via 'PATH' environment variable.
}

bool RASMToolChain::isPICDefault() const { return true; }

bool RASMToolChain::isPIEDefault() const { return false; }

bool RASMToolChain::isPICDefaultForced() const { return true; }

bool RASMToolChain::SupportsProfiling() const { return false; }

bool RASMToolChain::hasBlocksRuntime() const { return false; }