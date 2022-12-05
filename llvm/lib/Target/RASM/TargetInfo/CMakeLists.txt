add_llvm_component_library(LLVMRASMInfo
  RASMTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  RASM
  )
