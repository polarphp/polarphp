//===- PILFunctionConventions.h - Defines PIL func. conventions -*- C++ -*-===//
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
///
/// \file
///
/// This file defines the PILModuleConventions and PILFunctionConventions
/// classes.  These interfaces are used to determine when PIL can represent
/// values of a given lowered type by value and when they must be represented by
/// address. This is influenced by a PILModule-wide "lowered address" convention,
/// which reflects whether the current PIL stage requires lowered addresses.
///
/// The primary purpose of this API is mapping the formal PIL parameter and
/// result conventions onto the PIL argument types. The "formal" conventions are
/// immutably associated with a PILFunctionType--a PIL function's type
/// information never changes. The PIL conventions determine how those formal
/// conventions will be represented in the body of PIL functions and at call
/// sites.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_FUNCTIONCONVENTIONS_H
#define POLARPHP_PIL_FUNCTIONCONVENTIONS_H

#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/PILArgumentConvention.h"
#include "polarphp/pil/lang/PILType.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/ADT/iterator_range.h"

namespace polar {

class PILParameterInfo;
class PILResultInfo;
class PILFunctionConventions;
class PILYieldInfo;
enum class ResultConvention;
using llvm::iterator_range;

template<bool _, template<typename...> class T, typename...Args>
struct delay_template_expansion {
   using type = T<Args...>;
};

/// Transient wrapper for PILParameterInfo and PILResultInfo conventions. This
/// abstraction helps handle the transition from canonical PIL conventions to
/// lowered PIL conventions.
class PILModuleConventions {
   friend PILParameterInfo;
   friend PILResultInfo;
   friend PILFunctionConventions;

   static bool isIndirectPILParam(PILParameterInfo param,
                                  bool loweredAddresses);

   static bool isIndirectPILYield(PILYieldInfo yield,
                                  bool loweredAddresses);

   static bool isIndirectPILResult(PILResultInfo result,
                                   bool loweredAddresses);

   static PILType getPILParamInterfaceType(
      PILParameterInfo yield,
      bool loweredAddresses);

   static PILType getPILYieldInterfaceType(
      PILYieldInfo yield,
      bool loweredAddresses);

   static PILType getPILResultInterfaceType(
      PILResultInfo param,
      bool loweredAddresses);

public:
   static bool isPassedIndirectlyInPIL(PILType type, PILModule &M);

   static bool isReturnedIndirectlyInPIL(PILType type, PILModule &M);

   static PILModuleConventions getLoweredAddressConventions(PILModule &M) {
      return PILModuleConventions(M, true);
   }

private:
   PILModule *M;
   bool loweredAddresses;

   PILModuleConventions(PILModule &M, bool loweredAddresses)
      : M(&M), loweredAddresses(loweredAddresses)
   {}

public:
   PILModuleConventions(PILModule &M);

   PILFunctionConventions getFunctionConventions(CanPILFunctionType funcTy);

   PILModule &getModule() const { return *M; }

   bool useLoweredAddresses() const { return loweredAddresses; }

   bool isPILIndirect(PILParameterInfo param) const {
      return isIndirectPILParam(param, loweredAddresses);
   }

   bool isPILIndirect(PILYieldInfo yield) const {
      return isIndirectPILYield(yield, loweredAddresses);
   }

   bool isPILIndirect(PILResultInfo result) const {
      return isIndirectPILResult(result, loweredAddresses);
   }

   PILType getPILType(PILParameterInfo param,
                      CanPILFunctionType funcTy) const {
      auto interfaceTy = getPILParamInterfaceType(param, loweredAddresses);
      // TODO: Always require a function type
      if (funcTy)
         return funcTy->substInterfaceType(*M, interfaceTy);
      return interfaceTy;
   }

   PILType getPILType(PILYieldInfo yield,
                      CanPILFunctionType funcTy) const {
      auto interfaceTy = getPILYieldInterfaceType(yield, loweredAddresses);
      // TODO: Always require a function type
      if (funcTy)
         return funcTy->substInterfaceType(*M, interfaceTy);
      return interfaceTy;
   }

   PILType getPILType(PILResultInfo result,
                      CanPILFunctionType funcTy) const {
      auto interfaceTy = getPILResultInterfaceType(result, loweredAddresses);
      // TODO: Always require a function type
      if (funcTy)
         return funcTy->substInterfaceType(*M, interfaceTy);
      return interfaceTy;
   }
};

/// Transient wrapper for PIL-level argument conventions. This abstraction helps
/// handle the transition from canonical PIL conventions to lowered PIL
/// conventions.
class PILFunctionConventions {
public:
   PILModuleConventions silConv;
   CanPILFunctionType funcTy;

   PILFunctionConventions(CanPILFunctionType funcTy, PILModule &M)
      : silConv(M), funcTy(funcTy) {}

