//===-- RASMELFObjectWriter.cpp - RASM ELF Writer ---*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RASMFixupKinds.h"
#include "MCTargetDesc/RASMMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class RASMELFObjectWriter : public MCELFObjectTargetWriter {
public:
  RASMELFObjectWriter(uint8_t OSABI, bool Is64Bit);

  ~RASMELFObjectWriter() override;

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup, bool IsPCRel) const override;
};
} // end namespace

RASMELFObjectWriter::RASMELFObjectWriter(uint8_t OSABI, bool Is64Bit)
    : MCELFObjectTargetWriter(Is64Bit, OSABI, ELF::EM_RASM,
                              /*HasRelocationAddend*/ true) {}

RASMELFObjectWriter::~RASMELFObjectWriter() {}

unsigned RASMELFObjectWriter::getRelocType(MCContext &Ctx,
                                                const MCValue &Target,
                                                const MCFixup &Fixup,
                                                bool IsPCRel) const {
  // Determine the type of the relocation
  unsigned Kind = Fixup.getTargetKind();

  if (Kind >= FirstLiteralRelocationKind)
    return Kind - FirstLiteralRelocationKind;

  switch (Kind) {
  default:
    Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
    return ELF::R_LARCH_NONE;
  case FK_Data_1:
    Ctx.reportError(Fixup.getLoc(), "1-byte data relocations not supported");
    return ELF::R_LARCH_NONE;
  case FK_Data_2:
    Ctx.reportError(Fixup.getLoc(), "2-byte data relocations not supported");
    return ELF::R_LARCH_NONE;
  case FK_Data_4:
    return IsPCRel ? ELF::R_LARCH_32_PCREL : ELF::R_LARCH_32;
  case FK_Data_8:
    return ELF::R_LARCH_64;
  case RASM::fixup_loongarch_b16:
    return ELF::R_LARCH_B16;
  case RASM::fixup_loongarch_b21:
    return ELF::R_LARCH_B21;
  case RASM::fixup_loongarch_b26:
    return ELF::R_LARCH_B26;
  case RASM::fixup_loongarch_abs_hi20:
    return ELF::R_LARCH_ABS_HI20;
  case RASM::fixup_loongarch_abs_lo12:
    return ELF::R_LARCH_ABS_LO12;
  case RASM::fixup_loongarch_abs64_lo20:
    return ELF::R_LARCH_ABS64_LO20;
  case RASM::fixup_loongarch_abs64_hi12:
    return ELF::R_LARCH_ABS64_HI12;
  case RASM::fixup_loongarch_tls_le_hi20:
    return ELF::R_LARCH_TLS_LE_HI20;
  case RASM::fixup_loongarch_tls_le_lo12:
    return ELF::R_LARCH_TLS_LE_LO12;
  case RASM::fixup_loongarch_tls_le64_lo20:
    return ELF::R_LARCH_TLS_LE64_LO20;
  case RASM::fixup_loongarch_tls_le64_hi12:
    return ELF::R_LARCH_TLS_LE64_HI12;
    // TODO: Handle more fixup-kinds.
  }
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createRASMELFObjectWriter(uint8_t OSABI, bool Is64Bit) {
  return std::make_unique<RASMELFObjectWriter>(OSABI, Is64Bit);
}
