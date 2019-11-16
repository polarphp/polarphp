#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LLVMDemangle" for configuration "Release"
set_property(TARGET LLVMDemangle APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDemangle PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDemangle.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDemangle.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDemangle )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDemangle "${_IMPORT_PREFIX}/lib/libLLVMDemangle.dylib" )

# Import target "LLVMSupport" for configuration "Release"
set_property(TARGET LLVMSupport APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSupport PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSupport.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSupport.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSupport )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSupport "${_IMPORT_PREFIX}/lib/libLLVMSupport.dylib" )

# Import target "LLVMTableGen" for configuration "Release"
set_property(TARGET LLVMTableGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMTableGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMTableGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMTableGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMTableGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMTableGen "${_IMPORT_PREFIX}/lib/libLLVMTableGen.dylib" )

# Import target "llvm-tblgen" for configuration "Release"
set_property(TARGET llvm-tblgen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-tblgen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-tblgen"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-tblgen )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-tblgen "${_IMPORT_PREFIX}/bin/llvm-tblgen" )

# Import target "LLVMCore" for configuration "Release"
set_property(TARGET LLVMCore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMCore PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMCore.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMCore.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMCore )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMCore "${_IMPORT_PREFIX}/lib/libLLVMCore.dylib" )

# Import target "LLVMFuzzMutate" for configuration "Release"
set_property(TARGET LLVMFuzzMutate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMFuzzMutate PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMFuzzMutate.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMFuzzMutate.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMFuzzMutate )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMFuzzMutate "${_IMPORT_PREFIX}/lib/libLLVMFuzzMutate.dylib" )

# Import target "LLVMIRReader" for configuration "Release"
set_property(TARGET LLVMIRReader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMIRReader PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMIRReader.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMIRReader.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMIRReader )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMIRReader "${_IMPORT_PREFIX}/lib/libLLVMIRReader.dylib" )

# Import target "LLVMCodeGen" for configuration "Release"
set_property(TARGET LLVMCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMCodeGen "${_IMPORT_PREFIX}/lib/libLLVMCodeGen.dylib" )

# Import target "LLVMSelectionDAG" for configuration "Release"
set_property(TARGET LLVMSelectionDAG APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSelectionDAG PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSelectionDAG.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSelectionDAG.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSelectionDAG )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSelectionDAG "${_IMPORT_PREFIX}/lib/libLLVMSelectionDAG.dylib" )

# Import target "LLVMAsmPrinter" for configuration "Release"
set_property(TARGET LLVMAsmPrinter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAsmPrinter PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAsmPrinter.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAsmPrinter.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAsmPrinter )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAsmPrinter "${_IMPORT_PREFIX}/lib/libLLVMAsmPrinter.dylib" )

# Import target "LLVMMIRParser" for configuration "Release"
set_property(TARGET LLVMMIRParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMIRParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMIRParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMIRParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMIRParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMIRParser "${_IMPORT_PREFIX}/lib/libLLVMMIRParser.dylib" )

# Import target "LLVMGlobalISel" for configuration "Release"
set_property(TARGET LLVMGlobalISel APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMGlobalISel PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMGlobalISel.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMGlobalISel.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMGlobalISel )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMGlobalISel "${_IMPORT_PREFIX}/lib/libLLVMGlobalISel.dylib" )

# Import target "LLVMBinaryFormat" for configuration "Release"
set_property(TARGET LLVMBinaryFormat APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBinaryFormat PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBinaryFormat.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBinaryFormat.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBinaryFormat )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBinaryFormat "${_IMPORT_PREFIX}/lib/libLLVMBinaryFormat.dylib" )

# Import target "LLVMBitReader" for configuration "Release"
set_property(TARGET LLVMBitReader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBitReader PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBitReader.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBitReader.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBitReader )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBitReader "${_IMPORT_PREFIX}/lib/libLLVMBitReader.dylib" )

# Import target "LLVMBitWriter" for configuration "Release"
set_property(TARGET LLVMBitWriter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBitWriter PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBitWriter.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBitWriter.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBitWriter )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBitWriter "${_IMPORT_PREFIX}/lib/libLLVMBitWriter.dylib" )

# Import target "LLVMBitstreamReader" for configuration "Release"
set_property(TARGET LLVMBitstreamReader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBitstreamReader PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBitstreamReader.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBitstreamReader.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBitstreamReader )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBitstreamReader "${_IMPORT_PREFIX}/lib/libLLVMBitstreamReader.dylib" )

# Import target "LLVMTransformUtils" for configuration "Release"
set_property(TARGET LLVMTransformUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMTransformUtils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMTransformUtils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMTransformUtils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMTransformUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMTransformUtils "${_IMPORT_PREFIX}/lib/libLLVMTransformUtils.dylib" )

# Import target "LLVMInstrumentation" for configuration "Release"
set_property(TARGET LLVMInstrumentation APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMInstrumentation PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMInstrumentation.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMInstrumentation.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMInstrumentation )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMInstrumentation "${_IMPORT_PREFIX}/lib/libLLVMInstrumentation.dylib" )

# Import target "LLVMAggressiveInstCombine" for configuration "Release"
set_property(TARGET LLVMAggressiveInstCombine APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAggressiveInstCombine PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAggressiveInstCombine.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAggressiveInstCombine.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAggressiveInstCombine )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAggressiveInstCombine "${_IMPORT_PREFIX}/lib/libLLVMAggressiveInstCombine.dylib" )

# Import target "LLVMInstCombine" for configuration "Release"
set_property(TARGET LLVMInstCombine APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMInstCombine PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMInstCombine.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMInstCombine.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMInstCombine )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMInstCombine "${_IMPORT_PREFIX}/lib/libLLVMInstCombine.dylib" )

# Import target "LLVMScalarOpts" for configuration "Release"
set_property(TARGET LLVMScalarOpts APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMScalarOpts PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMScalarOpts.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMScalarOpts.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMScalarOpts )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMScalarOpts "${_IMPORT_PREFIX}/lib/libLLVMScalarOpts.dylib" )

# Import target "LLVMipo" for configuration "Release"
set_property(TARGET LLVMipo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMipo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMipo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMipo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMipo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMipo "${_IMPORT_PREFIX}/lib/libLLVMipo.dylib" )

# Import target "LLVMVectorize" for configuration "Release"
set_property(TARGET LLVMVectorize APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMVectorize PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMVectorize.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMVectorize.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMVectorize )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMVectorize "${_IMPORT_PREFIX}/lib/libLLVMVectorize.dylib" )

# Import target "LLVMObjCARCOpts" for configuration "Release"
set_property(TARGET LLVMObjCARCOpts APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMObjCARCOpts PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMObjCARCOpts.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMObjCARCOpts.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMObjCARCOpts )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMObjCARCOpts "${_IMPORT_PREFIX}/lib/libLLVMObjCARCOpts.dylib" )

