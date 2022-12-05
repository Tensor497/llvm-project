add_llvm_component_group(RASM)

set(LLVM_TARGET_DEFINITIONS RASM.td)

tablegen(LLVM RASMGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM RASMGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM RASMGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM RASMGenDisassemblerTables.inc -gen-disassembler)
tablegen(LLVM RASMGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM RASMGenMCPseudoLowering.inc -gen-pseudo-lowering)
tablegen(LLVM RASMGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM RASMGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RASMGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(RASMCommonTableGen)

add_llvm_target(RASMCodeGen
  RASMAsmPrinter.cpp
  RASMExpandAtomicPseudoInsts.cpp
  RASMExpandPseudoInsts.cpp
  RASMFrameLowering.cpp
  RASMInstrInfo.cpp
  RASMISelDAGToDAG.cpp
  RASMISelLowering.cpp
  RASMMCInstLower.cpp
  RASMRegisterInfo.cpp
  RASMSubtarget.cpp
  RASMTargetMachine.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  CodeGen
  Core
  MC
  RASMDesc
  RASMInfo
  SelectionDAG
  Support
  Target
  GlobalISel

  ADD_TO_COMPONENT
  RASM
  )

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
