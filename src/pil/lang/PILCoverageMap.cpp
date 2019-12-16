//===--- PILCoverageMap.cpp - Defines the PILCoverageMap class ------------===//
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
//
// This file defines the PILCoverageMap class, which is used to relay coverage
// mapping information from the AST to lower layers of the compiler.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/STLExtras.h"
#include "polarphp/pil/lang/PILCoverageMap.h"
#include "polarphp/pil/lang/PILModule.h"

using namespace polar;

using llvm::coverage::CounterExpression;

PILCoverageMap *
PILCoverageMap::create(PILModule &M, StringRef Filename, StringRef Name,
                       StringRef PGOFuncName, uint64_t Hash,
                       ArrayRef<MappedRegion> MappedRegions,
                       ArrayRef<CounterExpression> Expressions) {
   auto *Buf = M.allocate<PILCoverageMap>(1);
   PILCoverageMap *CM = ::new (Buf) PILCoverageMap(Hash);

   // Store a copy of the names so that we own the lifetime.
   CM->Filename = M.allocateCopy(Filename);
   CM->Name = M.allocateCopy(Name);
   CM->PGOFuncName = M.allocateCopy(PGOFuncName);

   // Since we have two arrays, we need to manually tail allocate each of them,
   // rather than relying on the flexible array trick.
   CM->MappedRegions = M.allocateCopy(MappedRegions);
   CM->Expressions = M.allocateCopy(Expressions);

   auto result = M.coverageMaps.insert({CM->PGOFuncName, CM});

   // Assert that this coverage map is unique.
   assert(result.second && "Duplicate coverage mapping for function");
   (void)result;

   return CM;
}

PILCoverageMap::PILCoverageMap(uint64_t Hash) : Hash(Hash) {}

PILCoverageMap::~PILCoverageMap() {}

namespace {
struct Printer {
   const llvm::coverage::Counter &C;
   ArrayRef<llvm::coverage::CounterExpression> Exprs;
   Printer(const llvm::coverage::Counter &C,
           ArrayRef<llvm::coverage::CounterExpression> Exprs)
      : C(C), Exprs(Exprs) {}

   void print(raw_ostream &OS) const {
      // TODO: This format's nice and human readable, but does it fit well with
      // PIL's relatively simple structure?
      if (C.isZero())
         OS << "zero";
      else if (C.isExpression()) {
         assert(C.getExpressionID() < Exprs.size() && "expression out of range");
         const auto &E = Exprs[C.getExpressionID()];
         OS << '(' << Printer(E.LHS, Exprs)
            << (E.Kind == CounterExpression::Add ? " + " : " - ")
            << Printer(E.RHS, Exprs) << ')';
      } else
         OS << C.getCounterID();
   }

   friend raw_ostream &operator<<(raw_ostream &OS, const Printer &P) {
      P.print(OS);
      return OS;
   }
};
} // end anonymous namespace

void PILCoverageMap::printCounter(llvm::raw_ostream &OS,
                                  llvm::coverage::Counter C) const {
   OS << Printer(C, getExpressions());
}