# Import target "LLVMCoroutines" for configuration "Release"
set_property(TARGET LLVMCoroutines APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMCoroutines PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMCoroutines.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMCoroutines.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMCoroutines )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMCoroutines "${_IMPORT_PREFIX}/lib/libLLVMCoroutines.dylib" )

# Import target "LLVMCFGuard" for configuration "Release"
set_property(TARGET LLVMCFGuard APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMCFGuard PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMCFGuard.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMCFGuard.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMCFGuard )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMCFGuard "${_IMPORT_PREFIX}/lib/libLLVMCFGuard.dylib" )

# Import target "LLVMLinker" for configuration "Release"
set_property(TARGET LLVMLinker APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLinker PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLinker.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLinker.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLinker )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLinker "${_IMPORT_PREFIX}/lib/libLLVMLinker.dylib" )

# Import target "LLVMAnalysis" for configuration "Release"
set_property(TARGET LLVMAnalysis APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAnalysis PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAnalysis.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAnalysis.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAnalysis )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAnalysis "${_IMPORT_PREFIX}/lib/libLLVMAnalysis.dylib" )

# Import target "LLVMLTO" for configuration "Release"
set_property(TARGET LLVMLTO APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLTO PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLTO.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLTO.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLTO )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLTO "${_IMPORT_PREFIX}/lib/libLLVMLTO.dylib" )

# Import target "LLVMMC" for configuration "Release"
set_property(TARGET LLVMMC APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMC PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMC.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMC.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMC )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMC "${_IMPORT_PREFIX}/lib/libLLVMMC.dylib" )

# Import target "LLVMMCParser" for configuration "Release"
set_property(TARGET LLVMMCParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMCParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMCParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMCParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMCParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMCParser "${_IMPORT_PREFIX}/lib/libLLVMMCParser.dylib" )

# Import target "LLVMMCDisassembler" for configuration "Release"
set_property(TARGET LLVMMCDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMCDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMCDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMCDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMCDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMCDisassembler "${_IMPORT_PREFIX}/lib/libLLVMMCDisassembler.dylib" )

# Import target "LLVMMCA" for configuration "Release"
set_property(TARGET LLVMMCA APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMCA PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMCA.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMCA.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMCA )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMCA "${_IMPORT_PREFIX}/lib/libLLVMMCA.dylib" )

# Import target "LLVMObject" for configuration "Release"
set_property(TARGET LLVMObject APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMObject PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMObject.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMObject.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMObject )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMObject "${_IMPORT_PREFIX}/lib/libLLVMObject.dylib" )

# Import target "LLVMObjectYAML" for configuration "Release"
set_property(TARGET LLVMObjectYAML APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMObjectYAML PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMObjectYAML.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMObjectYAML.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMObjectYAML )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMObjectYAML "${_IMPORT_PREFIX}/lib/libLLVMObjectYAML.dylib" )

# Import target "LLVMOption" for configuration "Release"
set_property(TARGET LLVMOption APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMOption PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMOption.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMOption.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMOption )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMOption "${_IMPORT_PREFIX}/lib/libLLVMOption.dylib" )

# Import target "LLVMRemarks" for configuration "Release"
set_property(TARGET LLVMRemarks APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRemarks PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRemarks.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRemarks.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRemarks )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRemarks "${_IMPORT_PREFIX}/lib/libLLVMRemarks.dylib" )

# Import target "LLVMDebugInfoDWARF" for configuration "Release"
set_property(TARGET LLVMDebugInfoDWARF APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDebugInfoDWARF PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoDWARF.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDebugInfoDWARF.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDebugInfoDWARF )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDebugInfoDWARF "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoDWARF.dylib" )

# Import target "LLVMDebugInfoGSYM" for configuration "Release"
set_property(TARGET LLVMDebugInfoGSYM APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDebugInfoGSYM PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoGSYM.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDebugInfoGSYM.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDebugInfoGSYM )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDebugInfoGSYM "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoGSYM.dylib" )

# Import target "LLVMDebugInfoMSF" for configuration "Release"
set_property(TARGET LLVMDebugInfoMSF APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDebugInfoMSF PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoMSF.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDebugInfoMSF.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDebugInfoMSF )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDebugInfoMSF "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoMSF.dylib" )

# Import target "LLVMDebugInfoCodeView" for configuration "Release"
set_property(TARGET LLVMDebugInfoCodeView APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDebugInfoCodeView PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoCodeView.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDebugInfoCodeView.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDebugInfoCodeView )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDebugInfoCodeView "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoCodeView.dylib" )

# Import target "LLVMDebugInfoPDB" for configuration "Release"
set_property(TARGET LLVMDebugInfoPDB APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDebugInfoPDB PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoPDB.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDebugInfoPDB.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDebugInfoPDB )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDebugInfoPDB "${_IMPORT_PREFIX}/lib/libLLVMDebugInfoPDB.dylib" )

# Import target "LLVMSymbolize" for configuration "Release"
set_property(TARGET LLVMSymbolize APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSymbolize PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSymbolize.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSymbolize.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSymbolize )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSymbolize "${_IMPORT_PREFIX}/lib/libLLVMSymbolize.dylib" )

# Import target "LLVMExecutionEngine" for configuration "Release"
set_property(TARGET LLVMExecutionEngine APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMExecutionEngine PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "LLVMCore;LLVMMC;LLVMObject;LLVMSupport;LLVMTarget"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMExecutionEngine.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMExecutionEngine.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMExecutionEngine )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMExecutionEngine "${_IMPORT_PREFIX}/lib/libLLVMExecutionEngine.dylib" )

# Import target "LLVMInterpreter" for configuration "Release"
set_property(TARGET LLVMInterpreter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMInterpreter PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMInterpreter.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMInterpreter.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMInterpreter )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMInterpreter "${_IMPORT_PREFIX}/lib/libLLVMInterpreter.dylib" )

# Import target "LLVMJITLink" for configuration "Release"
set_property(TARGET LLVMJITLink APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMJITLink PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMJITLink.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMJITLink.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMJITLink )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMJITLink "${_IMPORT_PREFIX}/lib/libLLVMJITLink.dylib" )

# Import target "LLVMMCJIT" for configuration "Release"
set_property(TARGET LLVMMCJIT APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMCJIT PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMCJIT.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMCJIT.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMCJIT )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMCJIT "${_IMPORT_PREFIX}/lib/libLLVMMCJIT.dylib" )

# Import target "LLVMOrcError" for configuration "Release"
set_property(TARGET LLVMOrcError APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMOrcError PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMOrcError.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMOrcError.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMOrcError )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMOrcError "${_IMPORT_PREFIX}/lib/libLLVMOrcError.dylib" )

