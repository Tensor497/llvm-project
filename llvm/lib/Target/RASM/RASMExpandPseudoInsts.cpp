//===-- RASMExpandPseudoInsts.cpp - Expand pseudo instructions -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions.
//
//===----------------------------------------------------------------------===//

#include "RASM.h"
#include "RASMInstrInfo.h"
#include "RASMTargetMachine.h"
#include "MCTargetDesc/RASMBaseInfo.h"
#include "MCTargetDesc/RASMMCTargetDesc.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/CodeGen.h"

using namespace llvm;

#define RASM_PRERA_EXPAND_PSEUDO_NAME                                     \
  "RASM Pre-RA pseudo instruction expansion pass"

namespace {

class RASMPreRAExpandPseudo : public MachineFunctionPass {
public:
  const RASMInstrInfo *TII;
  static char ID;

  RASMPreRAExpandPseudo() : MachineFunctionPass(ID) {
    initializeRASMPreRAExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  StringRef getPassName() const override {
    return RASM_PRERA_EXPAND_PSEUDO_NAME;
  }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandPcalau12iInstPair(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI,
                               MachineBasicBlock::iterator &NextMBBI,
                               unsigned FlagsHi, unsigned SecondOpcode,
                               unsigned FlagsLo);
  bool expandLoadAddressPcrel(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddressGot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddressTLSLE(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddressTLSIE(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddressTLSLD(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddressTLSGD(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandFunctionCALL(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MBBI,
                          MachineBasicBlock::iterator &NextMBBI,
                          bool IsTailCall);
};

char RASMPreRAExpandPseudo::ID = 0;

bool RASMPreRAExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  TII =
      static_cast<const RASMInstrInfo *>(MF.getSubtarget().getInstrInfo());
  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);
  return Modified;
}

bool RASMPreRAExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RASMPreRAExpandPseudo::expandMI(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  switch (MBBI->getOpcode()) {
  case RASM::PseudoLA_PCREL:
    return expandLoadAddressPcrel(MBB, MBBI, NextMBBI);
  case RASM::PseudoLA_GOT:
    return expandLoadAddressGot(MBB, MBBI, NextMBBI);
  case RASM::PseudoLA_TLS_LE:
    return expandLoadAddressTLSLE(MBB, MBBI, NextMBBI);
  case RASM::PseudoLA_TLS_IE:
    return expandLoadAddressTLSIE(MBB, MBBI, NextMBBI);
  case RASM::PseudoLA_TLS_LD:
    return expandLoadAddressTLSLD(MBB, MBBI, NextMBBI);
  case RASM::PseudoLA_TLS_GD:
    return expandLoadAddressTLSGD(MBB, MBBI, NextMBBI);
  case RASM::PseudoCALL:
    return expandFunctionCALL(MBB, MBBI, NextMBBI, /*IsTailCall=*/false);
  case RASM::PseudoTAIL:
    return expandFunctionCALL(MBB, MBBI, NextMBBI, /*IsTailCall=*/true);
  }
  return false;
}

bool RASMPreRAExpandPseudo::expandPcalau12iInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode, unsigned FlagsLo) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RASM::GPRRegClass);
  MachineOperand &Symbol = MI.getOperand(1);

  BuildMI(MBB, MBBI, DL, TII->get(RASM::PCALAU12I), ScratchReg)
      .addDisp(Symbol, 0, FlagsHi);

  MachineInstr *SecondMI =
      BuildMI(MBB, MBBI, DL, TII->get(SecondOpcode), DestReg)
          .addReg(ScratchReg)
          .addDisp(Symbol, 0, FlagsLo);

  if (MI.hasOneMemOperand())
    SecondMI->addMemOperand(*MF, *MI.memoperands_begin());

  MI.eraseFromParent();
  return true;
}

bool RASMPreRAExpandPseudo::expandLoadAddressPcrel(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // pcalau12i $rd, %pc_hi20(sym)
  // addi.w/d $rd, $rd, %pc_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  const auto &STI = MF->getSubtarget<RASMSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RASM::ADDI_D : RASM::ADDI_W;
  return expandPcalau12iInstPair(MBB, MBBI, NextMBBI, RASMII::MO_PCREL_HI,
                                 SecondOpcode, RASMII::MO_PCREL_LO);
}

bool RASMPreRAExpandPseudo::expandLoadAddressGot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // pcalau12i $rd, %got_pc_hi20(sym)
  // ld.w/d $rd, $rd, %got_pc_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  const auto &STI = MF->getSubtarget<RASMSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RASM::LD_D : RASM::LD_W;
  return expandPcalau12iInstPair(MBB, MBBI, NextMBBI, RASMII::MO_GOT_PC_HI,
                                 SecondOpcode, RASMII::MO_GOT_PC_LO);
}

bool RASMPreRAExpandPseudo::expandLoadAddressTLSLE(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // lu12i.w $rd, %le_hi20(sym)
  // ori $rd, $rd, %le_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RASM::GPRRegClass);
  MachineOperand &Symbol = MI.getOperand(1);

  BuildMI(MBB, MBBI, DL, TII->get(RASM::LU12I_W), ScratchReg)
      .addDisp(Symbol, 0, RASMII::MO_LE_HI);

  BuildMI(MBB, MBBI, DL, TII->get(RASM::ORI), DestReg)
      .addReg(ScratchReg)
      .addDisp(Symbol, 0, RASMII::MO_LE_LO);

  MI.eraseFromParent();
  return true;
}

bool RASMPreRAExpandPseudo::expandLoadAddressTLSIE(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // pcalau12i $rd, %ie_pc_hi20(sym)
  // ld.w/d $rd, $rd, %ie_pc_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  const auto &STI = MF->getSubtarget<RASMSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RASM::LD_D : RASM::LD_W;
  return expandPcalau12iInstPair(MBB, MBBI, NextMBBI, RASMII::MO_IE_PC_HI,
                                 SecondOpcode, RASMII::MO_IE_PC_LO);
}

bool RASMPreRAExpandPseudo::expandLoadAddressTLSLD(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // pcalau12i $rd, %ld_pc_hi20(sym)
  // addi.w/d $rd, $rd, %got_pc_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  const auto &STI = MF->getSubtarget<RASMSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RASM::ADDI_D : RASM::ADDI_W;
  return expandPcalau12iInstPair(MBB, MBBI, NextMBBI, RASMII::MO_LD_PC_HI,
                                 SecondOpcode, RASMII::MO_GOT_PC_LO);
}

bool RASMPreRAExpandPseudo::expandLoadAddressTLSGD(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Code Sequence:
  // pcalau12i $rd, %gd_pc_hi20(sym)
  // addi.w/d $rd, $rd, %got_pc_lo12(sym)
  MachineFunction *MF = MBB.getParent();
  const auto &STI = MF->getSubtarget<RASMSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RASM::ADDI_D : RASM::ADDI_W;
  return expandPcalau12iInstPair(MBB, MBBI, NextMBBI, RASMII::MO_GD_PC_HI,
                                 SecondOpcode, RASMII::MO_GOT_PC_LO);
}

bool RASMPreRAExpandPseudo::expandFunctionCALL(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, bool IsTailCall) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  const MachineOperand &Func = MI.getOperand(0);
  MachineInstrBuilder CALL;
  unsigned Opcode;

