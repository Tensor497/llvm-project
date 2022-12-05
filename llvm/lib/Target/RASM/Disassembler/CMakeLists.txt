add_llvm_component_library(LLVMRASMDisassembler
  RASMDisassembler.cpp

  LINK_COMPONENTS
  RASMDesc
  RASMInfo
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  RASM
  )
