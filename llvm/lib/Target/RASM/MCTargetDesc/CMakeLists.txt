add_llvm_component_library(LLVMRASMDesc
  RASMAsmBackend.cpp
  RASMBaseInfo.cpp
  RASMELFObjectWriter.cpp
  RASMELFStreamer.cpp
  RASMInstPrinter.cpp
  RASMMCAsmInfo.cpp
  RASMMCCodeEmitter.cpp
  RASMMCExpr.cpp
  RASMMCTargetDesc.cpp
  RASMMatInt.cpp
  RASMTargetStreamer.cpp

  LINK_COMPONENTS
  MC
  RASMInfo
  Support

  ADD_TO_COMPONENT
  RASM
  )
