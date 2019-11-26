# This file allows users to call find_package(Clang) and pick up our targets.
# Compute the installation prefix from this LLVMConfig.cmake file location.
set(CLANG_INSTALL_PREFIX ${POLAR_DEPS_INSTALL_DIR})

find_package(LLVM REQUIRED CONFIG
             HINTS "${POLAR_CMAKE_MODULES_DIR}/llvm")

set(CLANG_EXPORTED_TARGETS "clangBasic;clangLex;clangParse;clangAST;clangDynamicASTMatchers;clangASTMatchers;clangCrossTU;clangSema;clangCodeGen;clangAnalysis;clangEdit;clangRewrite;clangARCMigrate;clangDriver;clangSerialization;clangRewriteFrontend;clangFrontend;clangFrontendTool;clangToolingCore;clangToolingInclusions;clangToolingRefactoring;clangToolingASTDiff;clangToolingSyntax;clangDependencyScanning;clangTransformer;clangTooling;clangDirectoryWatcher;clangIndex;clangStaticAnalyzerCore;clangStaticAnalyzerCheckers;clangStaticAnalyzerFrontend;clangFormat;clang;clang-format;clangHandleCXX;clangHandleLLVM;clang-import-test;clang-offload-bundler;clang-offload-wrapper;clang-scan-deps;clang-rename;clang-refactor;clang-cpp;clang-check;clang-extdef-mapping;libclang")
set(CLANG_CMAKE_DIR "${POLAR_CMAKE_MODULES_DIR}/clang")
set(CLANG_INCLUDE_DIRS "${CLANG_INSTALL_PREFIX}/include")

# Provide all our library targets to users.
include("${CLANG_CMAKE_DIR}/ClangTargets.cmake")

# By creating clang-tablegen-targets here, subprojects that depend on Clang's
# tablegen-generated headers can always depend on this target whether building
# in-tree with Clang or not.
if(NOT TARGET clang-tablegen-targets)
  add_custom_target(clang-tablegen-targets)
endif()
