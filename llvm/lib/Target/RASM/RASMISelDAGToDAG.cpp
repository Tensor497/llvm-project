//=- RASMISelDAGToDAG.cpp - A dag to dag inst selector for RASM -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the RASM target.
//
//===----------------------------------------------------------------------===//

#include "RASMISelDAGToDAG.h"
#include "RASMISelLowering.h"
#include "MCTargetDesc/RASMMCTargetDesc.h"
#include "MCTargetDesc/RASMMatInt.h"
#include "llvm/Support/KnownBits.h"

using namespace llvm;

#define DEBUG_TYPE "loongarch-isel"

void RASMDAGToDAGISel::Select(SDNode *Node) {
  // If we have a custom node, we have already selected.
  if (Node->isMachineOpcode()) {
    LLVM_DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // Instruction Selection not handled by the auto-generated tablegen selection
  // should be handled here.
  unsigned Opcode = Node->getOpcode();
  MVT GRLenVT = Subtarget->getGRLenVT();
  SDLoc DL(Node);
  MVT VT = Node->getSimpleValueType(0);

  switch (Opcode) {
  default:
    break;
  case ISD::Constant: {
    int64_t Imm = cast<ConstantSDNode>(Node)->getSExtValue();
    if (Imm == 0 && VT == GRLenVT) {
      SDValue New = CurDAG->getCopyFromReg(CurDAG->getEntryNode(), DL,
                                           RASM::R0, GRLenVT);
      ReplaceNode(Node, New.getNode());
      return;
    }
    SDNode *Result = nullptr;
    SDValue SrcReg = CurDAG->getRegister(RASM::R0, GRLenVT);
    // The instructions in the sequence are handled here.
    for (RASMMatInt::Inst &Inst : RASMMatInt::generateInstSeq(Imm)) {
      SDValue SDImm = CurDAG->getTargetConstant(Inst.Imm, DL, GRLenVT);
      if (Inst.Opc == RASM::LU12I_W)
        Result = CurDAG->getMachineNode(RASM::LU12I_W, DL, GRLenVT, SDImm);
      else
        Result = CurDAG->getMachineNode(Inst.Opc, DL, GRLenVT, SrcReg, SDImm);
      SrcReg = SDValue(Result, 0);
    }

    ReplaceNode(Node, Result);
    return;
  }
  case ISD::FrameIndex: {
    SDValue Imm = CurDAG->getTargetConstant(0, DL, GRLenVT);
    int FI = cast<FrameIndexSDNode>(Node)->getIndex();
    SDValue TFI = CurDAG->getTargetFrameIndex(FI, VT);
    unsigned ADDIOp =
        Subtarget->is64Bit() ? RASM::ADDI_D : RASM::ADDI_W;
    ReplaceNode(Node, CurDAG->getMachineNode(ADDIOp, DL, VT, TFI, Imm));
    return;
  }
    // TODO: Add selection nodes needed later.
  }

  // Select the default instruction.
  SelectCode(Node);
}

bool RASMDAGToDAGISel::SelectInlineAsmMemoryOperand(
    const SDValue &Op, unsigned ConstraintID, std::vector<SDValue> &OutOps) {
  switch (ConstraintID) {
  default:
    llvm_unreachable("unexpected asm memory constraint");
  // Reg+Reg addressing.
  case InlineAsm::Constraint_k:
    OutOps.push_back(Op.getOperand(0));
    OutOps.push_back(Op.getOperand(1));
    return false;
  // Reg+simm12 addressing.
  case InlineAsm::Constraint_m: {
    SDValue Base = Op;
    SDValue Offset =
        CurDAG->getTargetConstant(0, SDLoc(Op), Subtarget->getGRLenVT());
    if (CurDAG->isBaseWithConstantOffset(Op)) {
      ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Op.getOperand(1));
      if (isIntN(12, CN->getSExtValue())) {
        Base = Op.getOperand(0);
        Offset = CurDAG->getTargetConstant(CN->getZExtValue(), SDLoc(Op),
                                           Op.getValueType());
      }
    }
    OutOps.push_back(Base);
    OutOps.push_back(Offset);
    return false;
  }
  case InlineAsm::Constraint_ZB:
    OutOps.push_back(Op);
    // No offset.
    return false;
  // Reg+(simm14<<2) addressing.
  case InlineAsm::Constraint_ZC: {
    SDValue Base = Op;
    SDValue Offset =
        CurDAG->getTargetConstant(0, SDLoc(Op), Subtarget->getGRLenVT());
    if (CurDAG->isBaseWithConstantOffset(Op)) {
      ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Op.getOperand(1));
      if (isIntN(16, CN->getSExtValue()) &&
          isAligned(Align(4ULL), CN->getZExtValue())) {
        Base = Op.getOperand(0);
        Offset = CurDAG->getTargetConstant(CN->getZExtValue(), SDLoc(Op),
                                           Op.getValueType());
      }
    }
    OutOps.push_back(Base);
    OutOps.push_back(Offset);
    return false;
  }
  }
  return true;
}

