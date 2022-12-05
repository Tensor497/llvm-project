//===- RASMInstPrinter.cpp - Convert RASM MCInst to asm syntax --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class prints an RASM MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "RASMInstPrinter.h"
#include "RASMBaseInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
using namespace llvm;

#define DEBUG_TYPE "loongarch-asm-printer"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "RASMGenAsmWriter.inc"

void RASMInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                     StringRef Annot,
                                     const MCSubtargetInfo &STI,
                                     raw_ostream &O) {
  if (!printAliasInstr(MI, Address, STI, O))
    printInstruction(MI, Address, STI, O);
  printAnnotation(O, Annot);
}

void RASMInstPrinter::printRegName(raw_ostream &O, unsigned RegNo) const {
  O << '$' << getRegisterName(RegNo);
}

void RASMInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                        const MCSubtargetInfo &STI,
                                        raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNo);

  if (MO.isReg()) {
    printRegName(O, MO.getReg());
    return;
  }

  if (MO.isImm()) {
    O << MO.getImm();
    return;
  }

  assert(MO.isExpr() && "Unknown operand kind in printOperand");
  MO.getExpr()->print(O, &MAI);
}

const char *RASMInstPrinter::getRegisterName(unsigned RegNo) {
  // Default print reg alias name
  return getRegisterName(RegNo, RASM::RegAliasName);
}
