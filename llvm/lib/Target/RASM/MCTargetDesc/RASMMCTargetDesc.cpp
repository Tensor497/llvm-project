//===-- RASMMCTargetDesc.cpp - RASM Target Descriptions ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides RASM specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "RASMMCTargetDesc.h"
#include "RASMBaseInfo.h"
#include "RASMELFStreamer.h"
#include "RASMInstPrinter.h"
#include "RASMMCAsmInfo.h"
#include "TargetInfo/RASMTargetInfo.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "RASMGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RASMGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "RASMGenSubtargetInfo.inc"

using namespace llvm;

static MCRegisterInfo *createRASMMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitRASMMCRegisterInfo(X, RASM::R1);
  return X;
}

static MCInstrInfo *createRASMMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitRASMMCInstrInfo(X);
  return X;
}

static MCSubtargetInfo *
createRASMMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty() || CPU == "generic")
    CPU = TT.isArch64Bit() ? "la464" : "generic-la32";
  return createRASMMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCAsmInfo *createRASMMCAsmInfo(const MCRegisterInfo &MRI,
                                           const Triple &TT,
                                           const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new RASMMCAsmInfo(TT);

  // Initial state of the frame pointer is sp(r3).
  MCRegister SP = MRI.getDwarfRegNum(RASM::R3, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createRASMMCInstPrinter(const Triple &T,
                                                   unsigned SyntaxVariant,
                                                   const MCAsmInfo &MAI,
                                                   const MCInstrInfo &MII,
                                                   const MCRegisterInfo &MRI) {
  return new RASMInstPrinter(MAI, MII, MRI);
}

static MCTargetStreamer *
createRASMObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  return STI.getTargetTriple().isOSBinFormatELF()
             ? new RASMTargetELFStreamer(S, STI)
             : nullptr;
}

namespace {

class RASMMCInstrAnalysis : public MCInstrAnalysis {
public:
  explicit RASMMCInstrAnalysis(const MCInstrInfo *Info)
      : MCInstrAnalysis(Info) {}

  bool evaluateBranch(const MCInst &Inst, uint64_t Addr, uint64_t Size,
                      uint64_t &Target) const override {
    unsigned NumOps = Inst.getNumOperands();
    if (isBranch(Inst) || Inst.getOpcode() == RASM::BL) {
      Target = Addr + Inst.getOperand(NumOps - 1).getImm();
      return true;
    }

    return false;
  }
};

} // end namespace

static MCInstrAnalysis *createRASMInstrAnalysis(const MCInstrInfo *Info) {
  return new RASMMCInstrAnalysis(Info);
}

namespace {
MCStreamer *createRASMELFStreamer(const Triple &T, MCContext &Context,
                                       std::unique_ptr<MCAsmBackend> &&MAB,
                                       std::unique_ptr<MCObjectWriter> &&MOW,
                                       std::unique_ptr<MCCodeEmitter> &&MCE,
                                       bool RelaxAll) {
  return createRASMELFStreamer(Context, std::move(MAB), std::move(MOW),
                                    std::move(MCE), RelaxAll);
}
} // end namespace

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRASMTargetMC() {
  for (Target *T : {&getTheRASM32Target(), &getTheRASM64Target()}) {
    TargetRegistry::RegisterMCRegInfo(*T, createRASMMCRegisterInfo);
    TargetRegistry::RegisterMCInstrInfo(*T, createRASMMCInstrInfo);
    TargetRegistry::RegisterMCSubtargetInfo(*T, createRASMMCSubtargetInfo);
    TargetRegistry::RegisterMCAsmInfo(*T, createRASMMCAsmInfo);
    TargetRegistry::RegisterMCCodeEmitter(*T, createRASMMCCodeEmitter);
    TargetRegistry::RegisterMCAsmBackend(*T, createRASMAsmBackend);
    TargetRegistry::RegisterMCInstPrinter(*T, createRASMMCInstPrinter);
    TargetRegistry::RegisterMCInstrAnalysis(*T, createRASMInstrAnalysis);
    TargetRegistry::RegisterELFStreamer(*T, createRASMELFStreamer);
    TargetRegistry::RegisterObjectTargetStreamer(
        *T, createRASMObjectTargetStreamer);
  }
}
