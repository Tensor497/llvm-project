//===-- RASMTargetStreamer.cpp - RASM Target Streamer Methods ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides RASM specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "RASMTargetStreamer.h"

using namespace llvm;

RASMTargetStreamer::RASMTargetStreamer(MCStreamer &S)
    : MCTargetStreamer(S) {}

void RASMTargetStreamer::setTargetABI(RASMABI::ABI ABI) {
  assert(ABI != RASMABI::ABI_Unknown &&
         "Improperly initialized target ABI");
  TargetABI = ABI;
}
