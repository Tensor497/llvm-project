//===- RASMMCTargetDesc.h - RASM Target Descriptions --*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCTARGETDESC_H
#define LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCTARGETDESC_H

#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createRASMMCCodeEmitter(const MCInstrInfo &MCII,
                                            MCContext &Ctx);

MCAsmBackend *createRASMAsmBackend(const Target &T,
                                        const MCSubtargetInfo &STI,
                                        const MCRegisterInfo &MRI,
                                        const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter>
createRASMELFObjectWriter(uint8_t OSABI, bool Is64Bit);

} // end namespace llvm

// Defines symbolic names for RASM registers.
#define GET_REGINFO_ENUM
#include "RASMGenRegisterInfo.inc"

// Defines symbolic names for RASM instructions.
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_MC_HELPER_DECLS
#include "RASMGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "RASMGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCTARGETDESC_H
