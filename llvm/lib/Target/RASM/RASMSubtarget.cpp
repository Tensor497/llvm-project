//===-- RASMSubtarget.cpp - RASM Subtarget Information -*- C++ -*--=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the RASM specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "RASMSubtarget.h"
#include "RASMFrameLowering.h"

using namespace llvm;

#define DEBUG_TYPE "loongarch-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "RASMGenSubtargetInfo.inc"

void RASMSubtarget::anchor() {}

RASMSubtarget &RASMSubtarget::initializeSubtargetDependencies(
    const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS,
    StringRef ABIName) {
  bool Is64Bit = TT.isArch64Bit();
  if (CPU.empty() || CPU == "generic")
    CPU = Is64Bit ? "generic-la64" : "generic-la32";

  if (TuneCPU.empty())
    TuneCPU = CPU;

  ParseSubtargetFeatures(CPU, TuneCPU, FS);
  if (Is64Bit) {
    GRLenVT = MVT::i64;
    GRLen = 64;
  }

  // TODO: ILP32{S,F} LP64{S,F}
  TargetABI = Is64Bit ? RASMABI::ABI_LP64D : RASMABI::ABI_ILP32D;
  return *this;
}

RASMSubtarget::RASMSubtarget(const Triple &TT, StringRef CPU,
                                       StringRef TuneCPU, StringRef FS,
                                       StringRef ABIName,
                                       const TargetMachine &TM)
    : RASMGenSubtargetInfo(TT, CPU, TuneCPU, FS),
      FrameLowering(
          initializeSubtargetDependencies(TT, CPU, TuneCPU, FS, ABIName)),
      InstrInfo(*this), RegInfo(getHwMode()), TLInfo(TM, *this) {}
