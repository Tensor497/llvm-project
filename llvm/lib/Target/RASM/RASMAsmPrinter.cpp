//===- RASMAsmPrinter.cpp - RASM LLVM Assembly Printer -*- C++ -*--=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format RASM assembly language.
//
//===----------------------------------------------------------------------===//

#include "RASMAsmPrinter.h"
#include "RASM.h"
#include "RASMTargetMachine.h"
#include "MCTargetDesc/RASMInstPrinter.h"
#include "TargetInfo/RASMTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "rasm-asm-printer"

// Simple pseudo-instructions have their lowering (with expansion to real
// instructions) auto-generated.
#include "RASMGenMCPseudoLowering.inc"

void RASMAsmPrinter::emitInstruction(const MachineInstr *MI) {
  RASM_MC::verifyInstructionPredicates(
      MI->getOpcode(), getSubtargetInfo().getFeatureBits());

  // Do any auto-generated pseudo lowerings.
  if (emitPseudoExpansionLowering(*OutStreamer, MI))
    return;

  MCInst TmpInst;
  if (!lowerRASMMachineInstrToMCInst(MI, TmpInst, *this))
    EmitToStreamer(*OutStreamer, TmpInst);
}

bool RASMAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                          const char *ExtraCode,
                                          raw_ostream &OS) {
  // First try the generic code, which knows about modifiers like 'c' and 'n'.
  if (!AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, OS))
    return false;

  const MachineOperand &MO = MI->getOperand(OpNo);
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0)
      return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default:
      return true; // Unknown modifier.
    case 'z':      // Print $zero register if zero, regular printing otherwise.
      if (MO.isImm() && MO.getImm() == 0) {
        OS << '$' << RASMInstPrinter::getRegisterName(RASM::R0);
        return false;
      }
      break;
      // TODO: handle other extra codes if any.
    }
  }

  switch (MO.getType()) {
  case MachineOperand::MO_Immediate:
    OS << MO.getImm();
    return false;
  case MachineOperand::MO_Register:
    OS << '$' << RASMInstPrinter::getRegisterName(MO.getReg());
    return false;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, OS);
    return false;
  default:
    llvm_unreachable("not implemented");
  }

  return true;
}

bool RASMAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                                unsigned OpNo,
                                                const char *ExtraCode,
                                                raw_ostream &OS) {
  // TODO: handle extra code.
  if (ExtraCode)
    return true;

  const MachineOperand &BaseMO = MI->getOperand(OpNo);
  // Base address must be a register.
  if (!BaseMO.isReg())
    return true;
  // Print the base address register.
  OS << "$" << RASMInstPrinter::getRegisterName(BaseMO.getReg());
  // Print the offset register or immediate if has.
  if (OpNo + 1 < MI->getNumOperands()) {
    const MachineOperand &OffsetMO = MI->getOperand(OpNo + 1);
    if (OffsetMO.isReg())
      OS << ", $" << RASMInstPrinter::getRegisterName(OffsetMO.getReg());
    else if (OffsetMO.isImm())
      OS << ", " << OffsetMO.getImm();
    else
      return true;
  }
  return false;
}

bool RASMAsmPrinter::runOnMachineFunction(MachineFunction &MF) {
  AsmPrinter::runOnMachineFunction(MF);
  return true;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRASMAsmPrinter() {
  RegisterAsmPrinter<RASMAsmPrinter> X(getTheRASM32Target());
  RegisterAsmPrinter<RASMAsmPrinter> Y(getTheRASM64Target());
}