   PILFunctionConventions(CanPILFunctionType funcTy,
                          PILModuleConventions silConv)
      : silConv(silConv), funcTy(funcTy) {}

   //===--------------------------------------------------------------------===//
   // PILModuleConventions API for convenience.
   //===--------------------------------------------------------------------===//

   bool useLoweredAddresses() const { return silConv.useLoweredAddresses(); }

   bool isPILIndirect(PILParameterInfo param) const {
      return silConv.isPILIndirect(param);
   }

   bool isPILIndirect(PILYieldInfo yield) const {
      return silConv.isPILIndirect(yield);
   }

   bool isPILIndirect(PILResultInfo result) const {
      return silConv.isPILIndirect(result);
   }

   PILType getPILType(PILParameterInfo param) const {
      return silConv.getPILType(param, funcTy);
   }

   PILType getPILType(PILYieldInfo yield) const {
      return silConv.getPILType(yield, funcTy);
   }

   PILType getPILType(PILResultInfo result) const {
      return silConv.getPILType(result, funcTy);
   }

   //===--------------------------------------------------------------------===//
   // PIL results.
   //===--------------------------------------------------------------------===//

   /// Get the normal result type of an apply that calls this function.
   /// This does not include indirect PIL results.
   PILType getPILResultType() {
      if (silConv.loweredAddresses)
         return funcTy->getDirectFormalResultsType(silConv.getModule());

      return funcTy->getAllResultsSubstType(silConv.getModule());
   }

   /// Get the PIL type for the single result which may be direct or indirect.
   PILType getSinglePILResultType() {
      return getPILType(funcTy->getSingleResult());
   }

   /// Get the error result type.
   PILType getPILErrorType() { return getPILType(funcTy->getErrorResult()); }

   /// Returns an array of result info.
   /// Provides convenient access to the underlying PILFunctionType.
   ArrayRef<PILResultInfo> getResults() const {
      return funcTy->getResults();
   }

   /// Get the number of PIL results passed as address-typed arguments.
   unsigned getNumIndirectPILResults() const {
      return silConv.loweredAddresses ? funcTy->getNumIndirectFormalResults() : 0;
   }

   /// Are any PIL results passed as address-typed arguments?
   bool hasIndirectPILResults() const { return getNumIndirectPILResults() != 0; }

   using IndirectPILResultIter = PILFunctionType::IndirectFormalResultIter;
   using IndirectPILResultRange = PILFunctionType::IndirectFormalResultRange;

   /// Return a range of indirect result information for results passed as
   /// address-typed PIL arguments.
   IndirectPILResultRange getIndirectPILResults() const {
      if (silConv.loweredAddresses)
         return funcTy->getIndirectFormalResults();

      return llvm::make_filter_range(
         llvm::make_range((const PILResultInfo *)0, (const PILResultInfo *)0),
         PILFunctionType::IndirectFormalResultFilter());
   }

   struct PILResultTypeFunc;

   // Gratuitous template parameter is to delay instantiating `mapped_iterator`
   // on the incomplete type PILParameterTypeFunc.
   template<bool _ = false>
   using IndirectPILResultTypeIter = typename delay_template_expansion<_,
      llvm::mapped_iterator, IndirectPILResultIter, PILResultTypeFunc>::type;
   template<bool _ = false>
   using IndirectPILResultTypeRange = iterator_range<IndirectPILResultTypeIter<_>>;

   /// Return a range of PILTypes for each result passed as an address-typed PIL
   /// argument.
   template<bool _ = false>
   IndirectPILResultTypeRange<_> getIndirectPILResultTypes() const;

   /// Get the number of PIL results directly returned by PIL value.
   unsigned getNumDirectPILResults() const {
      return silConv.loweredAddresses ? funcTy->getNumDirectFormalResults()
                                      : funcTy->getNumResults();
   }

   struct DirectPILResultFilter {
      bool loweredAddresses;
      DirectPILResultFilter(bool loweredAddresses)
         : loweredAddresses(loweredAddresses) {}
      bool operator()(PILResultInfo result) const {
         return !(loweredAddresses && result.isFormalIndirect());
      }
   };
   using DirectPILResultIter =
   llvm::filter_iterator<const PILResultInfo *, DirectPILResultFilter>;
   using DirectPILResultRange = iterator_range<DirectPILResultIter>;

   /// Return a range of direct result information for results directly returned
   /// by PIL value.
   DirectPILResultRange getDirectPILResults() const {
      return llvm::make_filter_range(
         funcTy->getResults(), DirectPILResultFilter(silConv.loweredAddresses));
   }

   template<bool _ = false>
   using DirectPILResultTypeIter = typename delay_template_expansion<_,
      llvm::mapped_iterator, DirectPILResultIter, PILResultTypeFunc>::type;
   template<bool _ = false>
   using DirectPILResultTypeRange = iterator_range<DirectPILResultTypeIter<_>>;

