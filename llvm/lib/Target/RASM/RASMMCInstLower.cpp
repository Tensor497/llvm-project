//=- RASMMCInstLower.cpp - Convert RASM MachineInstr to an MCInst -=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower RASM MachineInstrs to their
// corresponding MCInst records.
//
//===----------------------------------------------------------------------===//

#include "RASM.h"
#include "RASMSubtarget.h"
#include "MCTargetDesc/RASMBaseInfo.h"
#include "MCTargetDesc/RASMMCExpr.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static MCOperand lowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym,
                                    const AsmPrinter &AP) {
  MCContext &Ctx = AP.OutContext;
  RASMMCExpr::VariantKind Kind;

  switch (MO.getTargetFlags()) {
  default:
    llvm_unreachable("Unknown target flag on GV operand");
  case RASMII::MO_None:
    Kind = RASMMCExpr::VK_RASM_None;
    break;
  case RASMII::MO_CALL:
    Kind = RASMMCExpr::VK_RASM_CALL;
    break;
  case RASMII::MO_CALL_PLT:
    Kind = RASMMCExpr::VK_RASM_CALL_PLT;
    break;
  case RASMII::MO_PCREL_HI:
    Kind = RASMMCExpr::VK_RASM_PCALA_HI20;
    break;
  case RASMII::MO_PCREL_LO:
    Kind = RASMMCExpr::VK_RASM_PCALA_LO12;
    break;
  case RASMII::MO_GOT_PC_HI:
    Kind = RASMMCExpr::VK_RASM_GOT_PC_HI20;
    break;
  case RASMII::MO_GOT_PC_LO:
    Kind = RASMMCExpr::VK_RASM_GOT_PC_LO12;
    break;
  case RASMII::MO_LE_HI:
    Kind = RASMMCExpr::VK_RASM_TLS_LE_HI20;
    break;
  case RASMII::MO_LE_LO:
    Kind = RASMMCExpr::VK_RASM_TLS_LE_LO12;
    break;
  case RASMII::MO_IE_PC_HI:
    Kind = RASMMCExpr::VK_RASM_TLS_IE_PC_HI20;
    break;
  case RASMII::MO_IE_PC_LO:
    Kind = RASMMCExpr::VK_RASM_TLS_IE_PC_LO12;
    break;
  case RASMII::MO_LD_PC_HI:
    Kind = RASMMCExpr::VK_RASM_TLS_LD_PC_HI20;
    break;
  case RASMII::MO_GD_PC_HI:
    Kind = RASMMCExpr::VK_RASM_TLS_GD_PC_HI20;
    break;
    // TODO: Handle more target-flags.
  }

  const MCExpr *ME =
      MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset())
    ME = MCBinaryExpr::createAdd(
        ME, MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);

  if (Kind != RASMMCExpr::VK_RASM_None)
    ME = RASMMCExpr::create(ME, Kind, Ctx);
  return MCOperand::createExpr(ME);
}

bool llvm::lowerRASMMachineOperandToMCOperand(const MachineOperand &MO,
                                                   MCOperand &MCOp,
                                                   const AsmPrinter &AP) {
  switch (MO.getType()) {
  default:
    report_fatal_error(
        "lowerRASMMachineOperandToMCOperand: unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return false;
    MCOp = MCOperand::createReg(MO.getReg());
    break;
  case MachineOperand::MO_RegisterMask:
    // Regmasks are like implicit defs.
    return false;
  case MachineOperand::MO_Immediate:
    MCOp = MCOperand::createImm(MO.getImm());
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    MCOp = lowerSymbolOperand(MO, AP.GetCPISymbol(MO.getIndex()), AP);
    break;
  case MachineOperand::MO_GlobalAddress:
    MCOp = lowerSymbolOperand(MO, AP.getSymbolPreferLocal(*MO.getGlobal()), AP);
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MCOp = lowerSymbolOperand(MO, MO.getMBB()->getSymbol(), AP);
    break;
  case MachineOperand::MO_ExternalSymbol:
    MCOp = lowerSymbolOperand(
        MO, AP.GetExternalSymbolSymbol(MO.getSymbolName()), AP);
    break;
  case MachineOperand::MO_BlockAddress:
    MCOp = lowerSymbolOperand(
        MO, AP.GetBlockAddressSymbol(MO.getBlockAddress()), AP);
    break;
  case MachineOperand::MO_JumpTableIndex:
    MCOp = lowerSymbolOperand(MO, AP.GetJTISymbol(MO.getIndex()), AP);
    break;
  }
  return true;
}

bool llvm::lowerRASMMachineInstrToMCInst(const MachineInstr *MI,
                                              MCInst &OutMI, AsmPrinter &AP) {
  OutMI.setOpcode(MI->getOpcode());

  for (const MachineOperand &MO : MI->operands()) {
    MCOperand MCOp;
    if (lowerRASMMachineOperandToMCOperand(MO, MCOp, AP))
      OutMI.addOperand(MCOp);
  }
  return false;
}
