//===--- PassesFwd.h - Creation functions for LLVM passes -------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_LLVMPASSES_PASSESFWD_H
#define POLARPHP_LLVMPASSES_PASSESFWD_H

namespace llvm {
class ModulePass;
class FunctionPass;
class ImmutablePass;
class PassRegistry;

void initializePolarphpAAWrapperPassPass(PassRegistry &);
void initializePolarphpRCIdentityPass(PassRegistry &);
void initializePolarphpARCOptPass(PassRegistry &);
void initializePolarphpARCContractPass(PassRegistry &);
void initializeInlineTreePrinterPass(PassRegistry &);
void initializePolarphpMergeFunctionsPass(PassRegistry &);
}

namespace polar {
llvm::FunctionPass *createPolarphpARCOptPass();
llvm::FunctionPass *createPolarphpARCContractPass();
llvm::ModulePass *createInlineTreePrinterPass();
llvm::ModulePass *createPolarphpMergeFunctionsPass();
llvm::ImmutablePass *createPolarphpAAWrapperPass();
llvm::ImmutablePass *createPolarphpRCIdentityPass();
} // end namespace polar

#endif
