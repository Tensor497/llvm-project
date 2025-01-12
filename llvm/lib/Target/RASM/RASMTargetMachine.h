//=- RASMTargetMachine.h - Define TargetMachine for RASM -*- C++ -*-//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the RASM specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RASM_RASMTARGETMACHINE_H
#define LLVM_LIB_TARGET_RASM_RASMTARGETMACHINE_H

#include "RASMSubtarget.h"
#include "llvm/Target/TargetMachine.h"
#include <optional>

namespace llvm {

class RASMTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  mutable StringMap<std::unique_ptr<RASMSubtarget>> SubtargetMap;

public:
  RASMTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                         StringRef FS, const TargetOptions &Options,
                         std::optional<Reloc::Model> RM,
                         std::optional<CodeModel::Model> CM,
                         CodeGenOpt::Level OL, bool JIT);
  ~RASMTargetMachine() override;

  const RASMSubtarget *getSubtargetImpl(const Function &F) const override;
  const RASMSubtarget *getSubtargetImpl() const = delete;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_RASM_RASMTARGETMACHINE_H
