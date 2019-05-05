//===--- ParameterList.h - Functions & closures parameter lists -*- context++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/30.
//===----------------------------------------------------------------------===//
//
// This file defines the ParameterList class and support logic.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PARAMETERLIST_H
#define POLARPHP_AST_PARAMETERLIST_H

#include "polarphp/ast/Decl.h"
#include "polarphp/basic/adt/OptionSet.h"
#include "polarphp/utils/TrailingObjects.h"

namespace polar::ast {

using polar::utils::TrailingObjects;

/// This describes a list of parameters.  Each parameter descriptor is tail
/// allocated onto this list.
//class alignas(ParamDecl *) ParameterList final :
class ParameterList final :
      private TrailingObjects<ParameterList, ParamDecl *>
{
private:
   friend class TrailingObjects;
   void *operator new(size_t bytes) throw() = delete;
   void operator delete(void *data) throw() = delete;
   void *operator new(size_t bytes, void *mem) throw() = delete;
   void *operator new(size_t bytes, AstContext &context,
                      unsigned alignment = 8);
   SourceLoc lParenLoc;
   SourceLoc rParenLoc;
   size_t numParameters;

   ParameterList(SourceLoc lParenLoc, size_t numParameters, SourceLoc rParenLoc)
      : lParenLoc(lParenLoc),
        rParenLoc(rParenLoc),
        numParameters(numParameters)
   {}

   void operator=(const ParameterList&) = delete;
public:
   /// Create a parameter list with the specified parameters.
   static ParameterList *create(const AstContext &context, SourceLoc lParenLoc,
                                ArrayRef<ParamDecl*> params,
                                SourceLoc rParenLoc);

   /// Create a parameter list with the specified parameters, with no location
   /// info for the parens.
   static ParameterList *create(const AstContext &context,
                                ArrayRef<ParamDecl*> params)
   {
      return create(context, SourceLoc(), params, SourceLoc());
   }

   /// Create an empty parameter list.
   static ParameterList *createEmpty(const AstContext &context,
                                     SourceLoc lParenLoc = SourceLoc(),
                                     SourceLoc rParenLoc = SourceLoc())
   {
      return create(context, lParenLoc, {}, rParenLoc);
   }

   /// Create a parameter list for a single parameter lacking location info.
   static ParameterList *createWithoutLoc(ParamDecl *decl)
   {
      return create(decl->getAstContext(), decl);
   }

   SourceLoc getLParenLoc() const
   {
      return lParenLoc;
   }

   SourceLoc getRParenLoc() const
   {
      return rParenLoc;
   }

   typedef MutableArrayRef<ParamDecl *>::iterator iterator;
   typedef ArrayRef<ParamDecl *>::iterator const_iterator;
   iterator begin()
   {
      return getArray().begin();
   }

   iterator end()
   {
      return getArray().end();
   }

   const_iterator begin() const
   {
      return getArray().begin();
   }

   const_iterator end() const
   {
      return getArray().end();
   }

   MutableArrayRef<ParamDecl*> getArray()
   {
      return {getTrailingObjects<ParamDecl*>(), numParameters};
   }

   ArrayRef<ParamDecl*> getArray() const
   {
      return {getTrailingObjects<ParamDecl*>(), numParameters};
   }

   size_t size() const
   {
      return numParameters;
   }

   const ParamDecl *get(unsigned i) const
   {
      return getArray()[i];
   }

   ParamDecl *&get(unsigned i)
   {
      return getArray()[i];
   }

   const ParamDecl *operator[](unsigned i) const
   {
      return get(i);
   }

   ParamDecl *&operator[](unsigned i)
   {
      return get(i);
   }

   /// Change the DeclContext of any contained parameters to the specified
   /// DeclContext.
   void setDeclContextOfParamDecls(DeclContext *declContext);


   /// Flags used to indicate how ParameterList cloning should operate.
   enum CloneFlags
   {
      /// The cloned ParamDecls should be marked implicit.
      Implicit = 0x01,
      /// The cloned pattern is for an inherited constructor; mark default
      /// arguments as inherited, and mark unnamed arguments as named.
      Inherited = 0x02,
      /// The cloned pattern will strip type information.
      WithoutTypes = 0x04,
   };

   friend OptionSet<CloneFlags> operator|(CloneFlags flag1, CloneFlags flag2)
   {
      return OptionSet<CloneFlags>(flag1) | flag2;
   }

   /// Make a duplicate copy of this parameter list.  This allocates copies of
   /// the ParamDecls, so they can be reparented into a new DeclContext.
   ParameterList *clone(const AstContext &context,
                        OptionSet<CloneFlags> options = std::nullopt) const;

   /// Return a list of function parameters for this parameter list,
   /// based on the interface types of the parameters in this list.
//   void getParams(SmallVectorImpl<AnyFunctionType::Param> &params) const;

   /// Return a list of function parameters for this parameter list,
   /// based on types provided by a callback.
//   void getParams(SmallVectorImpl<AnyFunctionType::Param> &params,
//                  FunctionRef<Type(ParamDecl *)> getType) const;


   /// Return the full source range of this parameter.
   SourceRange getSourceRange() const;
   SourceLoc getStartLoc() const
   {
      return getSourceRange().m_start;
   }

   SourceLoc getEndLoc() const
   {
      return getSourceRange().m_end;
   }

   void dump() const;
   void dump(RawOutStream &outStream, unsigned indent = 0) const;

   //  void print(RawOutStream &outStream) const;
};

} // polar::ast

#endif // POLARPHP_AST_PARAMETERLIST_H