# Import target "LLVMOrcJIT" for configuration "Release"
set_property(TARGET LLVMOrcJIT APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMOrcJIT PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMOrcJIT.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMOrcJIT.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMOrcJIT )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMOrcJIT "${_IMPORT_PREFIX}/lib/libLLVMOrcJIT.dylib" )

# Import target "LLVMRuntimeDyld" for configuration "Release"
set_property(TARGET LLVMRuntimeDyld APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRuntimeDyld PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRuntimeDyld.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRuntimeDyld.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRuntimeDyld )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRuntimeDyld "${_IMPORT_PREFIX}/lib/libLLVMRuntimeDyld.dylib" )

# Import target "LLVMTarget" for configuration "Release"
set_property(TARGET LLVMTarget APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMTarget PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMTarget.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMTarget.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMTarget )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMTarget "${_IMPORT_PREFIX}/lib/libLLVMTarget.dylib" )

# Import target "LLVMAArch64CodeGen" for configuration "Release"
set_property(TARGET LLVMAArch64CodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64CodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64CodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64CodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64CodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64CodeGen "${_IMPORT_PREFIX}/lib/libLLVMAArch64CodeGen.dylib" )

# Import target "LLVMAArch64AsmParser" for configuration "Release"
set_property(TARGET LLVMAArch64AsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64AsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64AsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64AsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64AsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64AsmParser "${_IMPORT_PREFIX}/lib/libLLVMAArch64AsmParser.dylib" )

# Import target "LLVMAArch64Disassembler" for configuration "Release"
set_property(TARGET LLVMAArch64Disassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64Disassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64Disassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64Disassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64Disassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64Disassembler "${_IMPORT_PREFIX}/lib/libLLVMAArch64Disassembler.dylib" )

# Import target "LLVMAArch64Desc" for configuration "Release"
set_property(TARGET LLVMAArch64Desc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64Desc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64Desc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64Desc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64Desc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64Desc "${_IMPORT_PREFIX}/lib/libLLVMAArch64Desc.dylib" )

# Import target "LLVMAArch64Info" for configuration "Release"
set_property(TARGET LLVMAArch64Info APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64Info PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64Info.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64Info.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64Info )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64Info "${_IMPORT_PREFIX}/lib/libLLVMAArch64Info.dylib" )

# Import target "LLVMAArch64Utils" for configuration "Release"
set_property(TARGET LLVMAArch64Utils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAArch64Utils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAArch64Utils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAArch64Utils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAArch64Utils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAArch64Utils "${_IMPORT_PREFIX}/lib/libLLVMAArch64Utils.dylib" )

# Import target "LLVMAMDGPUCodeGen" for configuration "Release"
set_property(TARGET LLVMAMDGPUCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUCodeGen "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUCodeGen.dylib" )

# Import target "LLVMAMDGPUAsmParser" for configuration "Release"
set_property(TARGET LLVMAMDGPUAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUAsmParser "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUAsmParser.dylib" )

# Import target "LLVMAMDGPUDisassembler" for configuration "Release"
set_property(TARGET LLVMAMDGPUDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUDisassembler "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUDisassembler.dylib" )

# Import target "LLVMAMDGPUDesc" for configuration "Release"
set_property(TARGET LLVMAMDGPUDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUDesc "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUDesc.dylib" )

# Import target "LLVMAMDGPUInfo" for configuration "Release"
set_property(TARGET LLVMAMDGPUInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUInfo "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUInfo.dylib" )

# Import target "LLVMAMDGPUUtils" for configuration "Release"
set_property(TARGET LLVMAMDGPUUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAMDGPUUtils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUUtils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAMDGPUUtils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAMDGPUUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAMDGPUUtils "${_IMPORT_PREFIX}/lib/libLLVMAMDGPUUtils.dylib" )

# Import target "LLVMARMCodeGen" for configuration "Release"
set_property(TARGET LLVMARMCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMCodeGen "${_IMPORT_PREFIX}/lib/libLLVMARMCodeGen.dylib" )

# Import target "LLVMARMAsmParser" for configuration "Release"
set_property(TARGET LLVMARMAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMAsmParser "${_IMPORT_PREFIX}/lib/libLLVMARMAsmParser.dylib" )

# Import target "LLVMARMDisassembler" for configuration "Release"
set_property(TARGET LLVMARMDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMDisassembler "${_IMPORT_PREFIX}/lib/libLLVMARMDisassembler.dylib" )

# Import target "LLVMARMDesc" for configuration "Release"
set_property(TARGET LLVMARMDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMDesc "${_IMPORT_PREFIX}/lib/libLLVMARMDesc.dylib" )

# Import target "LLVMARMInfo" for configuration "Release"
set_property(TARGET LLVMARMInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMInfo "${_IMPORT_PREFIX}/lib/libLLVMARMInfo.dylib" )

# Import target "LLVMARMUtils" for configuration "Release"
set_property(TARGET LLVMARMUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMARMUtils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMARMUtils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMARMUtils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMARMUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMARMUtils "${_IMPORT_PREFIX}/lib/libLLVMARMUtils.dylib" )

# Import target "LLVMBPFCodeGen" for configuration "Release"
set_property(TARGET LLVMBPFCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBPFCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBPFCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBPFCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBPFCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBPFCodeGen "${_IMPORT_PREFIX}/lib/libLLVMBPFCodeGen.dylib" )

# Import target "LLVMBPFAsmParser" for configuration "Release"
set_property(TARGET LLVMBPFAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBPFAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBPFAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBPFAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBPFAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBPFAsmParser "${_IMPORT_PREFIX}/lib/libLLVMBPFAsmParser.dylib" )

# Import target "LLVMBPFDisassembler" for configuration "Release"
set_property(TARGET LLVMBPFDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBPFDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBPFDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBPFDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBPFDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBPFDisassembler "${_IMPORT_PREFIX}/lib/libLLVMBPFDisassembler.dylib" )

# Import target "LLVMBPFDesc" for configuration "Release"
set_property(TARGET LLVMBPFDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBPFDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBPFDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBPFDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBPFDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBPFDesc "${_IMPORT_PREFIX}/lib/libLLVMBPFDesc.dylib" )

# Import target "LLVMBPFInfo" for configuration "Release"
set_property(TARGET LLVMBPFInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMBPFInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMBPFInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMBPFInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMBPFInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMBPFInfo "${_IMPORT_PREFIX}/lib/libLLVMBPFInfo.dylib" )

# Import target "LLVMHexagonCodeGen" for configuration "Release"
set_property(TARGET LLVMHexagonCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMHexagonCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMHexagonCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMHexagonCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMHexagonCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMHexagonCodeGen "${_IMPORT_PREFIX}/lib/libLLVMHexagonCodeGen.dylib" )

# Import target "LLVMHexagonAsmParser" for configuration "Release"
set_property(TARGET LLVMHexagonAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMHexagonAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMHexagonAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMHexagonAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMHexagonAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMHexagonAsmParser "${_IMPORT_PREFIX}/lib/libLLVMHexagonAsmParser.dylib" )

