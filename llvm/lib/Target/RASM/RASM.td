//===-- RASM.td - Describe the RASM Target -------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// RASM subtarget features and instruction predicates.
//===----------------------------------------------------------------------===//

// RASM is divided into two versions, the 32-bit version (LA32) and the
// 64-bit version (LA64).
def Feature64Bit
    : SubtargetFeature<"64bit", "HasLA64", "true",
                       "LA64 Basic Integer and Privilege Instruction Set">;
def IsLA64
    : Predicate<"Subtarget->is64Bit()">,
      AssemblerPredicate<(all_of Feature64Bit),
                         "LA64 Basic Integer and Privilege Instruction Set">;
def IsLA32
    : Predicate<"!Subtarget->is64Bit()">,
      AssemblerPredicate<(all_of(not Feature64Bit)),
                         "LA32 Basic Integer and Privilege Instruction Set">;

defvar LA32 = DefaultMode;
def LA64 : HwMode<"+64bit">;

// Single Precision floating point
def FeatureBasicF
    : SubtargetFeature<"f", "HasBasicF", "true",
                       "'F' (Single-Precision Floating-Point)">;
def HasBasicF
    : Predicate<"Subtarget->hasBasicF()">,
      AssemblerPredicate<(all_of FeatureBasicF),
                         "'F' (Single-Precision Floating-Point)">;

// Double Precision floating point
def FeatureBasicD
    : SubtargetFeature<"d", "HasBasicD", "true",
                       "'D' (Double-Precision Floating-Point)",
                       [FeatureBasicF]>;
def HasBasicD
    : Predicate<"Subtarget->hasBasicD()">,
      AssemblerPredicate<(all_of FeatureBasicD),
                         "'D' (Double-Precision Floating-Point)">;

// Loongson SIMD eXtension (LSX)
def FeatureExtLSX
    : SubtargetFeature<"lsx", "HasExtLSX", "true",
                       "'LSX' (Loongson SIMD Extension)", [FeatureBasicD]>;
def HasExtLSX
    : Predicate<"Subtarget->hasExtLSX()">,
      AssemblerPredicate<(all_of FeatureExtLSX),
                         "'LSX' (Loongson SIMD Extension)">;

// Loongson Advanced SIMD eXtension (LASX)
def FeatureExtLASX
    : SubtargetFeature<"lasx", "HasExtLASX", "true",
                       "'LASX' (Loongson Advanced SIMD Extension)",
                       [FeatureExtLSX]>;
def HasExtLASX
    : Predicate<"Subtarget->hasExtLASX()">,
      AssemblerPredicate<(all_of FeatureExtLASX),
                         "'LASX' (Loongson Advanced SIMD Extension)">;

// Loongson VirtualiZation (LVZ)
def FeatureExtLVZ
    : SubtargetFeature<"lvz", "HasExtLVZ", "true",
                       "'LVZ' (Loongson Virtualization Extension)">;
def HasExtLVZ
    : Predicate<"Subtarget->hasExtLVZ()">,
      AssemblerPredicate<(all_of FeatureExtLVZ),
                         "'LVZ' (Loongson Virtualization Extension)">;

// Loongson Binary Translation (LBT)
def FeatureExtLBT
    : SubtargetFeature<"lbt", "HasExtLBT", "true",
                       "'LBT' (Loongson Binary Translation Extension)">;
def HasExtLBT
    : Predicate<"Subtarget->hasExtLBT()">,
      AssemblerPredicate<(all_of FeatureExtLBT),
                         "'LBT' (Loongson Binary Translation Extension)">;

// Expand la.global as la.pcrel
def LaGlobalWithPcrel
    : SubtargetFeature<"la-global-with-pcrel", "HasLaGlobalWithPcrel", "true",
                       "Expand la.global as la.pcrel">;
def HasLaGlobalWithPcrel
    : Predicate<"Subtarget->hasLaGlobalWithPcrel()">,
      AssemblerPredicate<(all_of LaGlobalWithPcrel),
                         "Expand la.global as la.pcrel">;

// Expand la.global as la.abs
def LaGlobalWithAbs
    : SubtargetFeature<"la-global-with-abs", "HasLaGlobalWithAbs", "true",
                       "Expand la.global as la.abs">;
def HasLaGlobalWithAbs
    : Predicate<"Subtarget->hasLaGlobalWithAbs()">,
      AssemblerPredicate<(all_of LaGlobalWithAbs),
                         "Expand la.global as la.abs">;

// Expand la.local as la.abs
def LaLocalWithAbs
    : SubtargetFeature<"la-local-with-abs", "HasLaLocalWithAbs", "true",
                       "Expand la.local as la.abs">;
def HasLaLocalWithAbs
    : Predicate<"Subtarget->hasLaLocalWithAbs()">,
      AssemblerPredicate<(all_of LaLocalWithAbs),
                         "Expand la.local as la.abs">;

//===----------------------------------------------------------------------===//
// Registers, instruction descriptions ...
//===----------------------------------------------------------------------===//

include "RASMRegisterInfo.td"
include "RASMCallingConv.td"
include "RASMInstrInfo.td"

//===----------------------------------------------------------------------===//
// RASM processors supported.
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic-la32", NoSchedModel, []>;
def : ProcessorModel<"generic-la64", NoSchedModel, [Feature64Bit]>;

// Support generic for compatibility with other targets. The triple will be used
// to change to the appropriate la32/la64 version.
def : ProcessorModel<"generic", NoSchedModel, []>;

def : ProcessorModel<"la464", NoSchedModel, [Feature64Bit,
                                             FeatureExtLASX,
                                             FeatureExtLVZ,
                                             FeatureExtLBT]>;

//===----------------------------------------------------------------------===//
// Define the RASM target.
//===----------------------------------------------------------------------===//

def RASMInstrInfo : InstrInfo {
  // guess mayLoad, mayStore, and hasSideEffects
  // This option is a temporary migration help. It will go away.
  let guessInstructionProperties = 1;
}

def RASMAsmParser : AsmParser {
  let ShouldEmitMatchRegisterAltName = 1;
  let AllowDuplicateRegisterNames = 1;
}

def RASMAsmParserVariant : AsmParserVariant {
  int Variant = 0;
  // Recognize hard coded registers.
  string RegisterPrefix = "$";
}

def RASMAsmWriter : AsmWriter {
  int PassSubtarget = 1;
}

def RASM : Target {
  let InstructionSet = RASMInstrInfo;
  let AssemblyParsers = [RASMAsmParser];
  let AssemblyParserVariants = [RASMAsmParserVariant];
  let AssemblyWriters = [RASMAsmWriter];
  let AllowRegisterRenaming = 1;
}