bool RASMDAGToDAGISel::SelectBaseAddr(SDValue Addr, SDValue &Base) {
  // If this is FrameIndex, select it directly. Otherwise just let it get
  // selected to a register independently.
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr))
    Base =
        CurDAG->getTargetFrameIndex(FIN->getIndex(), Subtarget->getGRLenVT());
  else
    Base = Addr;
  return true;
}

bool RASMDAGToDAGISel::selectNonFIBaseAddr(SDValue Addr, SDValue &Base) {
  // If this is FrameIndex, don't select it.
  if (isa<FrameIndexSDNode>(Addr))
    return false;
  Base = Addr;
  return true;
}

bool RASMDAGToDAGISel::selectShiftMask(SDValue N, unsigned ShiftWidth,
                                            SDValue &ShAmt) {
  // Shift instructions on RASM only read the lower 5 or 6 bits of the
  // shift amount. If there is an AND on the shift amount, we can bypass it if
  // it doesn't affect any of those bits.
  if (N.getOpcode() == ISD::AND && isa<ConstantSDNode>(N.getOperand(1))) {
    const APInt &AndMask = N->getConstantOperandAPInt(1);

    // Since the max shift amount is a power of 2 we can subtract 1 to make a
    // mask that covers the bits needed to represent all shift amounts.
    assert(isPowerOf2_32(ShiftWidth) && "Unexpected max shift amount!");
    APInt ShMask(AndMask.getBitWidth(), ShiftWidth - 1);

    if (ShMask.isSubsetOf(AndMask)) {
      ShAmt = N.getOperand(0);
      return true;
    }

    // SimplifyDemandedBits may have optimized the mask so try restoring any
    // bits that are known zero.
    KnownBits Known = CurDAG->computeKnownBits(N->getOperand(0));
    if (ShMask.isSubsetOf(AndMask | Known.Zero)) {
      ShAmt = N.getOperand(0);
      return true;
    }
  } else if (N.getOpcode() == RASMISD::BSTRPICK) {
    // Similar to the above AND, if there is a BSTRPICK on the shift amount, we
    // can bypass it.
    assert(isPowerOf2_32(ShiftWidth) && "Unexpected max shift amount!");
    assert(isa<ConstantSDNode>(N.getOperand(1)) && "Illegal msb operand!");
    assert(isa<ConstantSDNode>(N.getOperand(2)) && "Illegal lsb operand!");
    uint64_t msb = N.getConstantOperandVal(1), lsb = N.getConstantOperandVal(2);
    if (lsb == 0 && Log2_32(ShiftWidth) <= msb + 1) {
      ShAmt = N.getOperand(0);
      return true;
    }
  } else if (N.getOpcode() == ISD::SUB &&
             isa<ConstantSDNode>(N.getOperand(0))) {
    uint64_t Imm = N.getConstantOperandVal(0);
    // If we are shifting by N-X where N == 0 mod Size, then just shift by -X to
    // generate a NEG instead of a SUB of a constant.
    if (Imm != 0 && Imm % ShiftWidth == 0) {
      SDLoc DL(N);
      EVT VT = N.getValueType();
      SDValue Zero =
          CurDAG->getCopyFromReg(CurDAG->getEntryNode(), DL, RASM::R0, VT);
      unsigned NegOpc = VT == MVT::i64 ? RASM::SUB_D : RASM::SUB_W;
      MachineSDNode *Neg =
          CurDAG->getMachineNode(NegOpc, DL, VT, Zero, N.getOperand(1));
      ShAmt = SDValue(Neg, 0);
      return true;
    }
  }

  ShAmt = N;
  return true;
}

bool RASMDAGToDAGISel::selectSExti32(SDValue N, SDValue &Val) {
  if (N.getOpcode() == ISD::SIGN_EXTEND_INREG &&
      cast<VTSDNode>(N.getOperand(1))->getVT() == MVT::i32) {
    Val = N.getOperand(0);
    return true;
  }
  if (N.getOpcode() == RASMISD::BSTRPICK &&
      N.getConstantOperandVal(1) < UINT64_C(0X1F) &&
      N.getConstantOperandVal(2) == UINT64_C(0)) {
    Val = N;
    return true;
  }
  MVT VT = N.getSimpleValueType();
  if (CurDAG->ComputeNumSignBits(N) > (VT.getSizeInBits() - 32)) {
    Val = N;
    return true;
  }

  return false;
}

bool RASMDAGToDAGISel::selectZExti32(SDValue N, SDValue &Val) {
  if (N.getOpcode() == ISD::AND) {
    auto *C = dyn_cast<ConstantSDNode>(N.getOperand(1));
    if (C && C->getZExtValue() == UINT64_C(0xFFFFFFFF)) {
      Val = N.getOperand(0);
      return true;
    }
  }
  MVT VT = N.getSimpleValueType();
  APInt Mask = APInt::getHighBitsSet(VT.getSizeInBits(), 32);
  if (CurDAG->MaskedValueIsZero(N, Mask)) {
    Val = N;
    return true;
  }

  return false;
}

// This pass converts a legalized DAG into a RASM-specific DAG, ready
// for instruction scheduling.
FunctionPass *llvm::createRASMISelDag(RASMTargetMachine &TM) {
  return new RASMDAGToDAGISel(TM);
}