# Import target "LLVMHexagonDisassembler" for configuration "Release"
set_property(TARGET LLVMHexagonDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMHexagonDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMHexagonDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMHexagonDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMHexagonDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMHexagonDisassembler "${_IMPORT_PREFIX}/lib/libLLVMHexagonDisassembler.dylib" )

# Import target "LLVMHexagonDesc" for configuration "Release"
set_property(TARGET LLVMHexagonDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMHexagonDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMHexagonDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMHexagonDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMHexagonDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMHexagonDesc "${_IMPORT_PREFIX}/lib/libLLVMHexagonDesc.dylib" )

# Import target "LLVMHexagonInfo" for configuration "Release"
set_property(TARGET LLVMHexagonInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMHexagonInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMHexagonInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMHexagonInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMHexagonInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMHexagonInfo "${_IMPORT_PREFIX}/lib/libLLVMHexagonInfo.dylib" )

# Import target "LLVMLanaiCodeGen" for configuration "Release"
set_property(TARGET LLVMLanaiCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLanaiCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLanaiCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLanaiCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLanaiCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLanaiCodeGen "${_IMPORT_PREFIX}/lib/libLLVMLanaiCodeGen.dylib" )

# Import target "LLVMLanaiAsmParser" for configuration "Release"
set_property(TARGET LLVMLanaiAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLanaiAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLanaiAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLanaiAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLanaiAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLanaiAsmParser "${_IMPORT_PREFIX}/lib/libLLVMLanaiAsmParser.dylib" )

# Import target "LLVMLanaiDisassembler" for configuration "Release"
set_property(TARGET LLVMLanaiDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLanaiDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLanaiDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLanaiDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLanaiDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLanaiDisassembler "${_IMPORT_PREFIX}/lib/libLLVMLanaiDisassembler.dylib" )

# Import target "LLVMLanaiDesc" for configuration "Release"
set_property(TARGET LLVMLanaiDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLanaiDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLanaiDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLanaiDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLanaiDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLanaiDesc "${_IMPORT_PREFIX}/lib/libLLVMLanaiDesc.dylib" )

# Import target "LLVMLanaiInfo" for configuration "Release"
set_property(TARGET LLVMLanaiInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLanaiInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLanaiInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLanaiInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLanaiInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLanaiInfo "${_IMPORT_PREFIX}/lib/libLLVMLanaiInfo.dylib" )

# Import target "LLVMMipsCodeGen" for configuration "Release"
set_property(TARGET LLVMMipsCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMipsCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMipsCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMipsCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMipsCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMipsCodeGen "${_IMPORT_PREFIX}/lib/libLLVMMipsCodeGen.dylib" )

# Import target "LLVMMipsAsmParser" for configuration "Release"
set_property(TARGET LLVMMipsAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMipsAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMipsAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMipsAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMipsAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMipsAsmParser "${_IMPORT_PREFIX}/lib/libLLVMMipsAsmParser.dylib" )

# Import target "LLVMMipsDisassembler" for configuration "Release"
set_property(TARGET LLVMMipsDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMipsDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMipsDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMipsDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMipsDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMipsDisassembler "${_IMPORT_PREFIX}/lib/libLLVMMipsDisassembler.dylib" )

# Import target "LLVMMipsDesc" for configuration "Release"
set_property(TARGET LLVMMipsDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMipsDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMipsDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMipsDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMipsDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMipsDesc "${_IMPORT_PREFIX}/lib/libLLVMMipsDesc.dylib" )

# Import target "LLVMMipsInfo" for configuration "Release"
set_property(TARGET LLVMMipsInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMipsInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMipsInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMipsInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMipsInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMipsInfo "${_IMPORT_PREFIX}/lib/libLLVMMipsInfo.dylib" )

# Import target "LLVMMSP430CodeGen" for configuration "Release"
set_property(TARGET LLVMMSP430CodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMSP430CodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMSP430CodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMSP430CodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMSP430CodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMSP430CodeGen "${_IMPORT_PREFIX}/lib/libLLVMMSP430CodeGen.dylib" )

# Import target "LLVMMSP430Desc" for configuration "Release"
set_property(TARGET LLVMMSP430Desc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMSP430Desc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMSP430Desc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMSP430Desc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMSP430Desc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMSP430Desc "${_IMPORT_PREFIX}/lib/libLLVMMSP430Desc.dylib" )

# Import target "LLVMMSP430Info" for configuration "Release"
set_property(TARGET LLVMMSP430Info APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMSP430Info PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMSP430Info.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMSP430Info.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMSP430Info )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMSP430Info "${_IMPORT_PREFIX}/lib/libLLVMMSP430Info.dylib" )

# Import target "LLVMMSP430AsmParser" for configuration "Release"
set_property(TARGET LLVMMSP430AsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMSP430AsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMSP430AsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMSP430AsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMSP430AsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMSP430AsmParser "${_IMPORT_PREFIX}/lib/libLLVMMSP430AsmParser.dylib" )

# Import target "LLVMMSP430Disassembler" for configuration "Release"
set_property(TARGET LLVMMSP430Disassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMMSP430Disassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMMSP430Disassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMMSP430Disassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMMSP430Disassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMMSP430Disassembler "${_IMPORT_PREFIX}/lib/libLLVMMSP430Disassembler.dylib" )

# Import target "LLVMNVPTXCodeGen" for configuration "Release"
set_property(TARGET LLVMNVPTXCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMNVPTXCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMNVPTXCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMNVPTXCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMNVPTXCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMNVPTXCodeGen "${_IMPORT_PREFIX}/lib/libLLVMNVPTXCodeGen.dylib" )

# Import target "LLVMNVPTXDesc" for configuration "Release"
set_property(TARGET LLVMNVPTXDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMNVPTXDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMNVPTXDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMNVPTXDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMNVPTXDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMNVPTXDesc "${_IMPORT_PREFIX}/lib/libLLVMNVPTXDesc.dylib" )

# Import target "LLVMNVPTXInfo" for configuration "Release"
set_property(TARGET LLVMNVPTXInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMNVPTXInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMNVPTXInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMNVPTXInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMNVPTXInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMNVPTXInfo "${_IMPORT_PREFIX}/lib/libLLVMNVPTXInfo.dylib" )

# Import target "LLVMPowerPCCodeGen" for configuration "Release"
set_property(TARGET LLVMPowerPCCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPowerPCCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPowerPCCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPowerPCCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPowerPCCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPowerPCCodeGen "${_IMPORT_PREFIX}/lib/libLLVMPowerPCCodeGen.dylib" )

# Import target "LLVMPowerPCAsmParser" for configuration "Release"
set_property(TARGET LLVMPowerPCAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPowerPCAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPowerPCAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPowerPCAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPowerPCAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPowerPCAsmParser "${_IMPORT_PREFIX}/lib/libLLVMPowerPCAsmParser.dylib" )

