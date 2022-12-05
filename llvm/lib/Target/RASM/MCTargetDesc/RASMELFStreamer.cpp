//===-- RASMELFStreamer.cpp - RASM ELF Target Streamer Methods --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides RASM specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "RASMELFStreamer.h"
#include "RASMAsmBackend.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCObjectWriter.h"

using namespace llvm;

// This part is for ELF object output.
RASMTargetELFStreamer::RASMTargetELFStreamer(
    MCStreamer &S, const MCSubtargetInfo &STI)
    : RASMTargetStreamer(S) {
  // FIXME: select appropriate ABI.
  setTargetABI(STI.getTargetTriple().isArch64Bit() ? RASMABI::ABI_LP64D
                                                   : RASMABI::ABI_ILP32D);
}

MCELFStreamer &RASMTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer &>(Streamer);
}

void RASMTargetELFStreamer::finish() {
  RASMTargetStreamer::finish();
  MCAssembler &MCA = getStreamer().getAssembler();
  RASMABI::ABI ABI = getTargetABI();

  // Figure out the e_flags.
  //
  // Bitness is already represented with the EI_CLASS byte in the current spec,
  // so here we only record the base ABI modifier. Also set the object file ABI
  // version to v1, as upstream LLVM cannot handle the previous stack-machine-
  // based relocs from day one.
  //
  // Refer to RASM ELF psABI v2.01 for details.
  unsigned EFlags = MCA.getELFHeaderEFlags();
  EFlags |= ELF::EF_RASM_OBJABI_V1;
  switch (ABI) {
  case RASMABI::ABI_ILP32S:
  case RASMABI::ABI_LP64S:
    EFlags |= ELF::EF_RASM_ABI_SOFT_FLOAT;
    break;
  case RASMABI::ABI_ILP32F:
  case RASMABI::ABI_LP64F:
    EFlags |= ELF::EF_RASM_ABI_SINGLE_FLOAT;
    break;
  case RASMABI::ABI_ILP32D:
  case RASMABI::ABI_LP64D:
    EFlags |= ELF::EF_RASM_ABI_DOUBLE_FLOAT;
    break;
  case RASMABI::ABI_Unknown:
    llvm_unreachable("Improperly initialized target ABI");
  }
  MCA.setELFHeaderEFlags(EFlags);
}

namespace {
class RASMELFStreamer : public MCELFStreamer {
public:
  RASMELFStreamer(MCContext &C, std::unique_ptr<MCAsmBackend> MAB,
                       std::unique_ptr<MCObjectWriter> MOW,
                       std::unique_ptr<MCCodeEmitter> MCE)
      : MCELFStreamer(C, std::move(MAB), std::move(MOW), std::move(MCE)) {}
};
} // end namespace

namespace llvm {
MCELFStreamer *createRASMELFStreamer(MCContext &C,
                                          std::unique_ptr<MCAsmBackend> MAB,
                                          std::unique_ptr<MCObjectWriter> MOW,
                                          std::unique_ptr<MCCodeEmitter> MCE,
                                          bool RelaxAll) {
  RASMELFStreamer *S = new RASMELFStreamer(
      C, std::move(MAB), std::move(MOW), std::move(MCE));
  S->getAssembler().setRelaxAll(RelaxAll);
  return S;
}
} // end namespace llvm