   /// Return a range of PILTypes for each result directly returned
   /// by PIL value.
   template<bool _ = false>
   DirectPILResultTypeRange<_> getDirectPILResultTypes() const;

   //===--------------------------------------------------------------------===//
   // PIL parameters types.
   //===--------------------------------------------------------------------===//

   /// Returns the number of function parameters, not including any formally
   /// indirect results. Provides convenient access to the underlying
   /// PILFunctionType.
   unsigned getNumParameters() const { return funcTy->getNumParameters(); }

   /// Returns an array of parameter info, not including indirect
   /// results. Provides convenient access to the underlying PILFunctionType.
   ArrayRef<PILParameterInfo> getParameters() const {
      return funcTy->getParameters();
   }

   struct PILParameterTypeFunc;

   // Gratuitous template parameter is to delay instantiating `mapped_iterator`
   // on the incomplete type PILParameterTypeFunc.
   template<bool _ = false>
   using PILParameterTypeIter = typename
   delay_template_expansion<_, llvm::mapped_iterator,
      const PILParameterInfo *, PILParameterTypeFunc>::type;

   template<bool _ = false>
   using PILParameterTypeRange = iterator_range<PILParameterTypeIter<_>>;

   /// Return a range of PILTypes for each function parameter, not including
   /// indirect results.
   template<bool _ = false>
   PILParameterTypeRange<_> getParameterPILTypes() const;

   //===--------------------------------------------------------------------===//
   // PIL yield types.
   //===--------------------------------------------------------------------===//

   unsigned getNumYields() const { return funcTy->getNumYields(); }

   ArrayRef<PILYieldInfo> getYields() const {
      return funcTy->getYields();
   }

   template<bool _ = false>
   using PILYieldTypeIter = typename
   delay_template_expansion<_, llvm::mapped_iterator,
      const PILYieldInfo *, PILParameterTypeFunc>::type;
   template<bool _ = false>
   using PILYieldTypeRange = iterator_range<PILYieldTypeIter<_>>;

   template<bool _ = false>
   PILYieldTypeRange<_> getYieldPILTypes() const;

   PILYieldInfo getYieldInfoForOperandIndex(unsigned opIndex) const {
      return getYields()[opIndex];
   }

   //===--------------------------------------------------------------------===//
   // PILArgument API, including indirect results and parameters.
   //
   // The argument indices below relate to full applies in which the caller and
   // callee indices match. Partial apply indices are shifted on the caller
   // side. See ApplySite::getCalleeArgIndexOfFirstAppliedArg().
   //===--------------------------------------------------------------------===//

   unsigned getPILArgIndexOfFirstIndirectResult() const { return 0; }

   unsigned getPILArgIndexOfFirstParam() const {
      return getNumIndirectPILResults();
   }

   /// Get the index into formal indirect results corresponding to the given PIL
   /// indirect result argument index.
   unsigned getIndirectFormalResultIndexForPILArg(unsigned argIdx) const {
      assert(argIdx <= getNumIndirectPILResults());
      unsigned curArgIdx = 0;
      unsigned formalIdx = 0;
      for (auto formalResult : funcTy->getIndirectFormalResults()) {
         if (isPILIndirect(formalResult)) {
            if (curArgIdx == argIdx)
               return formalIdx;
            ++curArgIdx;
         }
         ++formalIdx;
      }
      llvm_unreachable("missing indirect formal result for PIL argument.");
   }

   /// Get the total number of arguments for a full apply in PIL of
   /// this function type. This is also the total number of PILArguments
   /// in the entry block.
   unsigned getNumPILArguments() const {
      return getNumIndirectPILResults() + funcTy->getNumParameters();
   }

   PILParameterInfo getParamInfoForPILArg(unsigned index) const {
      assert(index >= getNumIndirectPILResults()
             && index <= getNumPILArguments());
      return funcTy->getParameters()[index - getNumIndirectPILResults()];
   }

   /// Return the PIL argument convention of apply/entry argument at
   /// the given argument index.
   PILArgumentConvention getPILArgumentConvention(unsigned index) const;
   // See PILArgument.h.

   /// Return the PIL type of the apply/entry argument at the given index.
   PILType getPILArgumentType(unsigned index) const;
};

struct PILFunctionConventions::PILResultTypeFunc {
   PILFunctionConventions silConv;
   PILResultTypeFunc(const PILFunctionConventions &silConv)
      : silConv(silConv) {}

