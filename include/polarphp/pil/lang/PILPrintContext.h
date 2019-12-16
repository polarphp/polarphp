//===--- PILPrintContext.h - Context for PIL print functions ----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime LibraryPILProfiler.h Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PRINTCONTEXT_H
#define POLARPHP_PIL_PRINTCONTEXT_H

#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

namespace polar {

class PILDebugScope;
class PILInstruction;
class PILFunction;
class PILBasicBlock;

/// Used as context for the PIL print functions.
class PILPrintContext {
public:
   struct ID {
      enum ID_Kind { PILBasicBlock, PILUndef, SSAValue, Null } Kind;
      unsigned Number;

      // A stable ordering of ID objects.
      bool operator<(ID Other) const {
         if (unsigned(Kind) < unsigned(Other.Kind))
            return true;
         if (Number < Other.Number)
            return true;
         return false;
      }

      void print(raw_ostream &OS);
   };

protected:
   // Cache block and value identifiers for this function. This is useful in
   // general for identifying entities, not just emitting textual PIL.
   //
   const void *ContextFunctionOrBlock = nullptr;
   llvm::DenseMap<const PILBasicBlock *, unsigned> BlocksToIDMap;
   llvm::DenseMap<const PILNode *, unsigned> ValueToIDMap;

   llvm::raw_ostream &OutStream;

   llvm::DenseMap<const PILDebugScope *, unsigned> ScopeToIDMap;

   /// Dump more information in the PIL output.
   bool Verbose;

   /// Sort all kind of tables to ease diffing.
   bool SortedPIL;

   /// Print debug locations and scopes.
   bool DebugInfo;

public:
   /// Constructor with default values for options.
   ///
   /// DebugInfo will be set according to the -sil-print-debuginfo option.
   PILPrintContext(llvm::raw_ostream &OS, bool Verbose = false,
                   bool SortedPIL = false);

   PILPrintContext(llvm::raw_ostream &OS, bool Verbose,
                   bool SortedPIL, bool DebugInfo);

   virtual ~PILPrintContext();

   void setContext(const void *FunctionOrBlock);

   // Initialized block IDs from the order provided in `blocks`.
   void initBlockIDs(ArrayRef<const PILBasicBlock *> Blocks);

   /// Returns the output stream for printing.
   llvm::raw_ostream &OS() const { return OutStream; }

   /// Returns true if the PIL output should be sorted.
   bool sortPIL() const { return SortedPIL; }

   /// Returns true if verbose PIL should be printed.
   bool printVerbose() const { return Verbose; }

   /// Returns true if debug locations and scopes should be printed.
   bool printDebugInfo() const { return DebugInfo; }

   PILPrintContext::ID getID(const PILBasicBlock *Block);

   PILPrintContext::ID getID(const PILNode *node);

   /// Returns true if the \p Scope has and ID assigned.
   bool hasScopeID(const PILDebugScope *Scope) const {
      return ScopeToIDMap.count(Scope) != 0;
   }

   /// Returns the ID of \p Scope.
   unsigned getScopeID(const PILDebugScope *Scope) const {
      return ScopeToIDMap.lookup(Scope);
   }

   /// Assigns the next available ID to \p Scope.
   unsigned assignScopeID(const PILDebugScope *Scope) {
      assert(!hasScopeID(Scope));
      unsigned ID = ScopeToIDMap.size() + 1;
      ScopeToIDMap.insert({Scope, ID});
      return ID;
   }

   /// Callback which is invoked by the PILPrinter before the instruction \p I
   /// is written.
   virtual void printInstructionCallBack(const PILInstruction *I);
};

raw_ostream &operator<<(raw_ostream &OS, PILPrintContext::ID i);

} // end namespace polar

#endif // POLARPHP_PIL_PRINTCONTEXT_H
