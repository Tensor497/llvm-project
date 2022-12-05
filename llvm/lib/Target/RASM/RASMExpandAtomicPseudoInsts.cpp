//==- RASMExpandAtomicPseudoInsts.cpp - Expand atomic pseudo instrs. -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands atomic pseudo instructions into
// target instructions. This pass should be run at the last possible moment,
// avoiding the possibility for other passes to break the requirements for
// forward progress in the LL/SC block.
//
//===----------------------------------------------------------------------===//

#include "RASM.h"
#include "RASMInstrInfo.h"
#include "RASMTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define RASM_EXPAND_ATOMIC_PSEUDO_NAME                                    \
  "RASM atomic pseudo instruction expansion pass"

namespace {

class RASMExpandAtomicPseudo : public MachineFunctionPass {
public:
  const RASMInstrInfo *TII;
  static char ID;

  RASMExpandAtomicPseudo() : MachineFunctionPass(ID) {
    initializeRASMExpandAtomicPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return RASM_EXPAND_ATOMIC_PSEUDO_NAME;
  }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAtomicBinOp(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI, AtomicRMWInst::BinOp,
                         bool IsMasked, int Width,
                         MachineBasicBlock::iterator &NextMBBI);
  bool expandAtomicMinMaxOp(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            AtomicRMWInst::BinOp, bool IsMasked, int Width,
                            MachineBasicBlock::iterator &NextMBBI);
  bool expandAtomicCmpXchg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI, bool IsMasked,
                           int Width, MachineBasicBlock::iterator &NextMBBI);
};

char RASMExpandAtomicPseudo::ID = 0;

bool RASMExpandAtomicPseudo::runOnMachineFunction(MachineFunction &MF) {
  TII =
      static_cast<const RASMInstrInfo *>(MF.getSubtarget().getInstrInfo());
  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);
  return Modified;
}

bool RASMExpandAtomicPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RASMExpandAtomicPseudo::expandMI(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  switch (MBBI->getOpcode()) {
  case RASM::PseudoMaskedAtomicSwap32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Xchg, true, 32,
                             NextMBBI);
  case RASM::PseudoAtomicSwap32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Xchg, false, 32,
                             NextMBBI);
  case RASM::PseudoMaskedAtomicLoadAdd32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Add, true, 32, NextMBBI);
  case RASM::PseudoMaskedAtomicLoadSub32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Sub, true, 32, NextMBBI);
  case RASM::PseudoAtomicLoadNand32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Nand, false, 32,
                             NextMBBI);
  case RASM::PseudoAtomicLoadNand64:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Nand, false, 64,
                             NextMBBI);
  case RASM::PseudoMaskedAtomicLoadNand32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Nand, true, 32,
                             NextMBBI);
  case RASM::PseudoAtomicLoadAdd32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Add, false, 32,
                             NextMBBI);
  case RASM::PseudoAtomicLoadSub32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Sub, false, 32,
                             NextMBBI);
  case RASM::PseudoAtomicLoadAnd32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::And, false, 32,
                             NextMBBI);
  case RASM::PseudoAtomicLoadOr32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Or, false, 32, NextMBBI);
  case RASM::PseudoAtomicLoadXor32:
    return expandAtomicBinOp(MBB, MBBI, AtomicRMWInst::Xor, false, 32,
                             NextMBBI);
  case RASM::PseudoMaskedAtomicLoadUMax32:
    return expandAtomicMinMaxOp(MBB, MBBI, AtomicRMWInst::UMax, true, 32,
                                NextMBBI);
  case RASM::PseudoMaskedAtomicLoadUMin32:
    return expandAtomicMinMaxOp(MBB, MBBI, AtomicRMWInst::UMin, true, 32,
                                NextMBBI);
  case RASM::PseudoCmpXchg32:
    return expandAtomicCmpXchg(MBB, MBBI, false, 32, NextMBBI);
  case RASM::PseudoCmpXchg64:
    return expandAtomicCmpXchg(MBB, MBBI, false, 64, NextMBBI);
  case RASM::PseudoMaskedCmpXchg32:
    return expandAtomicCmpXchg(MBB, MBBI, true, 32, NextMBBI);
  case RASM::PseudoMaskedAtomicLoadMax32:
    return expandAtomicMinMaxOp(MBB, MBBI, AtomicRMWInst::Max, true, 32,
                                NextMBBI);
  case RASM::PseudoMaskedAtomicLoadMin32:
    return expandAtomicMinMaxOp(MBB, MBBI, AtomicRMWInst::Min, true, 32,
                                NextMBBI);
  }
  return false;
}

