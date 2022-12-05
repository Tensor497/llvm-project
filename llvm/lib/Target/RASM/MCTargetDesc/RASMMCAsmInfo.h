//===-- RASMMCAsmInfo.h - RASM Asm Info --------------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the RASMMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCASMINFO_H
#define LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class RASMMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit RASMMCAsmInfo(const Triple &TargetTriple);
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_RASM_MCTARGETDESC_RASMMCASMINFO_H
