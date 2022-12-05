//===-- RASMTargetInfo.cpp - RASM Target Implementation ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/RASMTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheRASM32Target() {
  static Target TheRASM32Target;
  return TheRASM32Target;
}

Target &llvm::getTheRASM64Target() {
  static Target TheRASM64Target;
  return TheRASM64Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRASMTargetInfo() {
  RegisterTarget<Triple::loongarch32, /*HasJIT=*/false> X(
      getTheRASM32Target(), "loongarch32", "32-bit RASM",
      "RASM");
  RegisterTarget<Triple::loongarch64, /*HasJIT=*/false> Y(
      getTheRASM64Target(), "loongarch64", "64-bit RASM",
      "RASM");
}