static void doAtomicBinOpExpansion(const RASMInstrInfo *TII,
                                   MachineInstr &MI, DebugLoc DL,
                                   MachineBasicBlock *ThisMBB,
                                   MachineBasicBlock *LoopMBB,
                                   MachineBasicBlock *DoneMBB,
                                   AtomicRMWInst::BinOp BinOp, int Width) {
  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg = MI.getOperand(1).getReg();
  Register AddrReg = MI.getOperand(2).getReg();
  Register IncrReg = MI.getOperand(3).getReg();
  AtomicOrdering Ordering =
      static_cast<AtomicOrdering>(MI.getOperand(4).getImm());

  // .loop:
  //   if(Ordering != AtomicOrdering::Monotonic)
  //     dbar 0
  //   ll.[w|d] dest, (addr)
  //   binop scratch, dest, val
  //   sc.[w|d] scratch, scratch, (addr)
  //   beqz scratch, loop
  if (Ordering != AtomicOrdering::Monotonic)
    BuildMI(LoopMBB, DL, TII->get(RASM::DBAR)).addImm(0);
  BuildMI(LoopMBB, DL,
          TII->get(Width == 32 ? RASM::LL_W : RASM::LL_D), DestReg)
      .addReg(AddrReg)
      .addImm(0);
  switch (BinOp) {
  default:
    llvm_unreachable("Unexpected AtomicRMW BinOp");
  case AtomicRMWInst::Xchg:
    BuildMI(LoopMBB, DL, TII->get(RASM::OR), ScratchReg)
        .addReg(IncrReg)
        .addReg(RASM::R0);
    break;
  case AtomicRMWInst::Nand:
    BuildMI(LoopMBB, DL, TII->get(RASM::AND), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    BuildMI(LoopMBB, DL, TII->get(RASM::NOR), ScratchReg)
        .addReg(ScratchReg)
        .addReg(RASM::R0);
    break;
  case AtomicRMWInst::Add:
    BuildMI(LoopMBB, DL, TII->get(RASM::ADD_W), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::Sub:
    BuildMI(LoopMBB, DL, TII->get(RASM::SUB_W), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::And:
    BuildMI(LoopMBB, DL, TII->get(RASM::AND), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::Or:
    BuildMI(LoopMBB, DL, TII->get(RASM::OR), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::Xor:
    BuildMI(LoopMBB, DL, TII->get(RASM::XOR), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  }
  BuildMI(LoopMBB, DL,
          TII->get(Width == 32 ? RASM::SC_W : RASM::SC_D), ScratchReg)
      .addReg(ScratchReg)
      .addReg(AddrReg)
      .addImm(0);
  BuildMI(LoopMBB, DL, TII->get(RASM::BEQZ))
      .addReg(ScratchReg)
      .addMBB(LoopMBB);
}

static void insertMaskedMerge(const RASMInstrInfo *TII, DebugLoc DL,
                              MachineBasicBlock *MBB, Register DestReg,
                              Register OldValReg, Register NewValReg,
                              Register MaskReg, Register ScratchReg) {
  assert(OldValReg != ScratchReg && "OldValReg and ScratchReg must be unique");
  assert(OldValReg != MaskReg && "OldValReg and MaskReg must be unique");
  assert(ScratchReg != MaskReg && "ScratchReg and MaskReg must be unique");

  // res = oldval ^ ((oldval ^ newval) & masktargetdata);
  BuildMI(MBB, DL, TII->get(RASM::XOR), ScratchReg)
      .addReg(OldValReg)
      .addReg(NewValReg);
  BuildMI(MBB, DL, TII->get(RASM::AND), ScratchReg)
      .addReg(ScratchReg)
      .addReg(MaskReg);
  BuildMI(MBB, DL, TII->get(RASM::XOR), DestReg)
      .addReg(OldValReg)
      .addReg(ScratchReg);
}

static void doMaskedAtomicBinOpExpansion(
    const RASMInstrInfo *TII, MachineInstr &MI, DebugLoc DL,
    MachineBasicBlock *ThisMBB, MachineBasicBlock *LoopMBB,
    MachineBasicBlock *DoneMBB, AtomicRMWInst::BinOp BinOp, int Width) {
  assert(Width == 32 && "Should never need to expand masked 64-bit operations");
  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg = MI.getOperand(1).getReg();
  Register AddrReg = MI.getOperand(2).getReg();
  Register IncrReg = MI.getOperand(3).getReg();
  Register MaskReg = MI.getOperand(4).getReg();
  AtomicOrdering Ordering =
      static_cast<AtomicOrdering>(MI.getOperand(5).getImm());

  // .loop:
  //   if(Ordering != AtomicOrdering::Monotonic)
  //     dbar 0
  //   ll.w destreg, (alignedaddr)
  //   binop scratch, destreg, incr
  //   xor scratch, destreg, scratch
  //   and scratch, scratch, masktargetdata
  //   xor scratch, destreg, scratch
  //   sc.w scratch, scratch, (alignedaddr)
  //   beqz scratch, loop
  if (Ordering != AtomicOrdering::Monotonic)
    BuildMI(LoopMBB, DL, TII->get(RASM::DBAR)).addImm(0);
  BuildMI(LoopMBB, DL, TII->get(RASM::LL_W), DestReg)
      .addReg(AddrReg)
      .addImm(0);
  switch (BinOp) {
  default:
    llvm_unreachable("Unexpected AtomicRMW BinOp");
  case AtomicRMWInst::Xchg:
    BuildMI(LoopMBB, DL, TII->get(RASM::ADDI_W), ScratchReg)
        .addReg(IncrReg)
        .addImm(0);
    break;
  case AtomicRMWInst::Add:
    BuildMI(LoopMBB, DL, TII->get(RASM::ADD_W), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::Sub:
    BuildMI(LoopMBB, DL, TII->get(RASM::SUB_W), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    break;
  case AtomicRMWInst::Nand:
    BuildMI(LoopMBB, DL, TII->get(RASM::AND), ScratchReg)
        .addReg(DestReg)
        .addReg(IncrReg);
    BuildMI(LoopMBB, DL, TII->get(RASM::NOR), ScratchReg)
        .addReg(ScratchReg)
        .addReg(RASM::R0);
    // TODO: support other AtomicRMWInst.
  }

  insertMaskedMerge(TII, DL, LoopMBB, ScratchReg, DestReg, ScratchReg, MaskReg,
                    ScratchReg);

  BuildMI(LoopMBB, DL, TII->get(RASM::SC_W), ScratchReg)
      .addReg(ScratchReg)
      .addReg(AddrReg)
      .addImm(0);
  BuildMI(LoopMBB, DL, TII->get(RASM::BEQZ))
      .addReg(ScratchReg)
      .addMBB(LoopMBB);
}

bool RASMExpandAtomicPseudo::expandAtomicBinOp(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    AtomicRMWInst::BinOp BinOp, bool IsMasked, int Width,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  auto LoopMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto DoneMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Insert new MBBs.
  MF->insert(++MBB.getIterator(), LoopMBB);
  MF->insert(++LoopMBB->getIterator(), DoneMBB);

  // Set up successors and transfer remaining instructions to DoneMBB.
  LoopMBB->addSuccessor(LoopMBB);
  LoopMBB->addSuccessor(DoneMBB);
  DoneMBB->splice(DoneMBB->end(), &MBB, MI, MBB.end());
  DoneMBB->transferSuccessors(&MBB);
  MBB.addSuccessor(LoopMBB);

  if (IsMasked)
    doMaskedAtomicBinOpExpansion(TII, MI, DL, &MBB, LoopMBB, DoneMBB, BinOp,
                                 Width);
  else
    doAtomicBinOpExpansion(TII, MI, DL, &MBB, LoopMBB, DoneMBB, BinOp, Width);

  NextMBBI = MBB.end();
  MI.eraseFromParent();

  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *LoopMBB);
  computeAndAddLiveIns(LiveRegs, *DoneMBB);

  return true;
}

static void insertSext(const RASMInstrInfo *TII, DebugLoc DL,
                       MachineBasicBlock *MBB, Register ValReg,
                       Register ShamtReg) {
  BuildMI(MBB, DL, TII->get(RASM::SLL_W), ValReg)
      .addReg(ValReg)
      .addReg(ShamtReg);
  BuildMI(MBB, DL, TII->get(RASM::SRA_W), ValReg)
      .addReg(ValReg)
      .addReg(ShamtReg);
}

bool RASMExpandAtomicPseudo::expandAtomicMinMaxOp(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    AtomicRMWInst::BinOp BinOp, bool IsMasked, int Width,
    MachineBasicBlock::iterator &NextMBBI) {
  assert(IsMasked == true &&
         "Should only need to expand masked atomic max/min");
  assert(Width == 32 && "Should never need to expand masked 64-bit operations");

  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  MachineFunction *MF = MBB.getParent();
  auto LoopHeadMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto LoopIfBodyMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto LoopTailMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto TailMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto DoneMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Insert new MBBs.
  MF->insert(++MBB.getIterator(), LoopHeadMBB);
  MF->insert(++LoopHeadMBB->getIterator(), LoopIfBodyMBB);
  MF->insert(++LoopIfBodyMBB->getIterator(), LoopTailMBB);
  MF->insert(++LoopTailMBB->getIterator(), TailMBB);
  MF->insert(++TailMBB->getIterator(), DoneMBB);

  // Set up successors and transfer remaining instructions to DoneMBB.
  LoopHeadMBB->addSuccessor(LoopIfBodyMBB);
  LoopHeadMBB->addSuccessor(LoopTailMBB);
  LoopIfBodyMBB->addSuccessor(LoopTailMBB);
  LoopTailMBB->addSuccessor(LoopHeadMBB);
  LoopTailMBB->addSuccessor(TailMBB);
  TailMBB->addSuccessor(DoneMBB);
  DoneMBB->splice(DoneMBB->end(), &MBB, MI, MBB.end());
  DoneMBB->transferSuccessors(&MBB);
  MBB.addSuccessor(LoopHeadMBB);

  Register DestReg = MI.getOperand(0).getReg();
  Register Scratch1Reg = MI.getOperand(1).getReg();
  Register Scratch2Reg = MI.getOperand(2).getReg();
  Register AddrReg = MI.getOperand(3).getReg();
  Register IncrReg = MI.getOperand(4).getReg();
  Register MaskReg = MI.getOperand(5).getReg();

  //
  // .loophead:
  //   dbar 0
  //   ll.w destreg, (alignedaddr)
  //   and scratch2, destreg, mask
  //   move scratch1, destreg
  BuildMI(LoopHeadMBB, DL, TII->get(RASM::DBAR)).addImm(0);
  BuildMI(LoopHeadMBB, DL, TII->get(RASM::LL_W), DestReg)
      .addReg(AddrReg)
      .addImm(0);
  BuildMI(LoopHeadMBB, DL, TII->get(RASM::AND), Scratch2Reg)
      .addReg(DestReg)
      .addReg(MaskReg);
  BuildMI(LoopHeadMBB, DL, TII->get(RASM::OR), Scratch1Reg)
      .addReg(DestReg)
      .addReg(RASM::R0);

  switch (BinOp) {
  default:
    llvm_unreachable("Unexpected AtomicRMW BinOp");
  // bgeu scratch2, incr, .looptail
  case AtomicRMWInst::UMax:
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BGEU))
        .addReg(Scratch2Reg)
        .addReg(IncrReg)
        .addMBB(LoopTailMBB);
    break;
  // bgeu incr, scratch2, .looptail
  case AtomicRMWInst::UMin:
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BGEU))
        .addReg(IncrReg)
        .addReg(Scratch2Reg)
        .addMBB(LoopTailMBB);
    break;
  case AtomicRMWInst::Max:
    insertSext(TII, DL, LoopHeadMBB, Scratch2Reg, MI.getOperand(6).getReg());
    // bge scratch2, incr, .looptail
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BGE))
        .addReg(Scratch2Reg)
        .addReg(IncrReg)
        .addMBB(LoopTailMBB);
    break;
  case AtomicRMWInst::Min:
    insertSext(TII, DL, LoopHeadMBB, Scratch2Reg, MI.getOperand(6).getReg());
    // bge incr, scratch2, .looptail
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BGE))
        .addReg(IncrReg)
        .addReg(Scratch2Reg)
        .addMBB(LoopTailMBB);
    break;
    // TODO: support other AtomicRMWInst.
  }

  // .loopifbody:
  //   xor scratch1, destreg, incr
  //   and scratch1, scratch1, mask
  //   xor scratch1, destreg, scratch1
  insertMaskedMerge(TII, DL, LoopIfBodyMBB, Scratch1Reg, DestReg, IncrReg,
                    MaskReg, Scratch1Reg);

  // .looptail:
  //   sc.w scratch1, scratch1, (addr)
  //   beqz scratch1, loop
  //   dbar 0x700
  BuildMI(LoopTailMBB, DL, TII->get(RASM::SC_W), Scratch1Reg)
      .addReg(Scratch1Reg)
      .addReg(AddrReg)
      .addImm(0);
  BuildMI(LoopTailMBB, DL, TII->get(RASM::BEQZ))
      .addReg(Scratch1Reg)
      .addMBB(LoopHeadMBB);

  // .tail:
  //   dbar 0x700
  BuildMI(TailMBB, DL, TII->get(RASM::DBAR)).addImm(0x700);

  NextMBBI = MBB.end();
  MI.eraseFromParent();

  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *LoopHeadMBB);
  computeAndAddLiveIns(LiveRegs, *LoopIfBodyMBB);
  computeAndAddLiveIns(LiveRegs, *LoopTailMBB);
  computeAndAddLiveIns(LiveRegs, *TailMBB);
  computeAndAddLiveIns(LiveRegs, *DoneMBB);

  return true;
}