# Import target "LLVMPowerPCDisassembler" for configuration "Release"
set_property(TARGET LLVMPowerPCDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPowerPCDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPowerPCDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPowerPCDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPowerPCDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPowerPCDisassembler "${_IMPORT_PREFIX}/lib/libLLVMPowerPCDisassembler.dylib" )

# Import target "LLVMPowerPCDesc" for configuration "Release"
set_property(TARGET LLVMPowerPCDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPowerPCDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPowerPCDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPowerPCDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPowerPCDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPowerPCDesc "${_IMPORT_PREFIX}/lib/libLLVMPowerPCDesc.dylib" )

# Import target "LLVMPowerPCInfo" for configuration "Release"
set_property(TARGET LLVMPowerPCInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPowerPCInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPowerPCInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPowerPCInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPowerPCInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPowerPCInfo "${_IMPORT_PREFIX}/lib/libLLVMPowerPCInfo.dylib" )

# Import target "LLVMRISCVCodeGen" for configuration "Release"
set_property(TARGET LLVMRISCVCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVCodeGen "${_IMPORT_PREFIX}/lib/libLLVMRISCVCodeGen.dylib" )

# Import target "LLVMRISCVAsmParser" for configuration "Release"
set_property(TARGET LLVMRISCVAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVAsmParser "${_IMPORT_PREFIX}/lib/libLLVMRISCVAsmParser.dylib" )

# Import target "LLVMRISCVDisassembler" for configuration "Release"
set_property(TARGET LLVMRISCVDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVDisassembler "${_IMPORT_PREFIX}/lib/libLLVMRISCVDisassembler.dylib" )

# Import target "LLVMRISCVDesc" for configuration "Release"
set_property(TARGET LLVMRISCVDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVDesc "${_IMPORT_PREFIX}/lib/libLLVMRISCVDesc.dylib" )

# Import target "LLVMRISCVInfo" for configuration "Release"
set_property(TARGET LLVMRISCVInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVInfo "${_IMPORT_PREFIX}/lib/libLLVMRISCVInfo.dylib" )

# Import target "LLVMRISCVUtils" for configuration "Release"
set_property(TARGET LLVMRISCVUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMRISCVUtils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMRISCVUtils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMRISCVUtils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMRISCVUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMRISCVUtils "${_IMPORT_PREFIX}/lib/libLLVMRISCVUtils.dylib" )

# Import target "LLVMSparcCodeGen" for configuration "Release"
set_property(TARGET LLVMSparcCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSparcCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSparcCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSparcCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSparcCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSparcCodeGen "${_IMPORT_PREFIX}/lib/libLLVMSparcCodeGen.dylib" )

# Import target "LLVMSparcAsmParser" for configuration "Release"
set_property(TARGET LLVMSparcAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSparcAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSparcAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSparcAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSparcAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSparcAsmParser "${_IMPORT_PREFIX}/lib/libLLVMSparcAsmParser.dylib" )

# Import target "LLVMSparcDisassembler" for configuration "Release"
set_property(TARGET LLVMSparcDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSparcDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSparcDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSparcDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSparcDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSparcDisassembler "${_IMPORT_PREFIX}/lib/libLLVMSparcDisassembler.dylib" )

# Import target "LLVMSparcDesc" for configuration "Release"
set_property(TARGET LLVMSparcDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSparcDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSparcDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSparcDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSparcDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSparcDesc "${_IMPORT_PREFIX}/lib/libLLVMSparcDesc.dylib" )

# Import target "LLVMSparcInfo" for configuration "Release"
set_property(TARGET LLVMSparcInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSparcInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSparcInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSparcInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSparcInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSparcInfo "${_IMPORT_PREFIX}/lib/libLLVMSparcInfo.dylib" )

# Import target "LLVMSystemZCodeGen" for configuration "Release"
set_property(TARGET LLVMSystemZCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSystemZCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSystemZCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSystemZCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSystemZCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSystemZCodeGen "${_IMPORT_PREFIX}/lib/libLLVMSystemZCodeGen.dylib" )

# Import target "LLVMSystemZAsmParser" for configuration "Release"
set_property(TARGET LLVMSystemZAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSystemZAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSystemZAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSystemZAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSystemZAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSystemZAsmParser "${_IMPORT_PREFIX}/lib/libLLVMSystemZAsmParser.dylib" )

# Import target "LLVMSystemZDisassembler" for configuration "Release"
set_property(TARGET LLVMSystemZDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSystemZDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSystemZDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSystemZDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSystemZDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSystemZDisassembler "${_IMPORT_PREFIX}/lib/libLLVMSystemZDisassembler.dylib" )

# Import target "LLVMSystemZDesc" for configuration "Release"
set_property(TARGET LLVMSystemZDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSystemZDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSystemZDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSystemZDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSystemZDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSystemZDesc "${_IMPORT_PREFIX}/lib/libLLVMSystemZDesc.dylib" )

# Import target "LLVMSystemZInfo" for configuration "Release"
set_property(TARGET LLVMSystemZInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMSystemZInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMSystemZInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMSystemZInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMSystemZInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMSystemZInfo "${_IMPORT_PREFIX}/lib/libLLVMSystemZInfo.dylib" )

# Import target "LLVMWebAssemblyCodeGen" for configuration "Release"
set_property(TARGET LLVMWebAssemblyCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWebAssemblyCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWebAssemblyCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWebAssemblyCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWebAssemblyCodeGen "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyCodeGen.dylib" )

# Import target "LLVMWebAssemblyAsmParser" for configuration "Release"
set_property(TARGET LLVMWebAssemblyAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWebAssemblyAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWebAssemblyAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWebAssemblyAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWebAssemblyAsmParser "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyAsmParser.dylib" )

# Import target "LLVMWebAssemblyDisassembler" for configuration "Release"
set_property(TARGET LLVMWebAssemblyDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWebAssemblyDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWebAssemblyDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWebAssemblyDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWebAssemblyDisassembler "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyDisassembler.dylib" )

# Import target "LLVMWebAssemblyDesc" for configuration "Release"
set_property(TARGET LLVMWebAssemblyDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWebAssemblyDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWebAssemblyDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWebAssemblyDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWebAssemblyDesc "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyDesc.dylib" )

# Import target "LLVMWebAssemblyInfo" for configuration "Release"
set_property(TARGET LLVMWebAssemblyInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWebAssemblyInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWebAssemblyInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWebAssemblyInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWebAssemblyInfo "${_IMPORT_PREFIX}/lib/libLLVMWebAssemblyInfo.dylib" )

# Import target "LLVMX86CodeGen" for configuration "Release"
set_property(TARGET LLVMX86CodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86CodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86CodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86CodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86CodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86CodeGen "${_IMPORT_PREFIX}/lib/libLLVMX86CodeGen.dylib" )

