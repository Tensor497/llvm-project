//===-- RASM.h - Top-level interface for RASM ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// RASM back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RASM_RASM_H
#define LLVM_LIB_TARGET_RASM_RASM_H

#include "MCTargetDesc/RASMBaseInfo.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class RASMTargetMachine;
class AsmPrinter;
class FunctionPass;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class PassRegistry;

bool lowerRASMMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                        AsmPrinter &AP);
bool lowerRASMMachineOperandToMCOperand(const MachineOperand &MO,
                                             MCOperand &MCOp,
                                             const AsmPrinter &AP);

FunctionPass *createRASMISelDag(RASMTargetMachine &TM);
FunctionPass *createRASMExpandAtomicPseudoPass();
void initializeRASMExpandAtomicPseudoPass(PassRegistry &);

FunctionPass *createRASMPreRAExpandPseudoPass();
void initializeRASMPreRAExpandPseudoPass(PassRegistry &);
} // end namespace llvm

#endif // LLVM_LIB_TARGET_RASM_RASM_H
