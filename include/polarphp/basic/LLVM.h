// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.
//===----------------------------------------------------------------------===//
//
// This file forward declares and imports various common LLVM datatypes that
// swift wants to use unqualified.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_LLVM_H
#define POLARPHP_BASIC_LLVM_H

#include "llvm/Support/Casting.h"
#include "llvm/ADT/None.h"

namespace llvm {

// Containers.
class StringRef;
class StringSaver;
class StringLiteral;
class Twine;
template <typename T> class SmallPtrSetImpl;
template <typename T, unsigned N> class SmallPtrSet;
template <typename T> class SmallVectorImpl;
template <typename T, unsigned N> class SmallVector;
template <unsigned N> class SmallString;
template <typename T, unsigned N> class SmallSetVector;
template<typename T> class ArrayRef;
template<typename T> class MutableArrayRef;
template<typename T> class TinyPtrVector;
template<typename T> class Optional;
template <typename... PTs> class PointerUnion;
template <typename PT1, typename PT2, typename PT3>
using PointerUnion3 = PointerUnion<PT1, PT2, PT3>;
/// A pointer union of four pointer types. See documentation for PointerUnion
/// for usage.
template <typename PT1, typename PT2, typename PT3, typename PT4>
using PointerUnion4 = PointerUnion<PT1, PT2, PT3, PT4>;

class SmallBitVector;

// Other common classes.
class raw_ostream;
class APInt;
class APFloat;
template <typename Fn> class function_ref;

} // llvm

namespace polar {

// Casting operators.
using llvm::isa;
using llvm::cast;
using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::cast_or_null;

// Containers.
using llvm::ArrayRef;
using llvm::MutableArrayRef;
using llvm::None;
using llvm::Optional;
using llvm::PointerUnion;
using llvm::PointerUnion3;
using llvm::SmallBitVector;
using llvm::SmallPtrSet;
using llvm::SmallPtrSetImpl;
using llvm::SmallSetVector;
using llvm::SmallString;
using llvm::SmallVector;
using llvm::SmallVectorImpl;
using llvm::StringLiteral;
using llvm::StringRef;
using llvm::TinyPtrVector;
using llvm::Twine;

// Other common classes.
using llvm::APFloat;
using llvm::APInt;
using llvm::function_ref;
using llvm::NoneType;
using llvm::raw_ostream;

} // polar

#endif // POLARPHP_BASIC_LLVM_H
