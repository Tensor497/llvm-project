//===-- RASMTargetStreamer.h - RASM Target Streamer --*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMTARGETSTREAMER_H
#define LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMTARGETSTREAMER_H

#include "RASM.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {
class RASMTargetStreamer : public MCTargetStreamer {
  RASMABI::ABI TargetABI = RASMABI::ABI_Unknown;

public:
  RASMTargetStreamer(MCStreamer &S);
  void setTargetABI(RASMABI::ABI ABI);
  RASMABI::ABI getTargetABI() const { return TargetABI; }
};

} // end namespace llvm
#endif