  switch (MF->getTarget().getCodeModel()) {
  default:
    report_fatal_error("Unsupported code model");
    break;
  case CodeModel::Small: {
    // CALL:
    // bl func
    // TAIL:
    // b func
    Opcode = IsTailCall ? RASM::PseudoB_TAIL : RASM::BL;
    CALL = BuildMI(MBB, MBBI, DL, TII->get(Opcode)).add(Func);
    break;
  }
  case CodeModel::Medium: {
    // CALL:
    // pcalau12i  $ra, %pc_hi20(func)
    // jirl       $ra, $ra, %pc_lo12(func)
    // TAIL:
    // pcalau12i  $scratch, %pc_hi20(func)
    // jirl       $r0, $scratch, %pc_lo12(func)
    Opcode =
        IsTailCall ? RASM::PseudoJIRL_TAIL : RASM::PseudoJIRL_CALL;
    Register ScratchReg =
        IsTailCall
            ? MF->getRegInfo().createVirtualRegister(&RASM::GPRRegClass)
            : RASM::R1;
    MachineInstrBuilder MIB =
        BuildMI(MBB, MBBI, DL, TII->get(RASM::PCALAU12I), ScratchReg);
    CALL = BuildMI(MBB, MBBI, DL, TII->get(Opcode)).addReg(ScratchReg);
    if (Func.isSymbol()) {
      const char *FnName = Func.getSymbolName();
      MIB.addExternalSymbol(FnName, RASMII::MO_PCREL_HI);
      CALL.addExternalSymbol(FnName, RASMII::MO_PCREL_LO);
      break;
    }
    assert(Func.isGlobal() && "Expected a GlobalValue at this time");
    const GlobalValue *GV = Func.getGlobal();
    MIB.addGlobalAddress(GV, 0, RASMII::MO_PCREL_HI);
    CALL.addGlobalAddress(GV, 0, RASMII::MO_PCREL_LO);
    break;
  }
  }

  // Transfer implicit operands.
  CALL.copyImplicitOps(MI);

  // Transfer MI flags.
  CALL.setMIFlags(MI.getFlags());

  MI.eraseFromParent();
  return true;
}

} // end namespace

INITIALIZE_PASS(RASMPreRAExpandPseudo, "loongarch-prera-expand-pseudo",
                RASM_PRERA_EXPAND_PSEUDO_NAME, false, false)

namespace llvm {

FunctionPass *createRASMPreRAExpandPseudoPass() {
  return new RASMPreRAExpandPseudo();
}

} // end namespace llvm
