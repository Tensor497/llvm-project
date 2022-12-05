add_llvm_component_library(LLVMRASMAsmParser
  RASMAsmParser.cpp

  LINK_COMPONENTS
  RASMDesc
  RASMInfo
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  RASM
  )
