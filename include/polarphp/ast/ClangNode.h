//===--- ClangNode.h - How Swift tracks imported Clang entities -*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_CLANG_NODE_H
#define POLARPHP_AST_CLANG_NODE_H

#include "llvm/ADT/PointerUnion.h"

namespace clang {
class Decl;
class MacroInfo;
class ModuleMacro;
class Module;
class SourceLocation;
class SourceRange;
} // clang

namespace polar::ast {
namespace internal {
/// A wrapper to avoid having to import Clang headers. We can't just
/// forward-declare their PointerLikeTypeTraits because we don't own
/// the types.
template <typename T>
struct ClangNodeBox
{
   const T * const value;

   ClangNodeBox()
      : value{nullptr} {}
   /*implicit*/ ClangNodeBox(const T *V)
      : value(V) {}

   explicit operator bool() const
   {
      return value;
   }
};
} // internal

/// Represents a clang declaration, macro, or module. A macro definition
/// imported from a module is recorded as the ModuleMacro, and a macro
/// defined locally is represented by the MacroInfo.
class ClangNode
{
   template <typename T>
   using Box = internal::ClangNodeBox<T>;

   llvm::PointerUnion4<Box<clang::Decl>, Box<clang::MacroInfo>,
   Box<clang::ModuleMacro>, Box<clang::Module>> m_ptr;

public:
   ClangNode() = default;
   ClangNode(const clang::Decl *decl)
      : m_ptr(decl) {}

   ClangNode(const clang::MacroInfo *macroInfo)
      : m_ptr(macroInfo) {}

   ClangNode(const clang::ModuleMacro *moduleMacro)
      : m_ptr(moduleMacro) {}

   ClangNode(const clang::Module *module)
      : m_ptr(module) {}

   bool isNull() const
   {
      return m_ptr.isNull();
   }

   explicit operator bool() const
   {
      return !isNull();
   }

   const clang::Decl *getAsDecl() const
   {
      return m_ptr.dyn_cast<Box<clang::Decl>>().value;
   }

   const clang::MacroInfo *getAsMacroInfo() const
   {
      return m_ptr.dyn_cast<Box<clang::MacroInfo>>().value;
   }

   const clang::ModuleMacro *getAsModuleMacro() const
   {
      return m_ptr.dyn_cast<Box<clang::ModuleMacro>>().value;
   }

   const clang::Module *getAsModule() const
   {
      return m_ptr.dyn_cast<Box<clang::Module>>().value;
   }

   const clang::Decl *castAsDecl() const
   {
      return m_ptr.get<Box<clang::Decl>>().value;
   }

   const clang::MacroInfo *castAsMacroInfo() const
   {
      return m_ptr.get<Box<clang::MacroInfo>>().value;
   }

   const clang::ModuleMacro *castAsModuleMacro() const
   {
      return m_ptr.get<Box<clang::ModuleMacro>>().value;
   }

   const clang::Module *castAsModule() const
   {
      return m_ptr.get<Box<clang::Module>>().value;
   }

   // Get the MacroInfo for a local definition, one imported from a
   // ModuleMacro, or null if it's neither.
   const clang::MacroInfo *getAsMacro() const;

   /// Returns the module either the one wrapped directly, the one from a
   /// clang::ImportDecl or null if it's neither.
   const clang::Module *getClangModule() const;

   clang::SourceLocation getLocation() const;
   clang::SourceRange getSourceRange() const;

   void *getOpaqueValue() const
   {
      return m_ptr.getOpaqueValue();
   }

   static inline ClangNode getFromOpaqueValue(void *VP)
   {
      ClangNode N;
      N.m_ptr = decltype(m_ptr)::getFromOpaqueValue(VP);
      return N;
   }
};

} // polar::ast

#endif // POLARPHP_AST_CLANG_NODE_H