# Import target "LLVMX86AsmParser" for configuration "Release"
set_property(TARGET LLVMX86AsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86AsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86AsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86AsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86AsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86AsmParser "${_IMPORT_PREFIX}/lib/libLLVMX86AsmParser.dylib" )

# Import target "LLVMX86Disassembler" for configuration "Release"
set_property(TARGET LLVMX86Disassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86Disassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86Disassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86Disassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86Disassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86Disassembler "${_IMPORT_PREFIX}/lib/libLLVMX86Disassembler.dylib" )

# Import target "LLVMX86Desc" for configuration "Release"
set_property(TARGET LLVMX86Desc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86Desc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86Desc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86Desc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86Desc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86Desc "${_IMPORT_PREFIX}/lib/libLLVMX86Desc.dylib" )

# Import target "LLVMX86Info" for configuration "Release"
set_property(TARGET LLVMX86Info APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86Info PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86Info.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86Info.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86Info )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86Info "${_IMPORT_PREFIX}/lib/libLLVMX86Info.dylib" )

# Import target "LLVMX86Utils" for configuration "Release"
set_property(TARGET LLVMX86Utils APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMX86Utils PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMX86Utils.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMX86Utils.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMX86Utils )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMX86Utils "${_IMPORT_PREFIX}/lib/libLLVMX86Utils.dylib" )

# Import target "LLVMXCoreCodeGen" for configuration "Release"
set_property(TARGET LLVMXCoreCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMXCoreCodeGen PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMXCoreCodeGen.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMXCoreCodeGen.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMXCoreCodeGen )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMXCoreCodeGen "${_IMPORT_PREFIX}/lib/libLLVMXCoreCodeGen.dylib" )

# Import target "LLVMXCoreDisassembler" for configuration "Release"
set_property(TARGET LLVMXCoreDisassembler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMXCoreDisassembler PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMXCoreDisassembler.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMXCoreDisassembler.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMXCoreDisassembler )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMXCoreDisassembler "${_IMPORT_PREFIX}/lib/libLLVMXCoreDisassembler.dylib" )

# Import target "LLVMXCoreDesc" for configuration "Release"
set_property(TARGET LLVMXCoreDesc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMXCoreDesc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMXCoreDesc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMXCoreDesc.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMXCoreDesc )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMXCoreDesc "${_IMPORT_PREFIX}/lib/libLLVMXCoreDesc.dylib" )

# Import target "LLVMXCoreInfo" for configuration "Release"
set_property(TARGET LLVMXCoreInfo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMXCoreInfo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMXCoreInfo.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMXCoreInfo.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMXCoreInfo )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMXCoreInfo "${_IMPORT_PREFIX}/lib/libLLVMXCoreInfo.dylib" )

# Import target "LLVMAsmParser" for configuration "Release"
set_property(TARGET LLVMAsmParser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMAsmParser PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMAsmParser.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMAsmParser.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMAsmParser )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMAsmParser "${_IMPORT_PREFIX}/lib/libLLVMAsmParser.dylib" )

# Import target "LLVMLineEditor" for configuration "Release"
set_property(TARGET LLVMLineEditor APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLineEditor PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLineEditor.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLineEditor.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLineEditor )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLineEditor "${_IMPORT_PREFIX}/lib/libLLVMLineEditor.dylib" )

# Import target "LLVMProfileData" for configuration "Release"
set_property(TARGET LLVMProfileData APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMProfileData PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMProfileData.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMProfileData.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMProfileData )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMProfileData "${_IMPORT_PREFIX}/lib/libLLVMProfileData.dylib" )

# Import target "LLVMCoverage" for configuration "Release"
set_property(TARGET LLVMCoverage APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMCoverage PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMCoverage.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMCoverage.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMCoverage )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMCoverage "${_IMPORT_PREFIX}/lib/libLLVMCoverage.dylib" )

# Import target "LLVMPasses" for configuration "Release"
set_property(TARGET LLVMPasses APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMPasses PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMPasses.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMPasses.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMPasses )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMPasses "${_IMPORT_PREFIX}/lib/libLLVMPasses.dylib" )

# Import target "LLVMTextAPI" for configuration "Release"
set_property(TARGET LLVMTextAPI APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMTextAPI PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMTextAPI.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMTextAPI.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMTextAPI )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMTextAPI "${_IMPORT_PREFIX}/lib/libLLVMTextAPI.dylib" )

# Import target "LLVMDlltoolDriver" for configuration "Release"
set_property(TARGET LLVMDlltoolDriver APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMDlltoolDriver PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMDlltoolDriver.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMDlltoolDriver.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMDlltoolDriver )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMDlltoolDriver "${_IMPORT_PREFIX}/lib/libLLVMDlltoolDriver.dylib" )

# Import target "LLVMLibDriver" for configuration "Release"
set_property(TARGET LLVMLibDriver APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMLibDriver PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMLibDriver.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMLibDriver.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMLibDriver )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMLibDriver "${_IMPORT_PREFIX}/lib/libLLVMLibDriver.dylib" )

# Import target "LLVMXRay" for configuration "Release"
set_property(TARGET LLVMXRay APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMXRay PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMXRay.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMXRay.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMXRay )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMXRay "${_IMPORT_PREFIX}/lib/libLLVMXRay.dylib" )

# Import target "LLVMWindowsManifest" for configuration "Release"
set_property(TARGET LLVMWindowsManifest APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LLVMWindowsManifest PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLLVMWindowsManifest.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLLVMWindowsManifest.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LLVMWindowsManifest )
list(APPEND _IMPORT_CHECK_FILES_FOR_LLVMWindowsManifest "${_IMPORT_PREFIX}/lib/libLLVMWindowsManifest.dylib" )

# Import target "LTO" for configuration "Release"
set_property(TARGET LTO APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LTO PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLTO.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libLTO.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS LTO )
list(APPEND _IMPORT_CHECK_FILES_FOR_LTO "${_IMPORT_PREFIX}/lib/libLTO.dylib" )

