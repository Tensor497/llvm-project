//===-- RASMRegisterInfo.td - RASM Register defs -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the RASM register files
//===----------------------------------------------------------------------===//

let Namespace = "RASM" in {
class RASMReg<bits<16> Enc, string n, list<string> alt = []>
    : Register<n> {
  let HWEncoding = Enc;
  let AltNames = alt;
}

class RASMReg32<bits<16> Enc, string n, list<string> alt = []>
    : Register<n> {
  let HWEncoding = Enc;
  let AltNames = alt;
}

def sub_32 : SubRegIndex<32>;
class RASMReg64<RASMReg32 subreg>
    : Register<""> {
  let HWEncoding = subreg.HWEncoding;
  let SubRegs = [subreg];
  let SubRegIndices = [sub_32];
  let AsmName = subreg.AsmName;
  let AltNames = subreg.AltNames;
}

let FallbackRegAltNameIndex = NoRegAltName in
def RegAliasName : RegAltNameIndex;
} // Namespace = "RASM"

// Integer registers

let RegAltNameIndices = [RegAliasName] in {
  let isConstant = true in
  def R0  : RASMReg<0,  "r0", ["zero"]>, DwarfRegNum<[0]>;
  def R1  : RASMReg<1,  "r1", ["ra"]>, DwarfRegNum<[1]>;
  def R2  : RASMReg<2,  "r2", ["tp"]>, DwarfRegNum<[2]>;
  def R3  : RASMReg<3,  "r3", ["sp"]>, DwarfRegNum<[3]>;
  def R4  : RASMReg<4,  "r4", ["a0"]>, DwarfRegNum<[4]>;
  def R5  : RASMReg<5,  "r5", ["a1"]>, DwarfRegNum<[5]>;
  def R6  : RASMReg<6,  "r6", ["a2"]>, DwarfRegNum<[6]>;
  def R7  : RASMReg<7,  "r7", ["a3"]>, DwarfRegNum<[7]>;
  def R8  : RASMReg<8,  "r8", ["a4"]>, DwarfRegNum<[8]>;
  def R9  : RASMReg<9,  "r9", ["a5"]>, DwarfRegNum<[9]>;
  def R10 : RASMReg<10, "r10", ["a6"]>, DwarfRegNum<[10]>;
  def R11 : RASMReg<11, "r11", ["a7"]>, DwarfRegNum<[11]>;
  def R12 : RASMReg<12, "r12", ["t0"]>, DwarfRegNum<[12]>;
  def R13 : RASMReg<13, "r13", ["t1"]>, DwarfRegNum<[13]>;
  def R14 : RASMReg<14, "r14", ["t2"]>, DwarfRegNum<[14]>;
  def R15 : RASMReg<15, "r15", ["t3"]>, DwarfRegNum<[15]>;
  def R16 : RASMReg<16, "r16", ["t4"]>, DwarfRegNum<[16]>;
  def R17 : RASMReg<17, "r17", ["t5"]>, DwarfRegNum<[17]>;
  def R18 : RASMReg<18, "r18", ["t6"]>, DwarfRegNum<[18]>;
  def R19 : RASMReg<19, "r19", ["t7"]>, DwarfRegNum<[19]>;
  def R20 : RASMReg<20, "r20", ["t8"]>, DwarfRegNum<[20]>;
  def R21 : RASMReg<21, "r21", [""]>, DwarfRegNum<[21]>;
  def R22 : RASMReg<22, "r22", ["fp", "s9"]>, DwarfRegNum<[22]>;
  def R23 : RASMReg<23, "r23", ["s0"]>, DwarfRegNum<[23]>;
  def R24 : RASMReg<24, "r24", ["s1"]>, DwarfRegNum<[24]>;
  def R25 : RASMReg<25, "r25", ["s2"]>, DwarfRegNum<[25]>;
  def R26 : RASMReg<26, "r26", ["s3"]>, DwarfRegNum<[26]>;
  def R27 : RASMReg<27, "r27", ["s4"]>, DwarfRegNum<[27]>;
  def R28 : RASMReg<28, "r28", ["s5"]>, DwarfRegNum<[28]>;
  def R29 : RASMReg<29, "r29", ["s6"]>, DwarfRegNum<[29]>;
  def R30 : RASMReg<30, "r30", ["s7"]>, DwarfRegNum<[30]>;
  def R31 : RASMReg<31, "r31", ["s8"]>, DwarfRegNum<[31]>;
} // RegAltNameIndices = [RegAliasName]

def GRLenVT : ValueTypeByHwMode<[LA32, LA64],
                                [i32,  i64]>;
def GRLenRI : RegInfoByHwMode<
      [LA32,              LA64],
      [RegInfo<32,32,32>, RegInfo<64,64,64>]>;

// The order of registers represents the preferred allocation sequence.
// Registers are listed in the order caller-save, callee-save, specials.
def GPR : RegisterClass<"RASM", [GRLenVT], 32, (add
    // Argument registers (a0...a7)
    (sequence "R%u", 4, 11),
    // Temporary registers (t0...t8)
    (sequence "R%u", 12, 20),
    // Static register (s9/fp, s0...s8)
    (sequence "R%u", 22, 31),
    // Specials (r0, ra, tp, sp)
    (sequence "R%u", 0, 3),
    // Reserved (Non-allocatable)
    R21
  )> {
  let RegInfos = GRLenRI;
}