bool RASMExpandAtomicPseudo::expandAtomicCmpXchg(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, bool IsMasked,
    int Width, MachineBasicBlock::iterator &NextMBBI) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  MachineFunction *MF = MBB.getParent();
  auto LoopHeadMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto LoopTailMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto TailMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  auto DoneMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Insert new MBBs.
  MF->insert(++MBB.getIterator(), LoopHeadMBB);
  MF->insert(++LoopHeadMBB->getIterator(), LoopTailMBB);
  MF->insert(++LoopTailMBB->getIterator(), TailMBB);
  MF->insert(++TailMBB->getIterator(), DoneMBB);

  // Set up successors and transfer remaining instructions to DoneMBB.
  LoopHeadMBB->addSuccessor(LoopTailMBB);
  LoopHeadMBB->addSuccessor(TailMBB);
  LoopTailMBB->addSuccessor(DoneMBB);
  LoopTailMBB->addSuccessor(LoopHeadMBB);
  TailMBB->addSuccessor(DoneMBB);
  DoneMBB->splice(DoneMBB->end(), &MBB, MI, MBB.end());
  DoneMBB->transferSuccessors(&MBB);
  MBB.addSuccessor(LoopHeadMBB);

  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg = MI.getOperand(1).getReg();
  Register AddrReg = MI.getOperand(2).getReg();
  Register CmpValReg = MI.getOperand(3).getReg();
  Register NewValReg = MI.getOperand(4).getReg();

  if (!IsMasked) {
    // .loophead:
    //   ll.[w|d] dest, (addr)
    //   bne dest, cmpval, tail
    BuildMI(LoopHeadMBB, DL,
            TII->get(Width == 32 ? RASM::LL_W : RASM::LL_D), DestReg)
        .addReg(AddrReg)
        .addImm(0);
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BNE))
        .addReg(DestReg)
        .addReg(CmpValReg)
        .addMBB(TailMBB);
    // .looptail:
    //   dbar 0
    //   move scratch, newval
    //   sc.[w|d] scratch, scratch, (addr)
    //   beqz scratch, loophead
    //   b done
    BuildMI(LoopTailMBB, DL, TII->get(RASM::DBAR)).addImm(0);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::OR), ScratchReg)
        .addReg(NewValReg)
        .addReg(RASM::R0);
    BuildMI(LoopTailMBB, DL,
            TII->get(Width == 32 ? RASM::SC_W : RASM::SC_D),
            ScratchReg)
        .addReg(ScratchReg)
        .addReg(AddrReg)
        .addImm(0);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::BEQZ))
        .addReg(ScratchReg)
        .addMBB(LoopHeadMBB);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::B)).addMBB(DoneMBB);
  } else {
    // .loophead:
    //   ll.[w|d] dest, (addr)
    //   and scratch, dest, mask
    //   bne scratch, cmpval, tail
    Register MaskReg = MI.getOperand(5).getReg();
    BuildMI(LoopHeadMBB, DL,
            TII->get(Width == 32 ? RASM::LL_W : RASM::LL_D), DestReg)
        .addReg(AddrReg)
        .addImm(0);
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::AND), ScratchReg)
        .addReg(DestReg)
        .addReg(MaskReg);
    BuildMI(LoopHeadMBB, DL, TII->get(RASM::BNE))
        .addReg(ScratchReg)
        .addReg(CmpValReg)
        .addMBB(TailMBB);

    // .looptail:
    //   dbar 0
    //   andn scratch, dest, mask
    //   or scratch, scratch, newval
    //   sc.[w|d] scratch, scratch, (addr)
    //   beqz scratch, loophead
    //   b done
    BuildMI(LoopTailMBB, DL, TII->get(RASM::DBAR)).addImm(0);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::ANDN), ScratchReg)
        .addReg(DestReg)
        .addReg(MaskReg);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::OR), ScratchReg)
        .addReg(ScratchReg)
        .addReg(NewValReg);
    BuildMI(LoopTailMBB, DL,
            TII->get(Width == 32 ? RASM::SC_W : RASM::SC_D),
            ScratchReg)
        .addReg(ScratchReg)
        .addReg(AddrReg)
        .addImm(0);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::BEQZ))
        .addReg(ScratchReg)
        .addMBB(LoopHeadMBB);
    BuildMI(LoopTailMBB, DL, TII->get(RASM::B)).addMBB(DoneMBB);
  }

  // .tail:
  //   dbar 0x700
  BuildMI(TailMBB, DL, TII->get(RASM::DBAR)).addImm(0x700);

  NextMBBI = MBB.end();
  MI.eraseFromParent();

  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *LoopHeadMBB);
  computeAndAddLiveIns(LiveRegs, *LoopTailMBB);
  computeAndAddLiveIns(LiveRegs, *TailMBB);
  computeAndAddLiveIns(LiveRegs, *DoneMBB);

  return true;
}

} // end namespace

INITIALIZE_PASS(RASMExpandAtomicPseudo, "loongarch-expand-atomic-pseudo",
                RASM_EXPAND_ATOMIC_PSEUDO_NAME, false, false)

namespace llvm {

FunctionPass *createRASMExpandAtomicPseudoPass() {
  return new RASMExpandAtomicPseudo();
}

} // end namespace llvm