# Import target "llvm-ar" for configuration "Release"
set_property(TARGET llvm-ar APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-ar PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-ar"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-ar )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-ar "${_IMPORT_PREFIX}/bin/llvm-ar" )

# Import target "llvm-config" for configuration "Release"
set_property(TARGET llvm-config APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-config PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-config"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-config )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-config "${_IMPORT_PREFIX}/bin/llvm-config" )

# Import target "llvm-lto" for configuration "Release"
set_property(TARGET llvm-lto APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-lto PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-lto"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-lto )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-lto "${_IMPORT_PREFIX}/bin/llvm-lto" )

# Import target "llvm-profdata" for configuration "Release"
set_property(TARGET llvm-profdata APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-profdata PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-profdata"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-profdata )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-profdata "${_IMPORT_PREFIX}/bin/llvm-profdata" )

# Import target "bugpoint" for configuration "Release"
set_property(TARGET bugpoint APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(bugpoint PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/bugpoint"
  )

list(APPEND _IMPORT_CHECK_TARGETS bugpoint )
list(APPEND _IMPORT_CHECK_FILES_FOR_bugpoint "${_IMPORT_PREFIX}/bin/bugpoint" )

# Import target "dsymutil" for configuration "Release"
set_property(TARGET dsymutil APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(dsymutil PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/dsymutil"
  )

list(APPEND _IMPORT_CHECK_TARGETS dsymutil )
list(APPEND _IMPORT_CHECK_FILES_FOR_dsymutil "${_IMPORT_PREFIX}/bin/dsymutil" )

# Import target "llc" for configuration "Release"
set_property(TARGET llc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llc"
  )

list(APPEND _IMPORT_CHECK_TARGETS llc )
list(APPEND _IMPORT_CHECK_FILES_FOR_llc "${_IMPORT_PREFIX}/bin/llc" )

# Import target "lli" for configuration "Release"
set_property(TARGET lli APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(lli PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/lli"
  )

list(APPEND _IMPORT_CHECK_TARGETS lli )
list(APPEND _IMPORT_CHECK_FILES_FOR_lli "${_IMPORT_PREFIX}/bin/lli" )

# Import target "llvm-as" for configuration "Release"
set_property(TARGET llvm-as APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-as PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-as"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-as )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-as "${_IMPORT_PREFIX}/bin/llvm-as" )

# Import target "llvm-bcanalyzer" for configuration "Release"
set_property(TARGET llvm-bcanalyzer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-bcanalyzer PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-bcanalyzer"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-bcanalyzer )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-bcanalyzer "${_IMPORT_PREFIX}/bin/llvm-bcanalyzer" )

# Import target "llvm-c-test" for configuration "Release"
set_property(TARGET llvm-c-test APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-c-test PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-c-test"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-c-test )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-c-test "${_IMPORT_PREFIX}/bin/llvm-c-test" )

# Import target "llvm-cat" for configuration "Release"
set_property(TARGET llvm-cat APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cat PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cat"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cat )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cat "${_IMPORT_PREFIX}/bin/llvm-cat" )

# Import target "llvm-cfi-verify" for configuration "Release"
set_property(TARGET llvm-cfi-verify APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cfi-verify PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cfi-verify"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cfi-verify )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cfi-verify "${_IMPORT_PREFIX}/bin/llvm-cfi-verify" )

# Import target "llvm-cov" for configuration "Release"
set_property(TARGET llvm-cov APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cov PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cov"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cov )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cov "${_IMPORT_PREFIX}/bin/llvm-cov" )

# Import target "llvm-cvtres" for configuration "Release"
set_property(TARGET llvm-cvtres APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cvtres PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cvtres"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cvtres )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cvtres "${_IMPORT_PREFIX}/bin/llvm-cvtres" )

# Import target "llvm-cxxdump" for configuration "Release"
set_property(TARGET llvm-cxxdump APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cxxdump PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cxxdump"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cxxdump )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cxxdump "${_IMPORT_PREFIX}/bin/llvm-cxxdump" )

# Import target "llvm-cxxfilt" for configuration "Release"
set_property(TARGET llvm-cxxfilt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cxxfilt PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cxxfilt"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cxxfilt )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cxxfilt "${_IMPORT_PREFIX}/bin/llvm-cxxfilt" )

# Import target "llvm-cxxmap" for configuration "Release"
set_property(TARGET llvm-cxxmap APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-cxxmap PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-cxxmap"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-cxxmap )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-cxxmap "${_IMPORT_PREFIX}/bin/llvm-cxxmap" )

# Import target "llvm-diff" for configuration "Release"
set_property(TARGET llvm-diff APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-diff PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-diff"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-diff )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-diff "${_IMPORT_PREFIX}/bin/llvm-diff" )

# Import target "llvm-dis" for configuration "Release"
set_property(TARGET llvm-dis APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-dis PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-dis"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-dis )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-dis "${_IMPORT_PREFIX}/bin/llvm-dis" )

# Import target "llvm-dwarfdump" for configuration "Release"
set_property(TARGET llvm-dwarfdump APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-dwarfdump PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-dwarfdump"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-dwarfdump )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-dwarfdump "${_IMPORT_PREFIX}/bin/llvm-dwarfdump" )

# Import target "llvm-dwp" for configuration "Release"
set_property(TARGET llvm-dwp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-dwp PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-dwp"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-dwp )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-dwp "${_IMPORT_PREFIX}/bin/llvm-dwp" )

# Import target "llvm-elfabi" for configuration "Release"
set_property(TARGET llvm-elfabi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-elfabi PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-elfabi"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-elfabi )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-elfabi "${_IMPORT_PREFIX}/bin/llvm-elfabi" )

# Import target "llvm-exegesis" for configuration "Release"
set_property(TARGET llvm-exegesis APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-exegesis PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-exegesis"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-exegesis )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-exegesis "${_IMPORT_PREFIX}/bin/llvm-exegesis" )

# Import target "llvm-extract" for configuration "Release"
set_property(TARGET llvm-extract APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-extract PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-extract"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-extract )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-extract "${_IMPORT_PREFIX}/bin/llvm-extract" )

# Import target "llvm-ifs" for configuration "Release"
set_property(TARGET llvm-ifs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-ifs PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-ifs"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-ifs )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-ifs "${_IMPORT_PREFIX}/bin/llvm-ifs" )

# Import target "llvm-jitlink" for configuration "Release"
set_property(TARGET llvm-jitlink APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-jitlink PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-jitlink"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-jitlink )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-jitlink "${_IMPORT_PREFIX}/bin/llvm-jitlink" )

# Import target "llvm-link" for configuration "Release"
set_property(TARGET llvm-link APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-link PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-link"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-link )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-link "${_IMPORT_PREFIX}/bin/llvm-link" )

# Import target "llvm-lipo" for configuration "Release"
set_property(TARGET llvm-lipo APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-lipo PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-lipo"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-lipo )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-lipo "${_IMPORT_PREFIX}/bin/llvm-lipo" )

# Import target "llvm-lto2" for configuration "Release"
set_property(TARGET llvm-lto2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-lto2 PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-lto2"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-lto2 )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-lto2 "${_IMPORT_PREFIX}/bin/llvm-lto2" )

# Import target "llvm-mc" for configuration "Release"
set_property(TARGET llvm-mc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-mc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-mc"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-mc )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-mc "${_IMPORT_PREFIX}/bin/llvm-mc" )

# Import target "llvm-mca" for configuration "Release"
set_property(TARGET llvm-mca APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-mca PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-mca"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-mca )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-mca "${_IMPORT_PREFIX}/bin/llvm-mca" )

# Import target "llvm-modextract" for configuration "Release"
set_property(TARGET llvm-modextract APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-modextract PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-modextract"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-modextract )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-modextract "${_IMPORT_PREFIX}/bin/llvm-modextract" )

# Import target "llvm-mt" for configuration "Release"
set_property(TARGET llvm-mt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-mt PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-mt"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-mt )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-mt "${_IMPORT_PREFIX}/bin/llvm-mt" )

# Import target "llvm-nm" for configuration "Release"
set_property(TARGET llvm-nm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-nm PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-nm"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-nm )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-nm "${_IMPORT_PREFIX}/bin/llvm-nm" )

# Import target "llvm-objcopy" for configuration "Release"
set_property(TARGET llvm-objcopy APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-objcopy PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-objcopy"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-objcopy )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-objcopy "${_IMPORT_PREFIX}/bin/llvm-objcopy" )

# Import target "llvm-objdump" for configuration "Release"
set_property(TARGET llvm-objdump APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-objdump PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-objdump"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-objdump )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-objdump "${_IMPORT_PREFIX}/bin/llvm-objdump" )

# Import target "llvm-opt-report" for configuration "Release"
set_property(TARGET llvm-opt-report APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-opt-report PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-opt-report"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-opt-report )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-opt-report "${_IMPORT_PREFIX}/bin/llvm-opt-report" )

# Import target "llvm-pdbutil" for configuration "Release"
set_property(TARGET llvm-pdbutil APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-pdbutil PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-pdbutil"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-pdbutil )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-pdbutil "${_IMPORT_PREFIX}/bin/llvm-pdbutil" )

# Import target "llvm-rc" for configuration "Release"
set_property(TARGET llvm-rc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-rc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-rc"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-rc )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-rc "${_IMPORT_PREFIX}/bin/llvm-rc" )

# Import target "llvm-readobj" for configuration "Release"
set_property(TARGET llvm-readobj APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-readobj PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-readobj"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-readobj )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-readobj "${_IMPORT_PREFIX}/bin/llvm-readobj" )

# Import target "llvm-reduce" for configuration "Release"
set_property(TARGET llvm-reduce APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-reduce PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-reduce"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-reduce )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-reduce "${_IMPORT_PREFIX}/bin/llvm-reduce" )

# Import target "llvm-rtdyld" for configuration "Release"
set_property(TARGET llvm-rtdyld APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-rtdyld PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-rtdyld"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-rtdyld )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-rtdyld "${_IMPORT_PREFIX}/bin/llvm-rtdyld" )

# Import target "llvm-size" for configuration "Release"
set_property(TARGET llvm-size APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-size PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-size"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-size )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-size "${_IMPORT_PREFIX}/bin/llvm-size" )

# Import target "llvm-split" for configuration "Release"
set_property(TARGET llvm-split APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-split PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-split"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-split )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-split "${_IMPORT_PREFIX}/bin/llvm-split" )

# Import target "llvm-stress" for configuration "Release"
set_property(TARGET llvm-stress APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-stress PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-stress"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-stress )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-stress "${_IMPORT_PREFIX}/bin/llvm-stress" )

# Import target "llvm-strings" for configuration "Release"
set_property(TARGET llvm-strings APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-strings PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-strings"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-strings )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-strings "${_IMPORT_PREFIX}/bin/llvm-strings" )

# Import target "llvm-symbolizer" for configuration "Release"
set_property(TARGET llvm-symbolizer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-symbolizer PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-symbolizer"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-symbolizer )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-symbolizer "${_IMPORT_PREFIX}/bin/llvm-symbolizer" )

# Import target "llvm-undname" for configuration "Release"
set_property(TARGET llvm-undname APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-undname PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-undname"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-undname )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-undname "${_IMPORT_PREFIX}/bin/llvm-undname" )

# Import target "llvm-xray" for configuration "Release"
set_property(TARGET llvm-xray APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(llvm-xray PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/llvm-xray"
  )

list(APPEND _IMPORT_CHECK_TARGETS llvm-xray )
list(APPEND _IMPORT_CHECK_FILES_FOR_llvm-xray "${_IMPORT_PREFIX}/bin/llvm-xray" )

# Import target "obj2yaml" for configuration "Release"
set_property(TARGET obj2yaml APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(obj2yaml PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/obj2yaml"
  )

list(APPEND _IMPORT_CHECK_TARGETS obj2yaml )
list(APPEND _IMPORT_CHECK_FILES_FOR_obj2yaml "${_IMPORT_PREFIX}/bin/obj2yaml" )

# Import target "opt" for configuration "Release"
set_property(TARGET opt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opt PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/opt"
  )

list(APPEND _IMPORT_CHECK_TARGETS opt )
list(APPEND _IMPORT_CHECK_FILES_FOR_opt "${_IMPORT_PREFIX}/bin/opt" )

# Import target "Remarks" for configuration "Release"
set_property(TARGET Remarks APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Remarks PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libRemarks.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libRemarks.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS Remarks )
list(APPEND _IMPORT_CHECK_FILES_FOR_Remarks "${_IMPORT_PREFIX}/lib/libRemarks.dylib" )

# Import target "sancov" for configuration "Release"
set_property(TARGET sancov APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sancov PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/sancov"
  )

list(APPEND _IMPORT_CHECK_TARGETS sancov )
list(APPEND _IMPORT_CHECK_FILES_FOR_sancov "${_IMPORT_PREFIX}/bin/sancov" )

# Import target "sanstats" for configuration "Release"
set_property(TARGET sanstats APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sanstats PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/sanstats"
  )

list(APPEND _IMPORT_CHECK_TARGETS sanstats )
list(APPEND _IMPORT_CHECK_FILES_FOR_sanstats "${_IMPORT_PREFIX}/bin/sanstats" )

# Import target "verify-uselistorder" for configuration "Release"
set_property(TARGET verify-uselistorder APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(verify-uselistorder PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/verify-uselistorder"
  )

list(APPEND _IMPORT_CHECK_TARGETS verify-uselistorder )
list(APPEND _IMPORT_CHECK_FILES_FOR_verify-uselistorder "${_IMPORT_PREFIX}/bin/verify-uselistorder" )

# Import target "yaml2obj" for configuration "Release"
set_property(TARGET yaml2obj APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(yaml2obj PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/yaml2obj"
  )

list(APPEND _IMPORT_CHECK_TARGETS yaml2obj )
list(APPEND _IMPORT_CHECK_FILES_FOR_yaml2obj "${_IMPORT_PREFIX}/bin/yaml2obj" )

# Import target "ExampleIRTransforms" for configuration "Release"
set_property(TARGET ExampleIRTransforms APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(ExampleIRTransforms PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libExampleIRTransforms.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libExampleIRTransforms.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS ExampleIRTransforms )
list(APPEND _IMPORT_CHECK_FILES_FOR_ExampleIRTransforms "${_IMPORT_PREFIX}/lib/libExampleIRTransforms.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