// GPR for indirect tail calls. We can't use callee-saved registers, as they are
// restored to the saved value before the tail call, which would clobber a call
// address.
def GPRT : RegisterClass<"RASM", [GRLenVT], 32, (add
    // a0...a7, t0...t8
    (sequence "R%u", 4, 20)
  )> {
  let RegInfos = GRLenRI;
}

// Floating point registers

let RegAltNameIndices = [RegAliasName] in {
  def F0  : RASMReg32<0, "f0", ["fa0"]>, DwarfRegNum<[32]>;
  def F1  : RASMReg32<1, "f1", ["fa1"]>, DwarfRegNum<[33]>;
  def F2  : RASMReg32<2, "f2", ["fa2"]>, DwarfRegNum<[34]>;
  def F3  : RASMReg32<3, "f3", ["fa3"]>, DwarfRegNum<[35]>;
  def F4  : RASMReg32<4, "f4", ["fa4"]>, DwarfRegNum<[36]>;
  def F5  : RASMReg32<5, "f5", ["fa5"]>, DwarfRegNum<[37]>;
  def F6  : RASMReg32<6, "f6", ["fa6"]>, DwarfRegNum<[38]>;
  def F7  : RASMReg32<7, "f7", ["fa7"]>, DwarfRegNum<[39]>;
  def F8  : RASMReg32<8, "f8", ["ft0"]>, DwarfRegNum<[40]>;
  def F9  : RASMReg32<9, "f9", ["ft1"]>, DwarfRegNum<[41]>;
  def F10 : RASMReg32<10,"f10", ["ft2"]>, DwarfRegNum<[42]>;
  def F11 : RASMReg32<11,"f11", ["ft3"]>, DwarfRegNum<[43]>;
  def F12 : RASMReg32<12,"f12", ["ft4"]>, DwarfRegNum<[44]>;
  def F13 : RASMReg32<13,"f13", ["ft5"]>, DwarfRegNum<[45]>;
  def F14 : RASMReg32<14,"f14", ["ft6"]>, DwarfRegNum<[46]>;
  def F15 : RASMReg32<15,"f15", ["ft7"]>, DwarfRegNum<[47]>;
  def F16 : RASMReg32<16,"f16", ["ft8"]>, DwarfRegNum<[48]>;
  def F17 : RASMReg32<17,"f17", ["ft9"]>, DwarfRegNum<[49]>;
  def F18 : RASMReg32<18,"f18", ["ft10"]>, DwarfRegNum<[50]>;
  def F19 : RASMReg32<19,"f19", ["ft11"]>, DwarfRegNum<[51]>;
  def F20 : RASMReg32<20,"f20", ["ft12"]>, DwarfRegNum<[52]>;
  def F21 : RASMReg32<21,"f21", ["ft13"]>, DwarfRegNum<[53]>;
  def F22 : RASMReg32<22,"f22", ["ft14"]>, DwarfRegNum<[54]>;
  def F23 : RASMReg32<23,"f23", ["ft15"]>, DwarfRegNum<[55]>;
  def F24 : RASMReg32<24,"f24", ["fs0"]>, DwarfRegNum<[56]>;
  def F25 : RASMReg32<25,"f25", ["fs1"]>, DwarfRegNum<[57]>;
  def F26 : RASMReg32<26,"f26", ["fs2"]>, DwarfRegNum<[58]>;
  def F27 : RASMReg32<27,"f27", ["fs3"]>, DwarfRegNum<[59]>;
  def F28 : RASMReg32<28,"f28", ["fs4"]>, DwarfRegNum<[60]>;
  def F29 : RASMReg32<29,"f29", ["fs5"]>, DwarfRegNum<[61]>;
  def F30 : RASMReg32<30,"f30", ["fs6"]>, DwarfRegNum<[62]>;
  def F31 : RASMReg32<31,"f31", ["fs7"]>, DwarfRegNum<[63]>;

  foreach I = 0-31 in {
    def F#I#_64 : RASMReg64<!cast<RASMReg32>("F"#I)>,
      DwarfRegNum<[!add(I, 32)]>;
  }
}

// The order of registers represents the preferred allocation sequence.
def FPR32 : RegisterClass<"RASM", [f32], 32, (sequence "F%u", 0, 31)>;
def FPR64 : RegisterClass<"RASM", [f64], 64, (sequence "F%u_64", 0, 31)>;

// Condition flag registers

foreach I = 0-7 in
def FCC#I : RASMReg<I, "fcc"#I>;

def CFR : RegisterClass<"RASM", [GRLenVT], 32, (sequence "FCC%u", 0, 7)> {
  let RegInfos = GRLenRI;
}

// Control and status registers

foreach I = 0-3 in
def FCSR#I : RASMReg<I, "fcsr"#I>;

let isAllocatable = false in
def FCSR : RegisterClass<"RASM", [i32], 32, (sequence "FCSR%u", 0, 3)>;