   PILType operator()(PILResultInfo result) const {
      return silConv.getPILType(result);
   }
};

template<bool _>
PILFunctionConventions::IndirectPILResultTypeRange<_>
PILFunctionConventions::getIndirectPILResultTypes() const {
   return llvm::map_range(getIndirectPILResults(), PILResultTypeFunc(*this));
}

template<bool _>
PILFunctionConventions::DirectPILResultTypeRange<_>
PILFunctionConventions::getDirectPILResultTypes() const {
   return llvm::map_range(getDirectPILResults(), PILResultTypeFunc(*this));
}

struct PILFunctionConventions::PILParameterTypeFunc {
   PILFunctionConventions silConv;
   PILParameterTypeFunc(const PILFunctionConventions &silConv)
      : silConv(silConv) {}

   PILType operator()(PILParameterInfo param) const {
      return silConv.getPILType(param);
   }
};

template<bool _>
PILFunctionConventions::PILParameterTypeRange<_>
PILFunctionConventions::getParameterPILTypes() const {
   return llvm::map_range(funcTy->getParameters(),
                          PILParameterTypeFunc(*this));
}

template<bool _>
PILFunctionConventions::PILYieldTypeRange<_>
PILFunctionConventions::getYieldPILTypes() const {
   return llvm::map_range(funcTy->getYields(),
                          PILParameterTypeFunc(*this));
}

inline PILType
PILFunctionConventions::getPILArgumentType(unsigned index) const {
   assert(index <= getNumPILArguments());
   if (index < getNumIndirectPILResults()) {
      return *std::next(getIndirectPILResultTypes().begin(), index);
   }
   return getPILType(
      funcTy->getParameters()[index - getNumIndirectPILResults()]);
}

inline PILFunctionConventions
PILModuleConventions::getFunctionConventions(CanPILFunctionType funcTy) {
   return PILFunctionConventions(funcTy, *this);
}

inline bool PILModuleConventions::isIndirectPILParam(PILParameterInfo param,
                                                     bool loweredAddresses) {
   switch (param.getConvention()) {
      case ParameterConvention::Direct_Unowned:
      case ParameterConvention::Direct_Guaranteed:
      case ParameterConvention::Direct_Owned:
         return false;

      case ParameterConvention::Indirect_In:
      case ParameterConvention::Indirect_In_Constant:
      case ParameterConvention::Indirect_In_Guaranteed:
         return (loweredAddresses ||
                 param.getInterfaceType()->isOpenedExistentialWithError());
      case ParameterConvention::Indirect_Inout:
      case ParameterConvention::Indirect_InoutAliasable:
         return true;
   }
   llvm_unreachable("covered switch isn't covered?!");
}

inline bool PILModuleConventions::isIndirectPILYield(PILYieldInfo yield,
                                                     bool loweredAddresses) {
   return isIndirectPILParam(yield, loweredAddresses);
}

inline bool PILModuleConventions::isIndirectPILResult(PILResultInfo result,
                                                      bool loweredAddresses) {
   switch (result.getConvention()) {
      case ResultConvention::Indirect:
         return (loweredAddresses ||
                 result.getInterfaceType()->isOpenedExistentialWithError());
      case ResultConvention::Owned:
      case ResultConvention::Unowned:
      case ResultConvention::UnownedInnerPointer:
      case ResultConvention::Autoreleased:
         return false;
   }

   llvm_unreachable("Unhandled ResultConvention in switch.");
}

inline PILType PILModuleConventions::getPILParamInterfaceType(
   PILParameterInfo param,
   bool loweredAddresses) {
   return PILModuleConventions::isIndirectPILParam(param,loweredAddresses)
          ? PILType::getPrimitiveAddressType(param.getInterfaceType())
          : PILType::getPrimitiveObjectType(param.getInterfaceType());
}

inline PILType PILModuleConventions::getPILYieldInterfaceType(
   PILYieldInfo yield,
   bool loweredAddresses) {
   return getPILParamInterfaceType(yield, loweredAddresses);
}

inline PILType PILModuleConventions::getPILResultInterfaceType(
   PILResultInfo result,
   bool loweredAddresses) {
   return PILModuleConventions::isIndirectPILResult(result, loweredAddresses)
          ? PILType::getPrimitiveAddressType(result.getInterfaceType())
          : PILType::getPrimitiveObjectType(result.getInterfaceType());
}

using namespace polar;

inline PILType
PILParameterInfo::getPILStorageInterfaceType() const {
   return PILModuleConventions::getPILParamInterfaceType(*this, true);
}

inline PILType
PILResultInfo::getPILStorageInterfaceType() const {
   return PILModuleConventions::getPILResultInterfaceType(*this, true);
}

inline PILType
PILParameterInfo::getPILStorageType(PILModule &M,
                                    const PILFunctionType *funcTy) const {
   return funcTy->substInterfaceType(M, getPILStorageInterfaceType());
}

inline PILType
PILResultInfo::getPILStorageType(PILModule &M,
                                 const PILFunctionType *funcTy) const {
   return funcTy->substInterfaceType(M, getPILStorageInterfaceType());
}

} // polar

#endif
