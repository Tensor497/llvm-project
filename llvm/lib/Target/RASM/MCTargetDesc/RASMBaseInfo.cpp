//= RASMBaseInfo.cpp - Top level definitions for RASM MC -*- C++ -*-//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements helper functions for the RASM target useful for the
// compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//

#include "RASMBaseInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

namespace RASMABI {

ABI getTargetABI(StringRef ABIName) {
  auto TargetABI = StringSwitch<ABI>(ABIName)
                       .Case("ilp32s", ABI_ILP32S)
                       .Case("ilp32f", ABI_ILP32F)
                       .Case("ilp32d", ABI_ILP32D)
                       .Case("lp64s", ABI_LP64S)
                       .Case("lp64f", ABI_LP64F)
                       .Case("lp64d", ABI_LP64D)
                       .Default(ABI_Unknown);
  return TargetABI;
}

// FIXME: other register?
MCRegister getBPReg() { return RASM::R31; }

} // end namespace RASMABI

} // end namespace llvm
