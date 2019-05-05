//===--- Attr.cpp - Swift Language Attr ASTs ------------------------------===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
//  This file implements routines relating to declaration attributes.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/Attr.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstPrinter.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/basic/adt/StringSwitch.h"

//#define DECL_ATTR(_, Id, ...) \
//   static_assert(IsTriviallyDestructible<Id##Attr>::value, \
//   "Attrs are BumpPtrAllocated; the destructor is never called");
//#include "polarphp/ast/AttrDefs.h"
//static_assert(IsTriviallyDestructible<DeclAttributes>::value,
//              "DeclAttributes are BumpPtrAllocated; the d'tor is never called");
//static_assert(IsTriviallyDestructible<TypeAttributes>::value,
//              "TypeAttributes are BumpPtrAllocated; the d'tor is never called");

namespace polar::ast {

//// Only allow allocation of attributes using the allocator in AstContext.
void *AttributeBase::operator new(size_t Bytes, AstContext &C,
                                  unsigned Alignment)
{
}

StringRef get_access_level_spelling(AccessLevel value)
{
   polar_unreachable("Unhandled AccessLevel in switch.");
}

/// Given a name like "autoclosure", return the type attribute ID that
/// corresponds to it.  This returns TAK_Count on failure.
///
TypeAttrKind TypeAttributes::getAttrKindFromString(StringRef Str)
{
}

/// Return the name (like "autoclosure") for an attribute ID.
const char *TypeAttributes::getAttrName(TypeAttrKind kind)
{
}


///// Given a name like "inline", return the decl attribute ID that corresponds
///// to it.  Note that this is a many-to-one mapping, and that the identifier
///// passed in may only be the first portion of the attribute (e.g. in the case
///// of the 'unowned(unsafe)' attribute, the string passed in is 'unowned'.
/////
///// Also note that this recognizes both attributes like '@inline' (with no @)
///// and decl modifiers like 'final'.  This returns DAK_Count on failure.
/////
DeclAttrKind DeclAttribute::getAttrKindFromString(StringRef Str)
{
}

/// Returns true if this attribute can appear on the specified decl.
bool DeclAttribute::canAttributeAppearOnDecl(DeclAttrKind DK, const Decl *D)
{

}

bool DeclAttribute::canAttributeAppearOnDeclKind(DeclAttrKind DAK, DeclKind DK)
{
}


const AvailableAttr *DeclAttributes::getUnavailable(
      const AstContext &ctx) const
{

}

const AvailableAttr *
DeclAttributes::getDeprecated(const AstContext &ctx) const
{
}

void DeclAttributes::dump(const Decl *D) const
{

}

///// Returns true if the attribute can be presented as a short form available
///// attribute (e.g., as @available(iOS 8.0, *). The presentation requires an
///// introduction version and does not support deprecation, obsoletion, or
///// messages.
POLAR_READONLY
static bool isShortAvailable(const DeclAttribute *DA)
{
   return true;
}

///// Print the short-form @available() attribute for an array of long-form
///// AvailableAttrs that can be represented in the short form.
///// For example, for:
/////   @available(OSX, introduced: 10.10)
/////   @available(iOS, introduced: 8.0)
///// this will print:
/////   @available(OSX 10.10, iOS 8.0, *)
static void printShortFormAvailable(ArrayRef<const DeclAttribute *> Attrs,
                                    AstPrinter &Printer,
                                    const PrintOptions &Options)
{
}

void DeclAttributes::print(AstPrinter &Printer, const PrintOptions &Options,
                           const Decl *D) const
{
}

SourceLoc DeclAttributes::getStartLoc(bool forModifiers) const
{

}

bool DeclAttribute::printImpl(AstPrinter &Printer, const PrintOptions &Options,
                              const Decl *D) const
{
   return true;
}

void DeclAttribute::print(AstPrinter &Printer, const PrintOptions &Options,
                          const Decl *D) const
{


}

void DeclAttribute::print(RawOutStream &OS, const Decl *D) const
{

}

uint64_t DeclAttribute::getOptions(DeclAttrKind DK)
{
}

StringRef DeclAttribute::getAttrName() const
{

   polar_unreachable("bad DeclAttrKind");
}

} // polar::ast
