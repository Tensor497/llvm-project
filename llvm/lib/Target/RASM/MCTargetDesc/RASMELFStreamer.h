//==-- RASMELFStreamer.h - RASM ELF Target Streamer --*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMELFSTREAMER_H
#define LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMELFSTREAMER_H

#include "RASMTargetStreamer.h"
#include "llvm/MC/MCELFStreamer.h"

namespace llvm {

class RASMTargetELFStreamer : public RASMTargetStreamer {
public:
  MCELFStreamer &getStreamer();
  RASMTargetELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI);

  void finish() override;
};

MCELFStreamer *createRASMELFStreamer(MCContext &C,
                                          std::unique_ptr<MCAsmBackend> MAB,
                                          std::unique_ptr<MCObjectWriter> MOW,
                                          std::unique_ptr<MCCodeEmitter> MCE,
                                          bool RelaxAll);
} // end namespace llvm
#endif
