//===- RASMRegisterInfo.cpp - RASM Register Information -*- C++ -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the RASM implementation of the TargetRegisterInfo
// class.
//
//===----------------------------------------------------------------------===//

#include "RASMRegisterInfo.h"
#include "RASM.h"
#include "RASMInstrInfo.h"
#include "RASMSubtarget.h"
#include "MCTargetDesc/RASMMCTargetDesc.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "RASMGenRegisterInfo.inc"

RASMRegisterInfo::RASMRegisterInfo(unsigned HwMode)
    : RASMGenRegisterInfo(RASM::R1, /*DwarfFlavour*/ 0,
                               /*EHFlavor*/ 0,
                               /*PC*/ 0, HwMode) {}

const MCPhysReg *
RASMRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  auto &Subtarget = MF->getSubtarget<RASMSubtarget>();

  switch (Subtarget.getTargetABI()) {
  default:
    llvm_unreachable("Unrecognized ABI");
  case RASMABI::ABI_ILP32S:
  case RASMABI::ABI_LP64S:
    return CSR_ILP32S_LP64S_SaveList;
  case RASMABI::ABI_ILP32F:
  case RASMABI::ABI_LP64F:
    return CSR_ILP32F_LP64F_SaveList;
  case RASMABI::ABI_ILP32D:
  case RASMABI::ABI_LP64D:
    return CSR_ILP32D_LP64D_SaveList;
  }
}

const uint32_t *
RASMRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                            CallingConv::ID CC) const {
  auto &Subtarget = MF.getSubtarget<RASMSubtarget>();

  switch (Subtarget.getTargetABI()) {
  default:
    llvm_unreachable("Unrecognized ABI");
  case RASMABI::ABI_ILP32S:
  case RASMABI::ABI_LP64S:
    return CSR_ILP32S_LP64S_RegMask;
  case RASMABI::ABI_ILP32F:
  case RASMABI::ABI_LP64F:
    return CSR_ILP32F_LP64F_RegMask;
  case RASMABI::ABI_ILP32D:
  case RASMABI::ABI_LP64D:
    return CSR_ILP32D_LP64D_RegMask;
  }
}

const uint32_t *RASMRegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}

BitVector
RASMRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  const RASMFrameLowering *TFI = getFrameLowering(MF);
  BitVector Reserved(getNumRegs());

  // Use markSuperRegs to ensure any register aliases are also reserved
  markSuperRegs(Reserved, RASM::R0);  // zero
  markSuperRegs(Reserved, RASM::R2);  // tp
  markSuperRegs(Reserved, RASM::R3);  // sp
  markSuperRegs(Reserved, RASM::R21); // non-allocatable
  if (TFI->hasFP(MF))
    markSuperRegs(Reserved, RASM::R22); // fp
  // Reserve the base register if we need to realign the stack and allocate
  // variable-sized objects at runtime.
  if (TFI->hasBP(MF))
    markSuperRegs(Reserved, RASMABI::getBPReg()); // bp

  // FIXME: To avoid generating COPY instructions between CFRs, only use $fcc0.
  // This is required to work around the fact that COPY instruction between CFRs
  // is not provided in RASM.
  if (MF.getSubtarget<RASMSubtarget>().hasBasicF())
    for (size_t Reg = RASM::FCC1; Reg <= RASM::FCC7; ++Reg)
      markSuperRegs(Reserved, Reg);

  assert(checkAllSuperRegsMarked(Reserved));
  return Reserved;
}

Register
RASMRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? RASM::R22 : RASM::R3;
}

bool RASMRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                                int SPAdj,
                                                unsigned FIOperandNum,
                                                RegScavenger *RS) const {
  // TODO: this implementation is a temporary placeholder which does just
  // enough to allow other aspects of code generation to be tested.

  assert(SPAdj == 0 && "Unexpected non-zero SPAdj value");

  MachineInstr &MI = *II;
  assert(MI.getOperand(FIOperandNum + 1).isImm() &&
         "Unexpected FI-consuming insn");

  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineRegisterInfo &MRI = MF.getRegInfo();
  const RASMSubtarget &STI = MF.getSubtarget<RASMSubtarget>();
  const RASMInstrInfo *TII = STI.getInstrInfo();
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  DebugLoc DL = MI.getDebugLoc();
  bool IsLA64 = STI.is64Bit();
  unsigned MIOpc = MI.getOpcode();

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  StackOffset Offset =
      TFI->getFrameIndexReference(MF, FrameIndex, FrameReg) +
      StackOffset::getFixed(MI.getOperand(FIOperandNum + 1).getImm());

  bool FrameRegIsKill = false;

  if (!isInt<12>(Offset.getFixed())) {
    unsigned Addi = IsLA64 ? RASM::ADDI_D : RASM::ADDI_W;
    unsigned Add = IsLA64 ? RASM::ADD_D : RASM::ADD_W;

    // The offset won't fit in an immediate, so use a scratch register instead.
    // Modify Offset and FrameReg appropriately.
    Register ScratchReg = MRI.createVirtualRegister(&RASM::GPRRegClass);
    TII->movImm(MBB, II, DL, ScratchReg, Offset.getFixed());
    if (MIOpc == Addi) {
      BuildMI(MBB, II, DL, TII->get(Add), MI.getOperand(0).getReg())
          .addReg(FrameReg)
          .addReg(ScratchReg, RegState::Kill);
      MI.eraseFromParent();
      return true;
    }
    BuildMI(MBB, II, DL, TII->get(Add), ScratchReg)
        .addReg(FrameReg)
        .addReg(ScratchReg, RegState::Kill);
    Offset = StackOffset::getFixed(0);
    FrameReg = ScratchReg;
    FrameRegIsKill = true;
  }

  // Spill CFRs.
  if (MIOpc == RASM::PseudoST_CFR) {
    Register ScratchReg = MRI.createVirtualRegister(&RASM::GPRRegClass);
    BuildMI(MBB, II, DL, TII->get(RASM::MOVCF2GR), ScratchReg)
        .add(MI.getOperand(0));
    BuildMI(MBB, II, DL, TII->get(IsLA64 ? RASM::ST_D : RASM::ST_W))
        .addReg(ScratchReg, RegState::Kill)
        .addReg(FrameReg)
        .addImm(Offset.getFixed());
    MI.eraseFromParent();
    return true;
  }

  // Reload CFRs.
  if (MIOpc == RASM::PseudoLD_CFR) {
    Register ScratchReg = MRI.createVirtualRegister(&RASM::GPRRegClass);
    BuildMI(MBB, II, DL, TII->get(IsLA64 ? RASM::LD_D : RASM::LD_W),
            ScratchReg)
        .addReg(FrameReg)
        .addImm(Offset.getFixed());
    BuildMI(MBB, II, DL, TII->get(RASM::MOVGR2CF))
        .add(MI.getOperand(0))
        .addReg(ScratchReg, RegState::Kill);
    MI.eraseFromParent();
    return true;
  }

  MI.getOperand(FIOperandNum)
      .ChangeToRegister(FrameReg, false, false, FrameRegIsKill);
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset.getFixed());
  return false;
}
