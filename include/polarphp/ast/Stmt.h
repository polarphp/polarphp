//===--- Stmt.h - Swift Language Statement ASTs -----------------*- context++ -*-===//
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the Stmt class and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_STMT_H
#define POLARPHP_AST_STMT_H

#include "polarphp/ast/Availability.h"
#include "polarphp/ast/AvailabilitySpec.h"
#include "polarphp/ast/AstNode.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/NullablePtr.h"
#include "polarphp/utils/TrailingObjects.h"

namespace polar::ast {

class AstContext;
class AstWalker;
class Decl;
class Expr;
class FuncDecl;
class VarDecl;

enum class StmtKind
{
#define STMT(ID, PARENT) ID,
#define LAST_STMT(ID) Last_Stmt = ID,
#define STMT_RANGE(Id, FirstId, LastId) \
   First_##Id##Stmt = FirstId, Last_##Id##Stmt = LastId,
#include "polarphp/ast/StmtNodesDefs.h"
};
enum : unsigned
{
   NumStmtKindBits =
   polar::basic::count_bits_used(static_cast<unsigned>(StmtKind::Last_Stmt))
};

/// Stmt - Base class for all statements in swift.
class alignas(8) Stmt
{
   Stmt(const Stmt&) = delete;
   Stmt& operator=(const Stmt&) = delete;

   protected:
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(
               Stmt, bitmax(NumStmtKindBits,8) + 1,
               /// kind - The subclass of Stmt that this is.
               kind : bitmax(NumStmtKindBits,8),

               /// implicit - Whether this statement is implicit.
               implicit : 1
               );

      POLAR_INLINE_BITFIELD_FULL(
               BraceStmt, Stmt, 32,
               : NumPadBits,
               NumElements : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               CaseStmt, Stmt, 32,
               : NumPadBits,
               NumPatterns : 32
               );

      POLAR_INLINE_BITFIELD_EMPTY(
               LabeledStmt, Stmt);

      POLAR_INLINE_BITFIELD_FULL(
               DoCatchStmt, LabeledStmt, 32,
               : NumPadBits,
               NumCatches : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               SwitchStmt, LabeledStmt, 32,
               : NumPadBits,
               caseCount : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               YieldStmt, Stmt, 32,
               : NumPadBits,
               NumYields : 32
               );

   } bits;

   /// Return the given value for the 'implicit' flag if present, or if std::nullopt,
   /// return true if the location is invalid.
   static bool getDefaultImplicitFlag(std::optional<bool> implicit, SourceLoc keyLoc)
   {
      return implicit.has_value() ? *implicit : keyLoc.isInvalid();
   }

   public:
   Stmt(StmtKind kind, bool implicit)
   {
      bits.OpaqueBits = 0;
      bits.Stmt.kind = static_cast<unsigned>(kind);
      bits.Stmt.implicit = implicit;
   }

   StmtKind getKind() const
   {
      return StmtKind(bits.Stmt.kind);
   }

   /// Retrieve the name of the given statement kind.
   ///
   /// This name should only be used for debugging dumps and other
   /// developer aids, and should never be part of a diagnostic or exposed
   /// to the user of the compiler in any way.
   static StringRef getKindName(StmtKind kind);

   /// Return the location of the start of the statement.
   SourceLoc getStartLoc() const;

   /// Return the location of the end of the statement.
   SourceLoc getEndLoc() const;

   SourceRange getSourceRange() const;
   SourceLoc TrailingSemiLoc;

   /// isImplicit - Determines whether this statement was implicitly-generated,
   /// rather than explicitly written in the AST.
   bool isImplicit() const
   {
      return bits.Stmt.implicit;
   }

   /// walk - This recursively walks the AST rooted at this statement.
   Stmt *walk(AstWalker &walker);
   Stmt *walk(AstWalker &&walker)
   {
      return walk(walker);
   }

   POLAR_ATTRIBUTE_DEPRECATED(
            void dump() const POLAR_ATTRIBUTE_USED,
            "only for use within the debugger");
   void dump(RawOutStream &outStream, const AstContext *context = nullptr, unsigned indent = 0) const;

   // Only allow allocation of Exprs using the allocator in AstContext
   // or by doing a placement new.
   void *operator new(size_t bytes, AstContext &context,
                      unsigned alignment = alignof(Stmt));

   // Make vanilla new/delete illegal for Stmts.
   void *operator new(size_t bytes) throw() = delete;
   void operator delete(void *data) throw() = delete;
   void *operator new(size_t bytes, void *mem) throw() = delete;
};

/// BraceStmt - A brace enclosed sequence of expressions, stmts, or decls, like
/// { var x = 10; print(10) }.
class BraceStmt final : public Stmt,
      private polar::utils::TrailingObjects<BraceStmt, AstNode>
{
public:
   static BraceStmt *create(AstContext &ctx, SourceLoc lbloc,
                            ArrayRef<AstNode> elements,
                            SourceLoc rbloc,
                            std::optional<bool> implicit = std::nullopt);

   SourceLoc getLBraceLoc() const
   {
      return m_lbLoc;
   }

   SourceLoc getRBraceLoc() const
   {
      return m_rbLoc;
   }

   SourceRange getSourceRange() const
   {
      return SourceRange(m_lbLoc, m_rbLoc);
   }

   unsigned getNumElements() const
   {
      return bits.BraceStmt.NumElements;
   }

   AstNode getElement(unsigned i) const
   {
      return getElements()[i];
   }

   void setElement(unsigned i, AstNode node)
   {
      getElements()[i] = node;
   }

   /// The elements contained within the BraceStmt.
   MutableArrayRef<AstNode> getElements()
   {
      return {getTrailingObjects<AstNode>(), bits.BraceStmt.NumElements};
   }

   /// The elements contained within the BraceStmt (const version).
   ArrayRef<AstNode> getElements() const
   {
      return {getTrailingObjects<AstNode>(), bits.BraceStmt.NumElements};
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Brace;
   }

private:
   friend class TrailingObjects;
   SourceLoc m_lbLoc;
   SourceLoc m_rbLoc;
   BraceStmt(SourceLoc lbloc, ArrayRef<AstNode> elements, SourceLoc rbloc,
             std::optional<bool> implicit);

};

/// ReturnStmt - A return statement.  The result is optional; "return" without
/// an expression is semantically equivalent to "return ()".
///    return 42
class ReturnStmt : public Stmt
{
public:
   ReturnStmt(SourceLoc returnLoc, Expr *result,
              std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Return, getDefaultImplicitFlag(implicit, returnLoc)),
        m_returnLoc(returnLoc), m_result(result)
   {}

   SourceLoc getReturnLoc() const
   {
      return m_returnLoc;
   }

   SourceLoc getStartLoc() const;
   SourceLoc getEndLoc() const;

   bool hasResult() const
   {
      return m_result != 0;
   }

   Expr *getResult() const
   {
      assert(m_result && "ReturnStmt doesn't have a result");
      return m_result;
   }

   void setResult(Expr *e)
   {
      m_result = e;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Return;
   }
private:
   SourceLoc m_returnLoc;
   Expr *m_result;
};

/// YieldStmt - A yield statement.  The yield-values sequence is not optional,
/// but the parentheses are.
///    yield 42
class YieldStmt final
      : public Stmt, private polar::utils::TrailingObjects<YieldStmt, Expr*>
{
public:
   static YieldStmt *create(const AstContext &ctx, SourceLoc yieldLoc,
                            SourceLoc lp, ArrayRef<Expr*> yields, SourceLoc rp,
                            std::optional<bool> implicit = std::nullopt);

   SourceLoc getYieldLoc() const
   {
      return m_yieldLoc;
   }

   SourceLoc getLParenLoc() const { return m_lpLoc; }
   SourceLoc getRParenLoc() const { return m_rpLoc; }

   SourceLoc getStartLoc() const { return m_yieldLoc; }
   SourceLoc getEndLoc() const;

   ArrayRef<Expr*> getYields() const
   {
      return {getTrailingObjects<Expr*>(), bits.YieldStmt.NumYields};
   }

   MutableArrayRef<Expr*> getMutableYields()
   {
      return {getTrailingObjects<Expr*>(), bits.YieldStmt.NumYields};
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Yield;
   }
private:
   friend class TrailingObjects;
   SourceLoc m_yieldLoc;
   SourceLoc m_lpLoc;
   SourceLoc m_rpLoc;
   YieldStmt(SourceLoc yieldLoc, SourceLoc lpLoc, ArrayRef<Expr *> yields,
             SourceLoc rpLoc, std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Yield, getDefaultImplicitFlag(implicit, yieldLoc)),
        m_yieldLoc(yieldLoc), m_lpLoc(lpLoc), m_rpLoc(rpLoc)
   {
      bits.YieldStmt.NumYields = yields.size();
      memcpy(getMutableYields().data(), yields.data(),
             yields.size() * sizeof(Expr*));
   }
};

/// DeferStmt - A 'defer' statement.  This runs the substatement it contains
/// when the enclosing scope is exited.
///
///    defer { cleanUp() }
///
/// The AST representation for a defer statement is a bit weird.  We model this
/// as if they wrote:
///
///    func tmpClosure() { body }
///    tmpClosure()   // This is emitted on each path that needs to run this.
///
/// As such, the body of the 'defer' is actually type checked within the
/// closure's DeclContext.  We do this because of unfortunateness in SILGen,
/// some expressions (e.g. OpenExistentialExpr) cannot be multiply emitted in a
/// composable way.  When this gets fixed, patches r27767 and r27768 can be
/// reverted to go back to the simpler and more obvious representation.
///
class DeferStmt : public Stmt
{
public:
   DeferStmt(SourceLoc deferLoc,
             FuncDecl *tempDecl, Expr *callExpr)
      : Stmt(StmtKind::Defer, /*implicit*/false),
        m_deferLoc(deferLoc), m_tempDecl(tempDecl),
        m_callExpr(callExpr)
   {}

   SourceLoc getDeferLoc() const
   {
      return m_deferLoc;
   }

   SourceLoc getStartLoc() const
   {
      return m_deferLoc;
   }

   SourceLoc getEndLoc() const;

   FuncDecl *getTempDecl() const
   {
      return m_tempDecl;
   }

   Expr *getCallExpr() const
   {
      return m_callExpr;
   }

   void setCallExpr(Expr *expr)
   {
      m_callExpr = expr;
   }

   /// Dig the original user's body of the defer out for AST fidelity.
   BraceStmt *getBodyAsWritten() const;

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Defer;
   }
private:
   SourceLoc m_deferLoc;
   /// This is the bound temp function.
   FuncDecl *m_tempDecl;
   /// This is the invocation of the closure, which is to be emitted on any error
   /// paths.
   Expr *m_callExpr;
};


/// An expression that guards execution based on whether the run-time
/// configuration supports a given API, e.g.,
/// #available(OSX >= 10.9, iOS >= 7.0).
class alignas(8) PoundAvailableInfo final :
   private polar::utils::TrailingObjects<PoundAvailableInfo, AvailabilitySpec *>
{
              public:
              static PoundAvailableInfo *create(AstContext &ctx, SourceLoc poundLoc,
                                                ArrayRef<AvailabilitySpec *> queries,
                                                SourceLoc rparenLoc);

ArrayRef<AvailabilitySpec *> getQueries() const
{
   return polar::basic::make_array_ref(getTrailingObjects<AvailabilitySpec *>(),
                                       m_numQueries);
}

SourceLoc getStartLoc() const
{
   return m_poundLoc;
}

SourceLoc getEndLoc() const;
SourceLoc getLoc() const
{
   return m_poundLoc;
}

SourceRange getSourceRange() const
{
   return SourceRange(getStartLoc(),getEndLoc());
}

const VersionRange &getAvailableRange() const
{
   return m_availableRange;
}

void setAvailableRange(const VersionRange &range)
{
   m_availableRange = range;
}

private:
friend class TrailingObjects;
SourceLoc m_poundLoc;
SourceLoc m_rparenLoc;

// The number of queries tail allocated after this object.
unsigned m_numQueries;

/// The version range when this query will return true. This value is
/// filled in by Sema.
VersionRange m_availableRange;

PoundAvailableInfo(SourceLoc poundLoc, ArrayRef<AvailabilitySpec *> queries,
                   SourceLoc rparenLoc)
   : m_poundLoc(poundLoc),
     m_rparenLoc(rparenLoc),
     m_numQueries(queries.size()),
     m_availableRange(VersionRange::empty())
{
   std::uninitialized_copy(queries.begin(), queries.end(),
                           getTrailingObjects<AvailabilitySpec *>());
}
};


/// This represents an entry in an "if" or "while" condition.  Pattern bindings
/// can bind any number of names in the pattern binding decl, and may have an
/// associated where clause.  When "if let" is involved, an arbitrary number of
/// pattern bindings and conditional expressions are permitted, e.g.:
///
///   if let x = ..., y = ... where x > y,
///      let z = ...
/// which would be represented as four StmtConditionElement entries, one for
/// the "x" binding, one for the "y" binding, one for the where clause, one for
/// "z"'s binding.  A simple "if" statement is represented as a single binding.
///
class StmtConditionElement
{
public:
   StmtConditionElement()
   {}

   StmtConditionElement(SourceLoc introducerLoc, Pattern *hePattern,
                        Expr *init)
      : m_introducerLoc(m_introducerLoc), m_thePattern(m_thePattern),
        m_condInitOrAvailable(init)
   {}

   StmtConditionElement(Expr *cond)
      : m_condInitOrAvailable(cond)
   {}

   StmtConditionElement(PoundAvailableInfo *info)
      : m_condInitOrAvailable(info)
   {}

   SourceLoc getIntroducerLoc() const
   {
      return m_introducerLoc;
   }

   void setIntroducerLoc(SourceLoc loc)
   {
      m_introducerLoc = loc;
   }

   /// ConditionKind - This indicates the sort of condition this is.
   enum ConditionKind
   {
      CK_Boolean,
      CK_PatternBinding,
      CK_Availability
   };

   ConditionKind getKind() const
   {
      if (m_thePattern) {
         return CK_PatternBinding;
      }
      return m_condInitOrAvailable.is<Expr*>() ? CK_Boolean : CK_Availability;
   }

   /// Boolean Condition Accessors.
   Expr *getBooleanOrNull() const
   {
      return getKind() == CK_Boolean ? m_condInitOrAvailable.get<Expr*>() : nullptr;
   }

   Expr *getBoolean() const
   {
      assert(getKind() == CK_Boolean && "Not a condition");
      return m_condInitOrAvailable.get<Expr*>();
   }

   void setBoolean(Expr *expr)
   {
      assert(getKind() == CK_Boolean && "Not a condition");
      m_condInitOrAvailable = expr;
   }

   Expr *getInitializer() const
   {
      assert(getKind() == CK_PatternBinding && "Not a pattern binding condition");
      return m_condInitOrAvailable.get<Expr*>();
   }

   void setInitializer(Expr *expr)
   {
      assert(getKind() == CK_PatternBinding && "Not a pattern binding condition");
      m_condInitOrAvailable = expr;
   }

   // Availability Accessors
   PoundAvailableInfo *getAvailability() const
   {
      assert(getKind() == CK_Availability && "Not an #available condition");
      return m_condInitOrAvailable.get<PoundAvailableInfo*>();
   }

   void setAvailability(PoundAvailableInfo *info)
   {
      assert(getKind() == CK_Availability && "Not an #available condition");
      m_condInitOrAvailable = info;
   }

   SourceLoc getStartLoc() const;
   SourceLoc getEndLoc() const;
   SourceRange getSourceRange() const;

   /// Recursively walks the AST rooted at this statement condition element
   StmtConditionElement *walk(AstWalker &walker);
   StmtConditionElement *walk(AstWalker &&walker)
   {
      return walk(walker);
   }

private:
   /// If this is a pattern binding, it may be the first one in a declaration, in
   /// which case this is the location of the var/let/case keyword.  If this is
   /// the second pattern (e.g. for 'y' in "var x = ..., y = ...") then this
   /// location is invalid.
   SourceLoc m_introducerLoc;

   /// In a pattern binding, this is pattern being matched.  In the case of an
   /// "implicit optional" pattern, the OptionalSome pattern is explicitly added
   /// to this as an 'implicit' pattern.
   Pattern *m_thePattern = nullptr;

   /// This is either the boolean condition, the initializer for a pattern
   /// binding, or the #available information.
   PointerUnion<PoundAvailableInfo*, Expr *> m_condInitOrAvailable;
};

struct LabeledStmtInfo
{
   Identifier name;
   SourceLoc loc;

   // Evaluates to true if set.
   operator bool() const
   {
      return !name.empty();
   }
};

/// LabeledStmt - Common base class between the labeled statements (loops and
/// switch).
class LabeledStmt : public Stmt
{
public:
   LabeledStmt(StmtKind kind, bool implicit, LabeledStmtInfo labelInfo)
      : Stmt(kind, implicit),
        m_labelInfo(labelInfo)
   {}

   LabeledStmtInfo getLabelInfo() const
   {
      return m_labelInfo;
   }

   void setLabelInfo(LabeledStmtInfo label)
   {
      m_labelInfo = label;
   }

   /// Is this statement a valid target of "continue" if labeled?
   ///
   /// For the most part, non-looping constructs shouldn't be
   /// continue-able, but we threw in "do" as a sop.
   bool isPossibleContinueTarget() const;

   /// Is this statement a valid target of an unlabeled "break" or
   /// "continue"?
   ///
   /// The nice, consistent language rule is that unlabeled "break" and
   /// "continue" leave the innermost loop.  We have to include
   /// "switch" (for "break") for consistency with context: Swift doesn't
   /// require "break" to leave a switch case, but it's still way too
   /// similar to context's switch to allow different behavior for "break".
   bool requiresLabelOnJump() const;

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() >= StmtKind::First_LabeledStmt &&
            stmt->getKind() <= StmtKind::Last_LabeledStmt;
   }

private:
   LabeledStmtInfo m_labelInfo;

protected:
   SourceLoc getLabelLocOrKeywordLoc(SourceLoc loc) const
   {
      return m_labelInfo ? m_labelInfo.loc : loc;
   }
};


/// DoStmt - do statement, without any trailing clauses.
class DoStmt : public LabeledStmt
{
public:
   DoStmt(LabeledStmtInfo labelInfo, SourceLoc doLoc,
          Stmt *body, std::optional<bool> implicit = std::nullopt)
      : LabeledStmt(StmtKind::Do, getDefaultImplicitFlag(implicit, doLoc),
                    labelInfo),
        m_doLoc(doLoc),
        m_body(body)
   {}

   SourceLoc getDoLoc() const
   {
      return m_doLoc;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_doLoc);
   }

   SourceLoc getEndLoc() const
   {
      return m_body->getEndLoc();
   }

   Stmt *getBody() const
   {
      return m_body;
   }

   void setBody(Stmt *stmt)
   {
      m_body = stmt;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Do;
   }
private:
   SourceLoc m_doLoc;
   Stmt *m_body;
};

/// An individual 'catch' clause.
///
/// This isn't really an independent statement any more than CaseStmt
/// is; it's just a structural part of a DoCatchStmt.
class CatchStmt : public Stmt
{
public:
   CatchStmt(SourceLoc catchLoc, SourceLoc whereLoc,
             Expr *guardExpr, Stmt *body,
             std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Catch, getDefaultImplicitFlag(implicit, catchLoc)),
        m_catchLoc(catchLoc),
        m_whereLoc(whereLoc),
        m_errorPattern(nullptr),
        m_guardExpr(guardExpr),
        m_catchBody(body)
   {}

   SourceLoc getCatchLoc() const
   {
      return m_catchLoc;
   }

   /// The location of the 'where' keyword if there's a guard expression.
   SourceLoc getWhereLoc() const
   {
      return m_whereLoc;
   }

   SourceLoc getStartLoc() const
   {
      return m_catchLoc;
   }

   SourceLoc getEndLoc() const
   {
      return m_catchBody->getEndLoc();
   }

   Stmt *getBody() const
   {
      return m_catchBody;
   }

   void setBody(Stmt *body)
   {
      m_catchBody = body;
   }

   /// Is this catch clause "syntactically exhaustive"?
   bool isSyntacticallyExhaustive() const;

   /// Return the guard expression if present, or null if the catch has
   /// no guard.
   Expr *getGuardExpr() const
   {
      return m_guardExpr;
   }

   void setGuardExpr(Expr *guard)
   {
      m_guardExpr = guard;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Catch;
   }

private:
   SourceLoc m_catchLoc;
   SourceLoc m_whereLoc;
   Pattern *m_errorPattern;
   Expr *m_guardExpr;
   Stmt *m_catchBody;
};

/// DoCatchStmt - do statement with trailing 'catch' clauses.
class DoCatchStmt final : public LabeledStmt,
      private polar::utils::TrailingObjects<DoCatchStmt, CatchStmt *>
{
public:
   static DoCatchStmt *create(AstContext &ctx, LabeledStmtInfo labelInfo,
                              SourceLoc doLoc, Stmt *body,
                              ArrayRef<CatchStmt*> catches,
                              std::optional<bool> implicit = std::nullopt);

   SourceLoc getDoLoc() const
   {
      return m_doLoc;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_doLoc);
   }

   SourceLoc getEndLoc() const
   {
      return getCatches().back()->getEndLoc();
   }

   Stmt *getBody() const
   {
      return m_body;
   }

   void setBody(Stmt *stmt)
   {
      m_body = stmt;
   }

   ArrayRef<CatchStmt*> getCatches() const
   {
      return {getTrailingObjects<CatchStmt*>(), bits.DoCatchStmt.NumCatches};
   }

   MutableArrayRef<CatchStmt*> getMutableCatches()
   {
      return {getTrailingObjects<CatchStmt*>(), bits.DoCatchStmt.NumCatches};
   }

   /// Does this statement contain a syntactically exhaustive catch
   /// clause?
   ///
   /// Note that an exhaustive do/catch statement can still throw
   /// errors out of its catch block(s).
   bool isSyntacticallyExhaustive() const;

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::DoCatch;
   }

private:
   friend class TrailingObjects;
   SourceLoc m_doLoc;
   Stmt *m_body;

   DoCatchStmt(LabeledStmtInfo labelInfo, SourceLoc doLoc,
               Stmt *body, ArrayRef<CatchStmt*> catches,
               std::optional<bool> implicit)
      : LabeledStmt(StmtKind::DoCatch, getDefaultImplicitFlag(implicit, doLoc),
                    labelInfo), m_doLoc(doLoc), m_body(body)
   {
      bits.DoCatchStmt.NumCatches = catches.size();
      std::uninitialized_copy(catches.begin(), catches.end(),
                              getTrailingObjects<CatchStmt *>());
   }
};

/// Either an "if let" case or a simple boolean expression can appear as the
/// condition of an 'if' or 'while' statement.
using StmtCondition = MutableArrayRef<StmtConditionElement>;

/// This is the common base class between statements that can have labels, and
/// also have complex "if let" style conditions: 'if' and 'while'.
class LabeledConditionalStmt : public LabeledStmt
{
public:
   LabeledConditionalStmt(StmtKind kind, bool implicit,
                          LabeledStmtInfo labelInfo, StmtCondition cond)
      : LabeledStmt(kind, implicit, labelInfo)
   {
      setCond(cond);
   }

   StmtCondition getCond() const
   {
      return m_cond;
   }

   void setCond(StmtCondition e);

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() >= StmtKind::First_LabeledConditionalStmt &&
            stmt->getKind() <= StmtKind::Last_LabeledConditionalStmt;
   }

private:
   StmtCondition m_cond;
};


/// IfStmt - if/then/else statement.  If no 'else' is specified, then the
/// m_elseLoc location is not specified and the m_else statement is null. After
/// type-checking, the condition is of type Builtin.Int1.
class IfStmt : public LabeledConditionalStmt
{
public:
   IfStmt(LabeledStmtInfo labelInfo, SourceLoc ifLoc, StmtCondition cond,
          Stmt *then, SourceLoc elseLoc, Stmt *elseStmt,
          std::optional<bool> implicit = std::nullopt)
      : LabeledConditionalStmt(StmtKind::If,
                               getDefaultImplicitFlag(implicit, ifLoc),
                               labelInfo, cond),
        m_ifLoc(ifLoc),
        m_elseLoc(elseLoc),
        m_then(then),
        m_else(elseStmt)
   {}

   IfStmt(SourceLoc ifLoc, Expr *cond, Stmt *then, SourceLoc elseLoc,
          Stmt *elseStmt, std::optional<bool> implicit, AstContext &ctx);

   SourceLoc getIfLoc() const
   {
      return m_ifLoc;
   }

   SourceLoc getElseLoc() const
   {
      return m_elseLoc;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_ifLoc);
   }

   SourceLoc getEndLoc() const
   {
      return (m_else ? m_else->getEndLoc() : m_then->getEndLoc());
   }

   Stmt *getThenStmt() const
   {
      return m_then;
   }

   void setThenStmt(Stmt *s)
   {
      m_then = s;
   }

   Stmt *getElseStmt() const
   {
      return m_else;
   }

   void setElseStmt(Stmt *stmt)
   {
      m_else = stmt;
   }

   // Implement isa/cast/dyncast/etc.
   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::If;
   }

private:
   SourceLoc m_ifLoc;
   SourceLoc m_elseLoc;
   Stmt *m_then;
   Stmt *m_else;
};

/// GuardStmt - 'guard' statement.  Evaluate a condition and if it fails, run
/// its body.  The body is always guaranteed to exit the current scope (or
/// abort), it never falls through.
///
class GuardStmt : public LabeledConditionalStmt
{
public:
   GuardStmt(SourceLoc m_guardLoc, StmtCondition cond,
             Stmt *m_body, std::optional<bool> implicit = std::nullopt)
      : LabeledConditionalStmt(StmtKind::Guard,
                               getDefaultImplicitFlag(implicit, m_guardLoc),
                               LabeledStmtInfo(), cond),
        m_guardLoc(m_guardLoc), m_body(m_body)
   {}

   GuardStmt(SourceLoc m_guardLoc, Expr *cond, Stmt *m_body,
             std::optional<bool> implicit, AstContext &Ctx);

   SourceLoc getGuardLoc() const
   {
      return m_guardLoc;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_guardLoc);
   }

   SourceLoc getEndLoc() const
   {
      return m_body->getEndLoc();
   }

   Stmt *getBody() const
   {
      return m_body;
   }

   void setBody(Stmt *stmt)
   {
      m_body = stmt;
   }

   // Implement isa/cast/dyncast/etc.
   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Guard;
   }
private:
   SourceLoc m_guardLoc;
   Stmt *m_body;
};

/// WhileStmt - while statement. After type-checking, the condition is of
/// type Builtin.Int1.
class WhileStmt : public LabeledConditionalStmt
{
public:
   WhileStmt(LabeledStmtInfo labelInfo, SourceLoc whileLoc, StmtCondition cond,
             Stmt *body, std::optional<bool> implicit = std::nullopt)
      : LabeledConditionalStmt(StmtKind::While,
                               getDefaultImplicitFlag(implicit, whileLoc),
                               labelInfo, cond),
        m_whileLoc(whileLoc),
        m_body(body)
   {}

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_whileLoc);
   }

   SourceLoc getEndLoc() const
   {
      return m_body->getEndLoc();
   }

   Stmt *getBody() const
   {
      return m_body;
   }

   void setBody(Stmt *stmt)
   {
      m_body = stmt;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::While;
   }
private:
   SourceLoc m_whileLoc;
   StmtCondition m_cond;
   Stmt *m_body;
};

/// RepeatWhileStmt - repeat/while statement. After type-checking, the
/// condition is of type Builtin.Int1.
class RepeatWhileStmt : public LabeledStmt
{
public:
   RepeatWhileStmt(LabeledStmtInfo labelInfo, SourceLoc repeatLoc, Expr *cond,
                   SourceLoc whileLoc, Stmt *body, std::optional<bool> implicit = std::nullopt)
      : LabeledStmt(StmtKind::RepeatWhile,
                    getDefaultImplicitFlag(implicit, repeatLoc),
                    labelInfo),
        m_repeatLoc(repeatLoc),
        m_whileLoc(whileLoc),
        m_body(body),
        m_cond(cond)
   {}

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_repeatLoc);
   }

   SourceLoc getEndLoc() const;

   Stmt *getBody() const
   {
      return m_body;
   }

   void setBody(Stmt *stmt)
   {
      m_body = stmt;
   }

   Expr *getCond() const
   {
      return m_cond;
   }

   void setCond(Expr *expr)
   {
      m_cond = expr;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::RepeatWhile;
   }
private:
   SourceLoc m_repeatLoc;
   SourceLoc m_whileLoc;
   Stmt *m_body;
   Expr *m_cond;
};

/// ForEachStmt - foreach statement that iterates over the elements in a
/// container.
///
/// Example:
/// \code
/// for i in 0...10 {
///   print(String(i))
/// }
/// \endcode
class ForEachStmt : public LabeledStmt
{
public:
   ForEachStmt(LabeledStmtInfo labelInfo, SourceLoc forLoc,
               SourceLoc inLoc, Expr *sequence, Expr *whereExpr, BraceStmt *body,
               std::optional<bool> implicit = std::nullopt)
      : LabeledStmt(StmtKind::ForEach, getDefaultImplicitFlag(implicit, forLoc),
                    labelInfo),
        m_forLoc(forLoc), m_inLoc(inLoc), m_sequence(sequence),
        m_whereExpr(whereExpr), m_body(body)
   {}

   /// getForLoc - Retrieve the location of the 'for' keyword.
   SourceLoc getForLoc() const
   {
      return m_forLoc;
   }

   /// getInLoc - Retrieve the location of the 'in' keyword.
   SourceLoc getInLoc() const
   {
      return m_inLoc;
   }

   Expr *getWhere() const
   {
      return m_whereExpr;
   }

   void setWhere(Expr *where)
   {
      m_whereExpr = where;
   }

   /// getSequence - Retrieve the m_sequence whose elements will be visited
   /// by this foreach loop, as it was written in the source code and
   /// subsequently type-checked. To determine the semantic behavior of this
   /// expression to extract a range, use \c getRangeInit().
   Expr *getSequence() const
   {
      return m_sequence;
   }

   void setSequence(Expr *seqExpr)
   {
      m_sequence = seqExpr;
   }

   /// getBody - Retrieve the body of the loop.
   BraceStmt *getBody() const
   {
      return m_body;
   }

   void setBody(BraceStmt *body)
   {
      m_body = body;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_forLoc);
   }

   SourceLoc getEndLoc() const
   {
      return m_body->getEndLoc();
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::ForEach;
   }

private:
   SourceLoc m_forLoc;
   SourceLoc m_inLoc;
   Expr *m_sequence;
   Expr *m_whereExpr = nullptr;
   BraceStmt *m_body;
};

/// A pattern and an optional guard expression used in a 'case' statement.
class CaseLabelItem
{
public:
   CaseLabelItem(const CaseLabelItem &) = default;

   SourceLoc getWhereLoc() const { return m_whereLoc; }

   SourceLoc getStartLoc() const;
   SourceLoc getEndLoc() const;
   SourceRange getSourceRange() const;

   /// Return the guard expression if present, or null if the case label has
   /// no guard.
   Expr *getGuardExpr()
   {
      return m_guardExprAndKind.getPointer();
   }

   const Expr *getGuardExpr() const
   {
      return m_guardExprAndKind.getPointer();
   }

   void setGuardExpr(Expr *expr)
   {
      m_guardExprAndKind.setPointer(expr);
   }

   /// Returns true if this is syntactically a 'default' label.
   bool isDefault() const
   {
      return m_guardExprAndKind.getInt() == kind::Default;
   }
private:
   enum class kind
   {
      /// A normal pattern
      Normal = 0,
      /// `default`
      Default,
   };

   SourceLoc m_whereLoc;
   PointerIntPair<Expr *, 1, kind> m_guardExprAndKind;

   CaseLabelItem(kind kind, SourceLoc whereLoc,
                 Expr *guardExpr)
      : m_whereLoc(whereLoc),
        m_guardExprAndKind(guardExpr, kind)
   {}
};

/// A 'case' or 'default' block of a switch statement.  Only valid as the
/// substatement of a SwitchStmt.  A case block begins either with one or more
/// m_caseLabelItems or a single 'default' label.
///
/// Some examples:
/// \code
///   case 1:
///   case 2, 3:
///   case Foo(var x, var y) where x < y:
///   case 2 where foo(), 3 where bar():
///   default:
/// \endcode
///
class CaseStmt final : public Stmt,
      private polar::utils::TrailingObjects<CaseStmt, CaseLabelItem>
{
public:
   static CaseStmt *create(AstContext &context, SourceLoc caseLoc,
                           ArrayRef<CaseLabelItem> caseLabelItems,
                           bool hasBoundDecls, SourceLoc unknownAttrLoc,
                           SourceLoc colonLoc, Stmt *body,
                           std::optional<bool> implicit = std::nullopt);

   ArrayRef<CaseLabelItem> getCaseLabelItems() const
   {
      return {getTrailingObjects<CaseLabelItem>(), bits.CaseStmt.NumPatterns};
   }

   MutableArrayRef<CaseLabelItem> getMutableCaseLabelItems()
   {
      return {getTrailingObjects<CaseLabelItem>(), bits.CaseStmt.NumPatterns};
   }

   Stmt *getBody() const
   {
      return m_bodyAndHasBoundDecls.getPointer();
   }

   void setBody(Stmt *body)
   {
      m_bodyAndHasBoundDecls.setPointer(body);
   }

   /// True if the case block declares any patterns with local variable bindings.
   bool hasBoundDecls() const
   {
      return m_bodyAndHasBoundDecls.getInt();
   }

   /// Get the source location of the 'case' or 'default' of the first label.
   SourceLoc getLoc() const
   {
      return m_caseLoc;
   }

   SourceLoc getStartLoc() const
   {
      if (m_unknownAttrLoc.isValid())
         return m_unknownAttrLoc;
      return getLoc();
   }
   SourceLoc getEndLoc() const { return getBody()->getEndLoc(); }
   SourceRange getLabelItemsRange() const {
      return m_colonLoc.isValid() ? SourceRange(getLoc(), m_colonLoc) : getSourceRange();
   }

   bool isDefault() { return getCaseLabelItems()[0].isDefault(); }

   bool hasUnknownAttr() const {
      // Note: This representation doesn't allow for synthesized @unknown cases.
      // However, that's probably sensible; the purpose of @unknown is for
      // diagnosing otherwise-non-exhaustive switches, and the user can't edit
      // a synthesized case.
      return m_unknownAttrLoc.isValid();
   }

   static bool classof(const Stmt *S) { return S->getKind() == StmtKind::Case; }
private:
   friend class TrailingObjects;
   SourceLoc m_unknownAttrLoc;
   SourceLoc m_caseLoc;
   SourceLoc m_colonLoc;
   PointerIntPair<Stmt *, 1, bool> m_bodyAndHasBoundDecls;
   CaseStmt(SourceLoc caseLoc, ArrayRef<CaseLabelItem> caseLabelItems,
            bool hasBoundDecls, SourceLoc unknownAttrLoc, SourceLoc colonLoc,
            Stmt *body, std::optional<bool> implicit);
};

/// Switch statement.
class SwitchStmt final : public LabeledStmt,
      private polar::utils::TrailingObjects<SwitchStmt, AstNode>
{
public:
   /// Allocate a new SwitchStmt in the given AstContext.
   static SwitchStmt *create(LabeledStmtInfo labelInfo, SourceLoc switchLoc,
                             Expr *subjectExpr,
                             SourceLoc lbraceLoc,
                             ArrayRef<AstNode> cases,
                             SourceLoc rbraceLoc,
                             AstContext &context);

   /// Get the source location of the 'switch' keyword.
   SourceLoc getSwitchLoc() const
   {
      return m_switchLoc;
   }

   /// Get the source location of the opening brace.
   SourceLoc getLBraceLoc() const
   {
      return m_lbraceLoc;
   }

   /// Get the source location of the closing brace.
   SourceLoc getRBraceLoc() const
   {
      return m_rbraceLoc;
   }

   SourceLoc getLoc() const
   {
      return m_switchLoc;
   }

   SourceLoc getStartLoc() const
   {
      return getLabelLocOrKeywordLoc(m_switchLoc);
   }

   SourceLoc getEndLoc() const
   {
      return m_rbraceLoc;
   }

   /// Get the subject expression of the switch.
   Expr *getSubjectExpr() const
   {
      return m_subjectExpr;
   }

   void setSubjectExpr(Expr *e)
   {
      m_subjectExpr = e;
   }

   ArrayRef<AstNode> getRawCases() const
   {
      return {getTrailingObjects<AstNode>(), bits.SwitchStmt.caseCount};
   }

private:
   struct AsCaseStmtWithSkippingNonCaseStmts
   {
      AsCaseStmtWithSkippingNonCaseStmts()
      {}

      std::optional<CaseStmt*> operator()(const AstNode &node) const
      {
         if (auto *cs = polar::utils::dyn_cast_or_null<CaseStmt>(node.dynamicCast<Stmt*>())) {
            return cs;
         }
         return std::nullopt;
      }
   };

public:
   using AsCaseStmtRange = OptionalTransformRange<ArrayRef<AstNode>,
   AsCaseStmtWithSkippingNonCaseStmts>;

   /// Get the list of case clauses.
   AsCaseStmtRange getCases() const
   {
      return AsCaseStmtRange(getRawCases(), AsCaseStmtWithSkippingNonCaseStmts());
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Switch;
   }
private:
   friend class TrailingObjects;
   SourceLoc m_switchLoc;
   SourceLoc m_lbraceLoc;
   SourceLoc m_rbraceLoc;
   Expr *m_subjectExpr;
   SwitchStmt(LabeledStmtInfo labelInfo, SourceLoc switchLoc, Expr *subjectExpr,
              SourceLoc lbraceLoc, unsigned caseCount, SourceLoc rbraceLoc,
              std::optional<bool> implicit = std::nullopt)
      : LabeledStmt(StmtKind::Switch, getDefaultImplicitFlag(implicit, switchLoc),
                    labelInfo),
        m_switchLoc(switchLoc),
        m_lbraceLoc(lbraceLoc),
        m_rbraceLoc(rbraceLoc),
        m_subjectExpr(subjectExpr)
   {
      bits.SwitchStmt.caseCount = caseCount;
   }

};

/// BreakStmt - The "break" and "break label" statement.
class BreakStmt : public Stmt
{
public:
   BreakStmt(SourceLoc loc, Identifier targetName, SourceLoc targetLoc,
             std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Break, getDefaultImplicitFlag(implicit, loc)),
        m_loc(loc),
        m_targetName(targetName),
        m_targetLoc(targetLoc)
   {}

   SourceLoc getLoc() const
   {
      return m_loc;
   }

   Identifier getTargetName() const
   {
      return m_targetName;
   }

   void setTargetName(Identifier node)
   {
      m_targetName = node;
   }

   SourceLoc getTargetLoc() const
   {
      return m_targetLoc;
   }

   void setTargetLoc(SourceLoc loc)
   {
      m_targetLoc = loc;
   }

   // Manipulate the target loop/switch that is bring broken out of.  This is set
   // by sema during type checking.
   void setTarget(LabeledStmt *lableStmt)
   {
      m_target = lableStmt;
   }

   LabeledStmt *getTarget() const
   {
      return m_target;
   }

   SourceLoc getStartLoc() const
   {
      return m_loc;
   }

   SourceLoc getEndLoc() const
   {
      return (m_targetLoc.isValid() ? m_targetLoc : m_loc);
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Break;
   }
private:
   SourceLoc m_loc;
   Identifier m_targetName; // Named target statement, if specified in the source.
   SourceLoc m_targetLoc;
   LabeledStmt *m_target = nullptr;  // Target stmt, wired up by Sema.
};

/// ContinueStmt - The "continue" and "continue label" statement.
class ContinueStmt : public Stmt
{
public:
   ContinueStmt(SourceLoc loc, Identifier targetName, SourceLoc targetLoc,
                std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Continue, getDefaultImplicitFlag(implicit, loc)),
        m_loc(loc),
        m_targetName(targetName),
        m_targetLoc(targetLoc)
   {}

   Identifier getTargetName() const
   {
      return m_targetName;
   }

   void setTargetName(Identifier node)
   {
      m_targetName = node;
   }

   SourceLoc getTargetLoc() const
   {
      return m_targetLoc;
   }

   void setTargetLoc(SourceLoc loc)
   {
      m_targetLoc = loc;
   }

   // Manipulate the target loop that is bring continued.  This is set by sema
   // during type checking.
   void setTarget(LabeledStmt *labelStmt)
   {
      m_target = labelStmt;
   }

   LabeledStmt *getTarget() const
   {
      return m_target;
   }

   SourceLoc getLoc() const
   {
      return m_loc;
   }

   SourceLoc getStartLoc() const
   {
      return m_loc;

   }
   SourceLoc getEndLoc() const
   {
      return (m_targetLoc.isValid() ? m_targetLoc : m_loc);
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Continue;
   }

private:
   SourceLoc m_loc;
   Identifier m_targetName; // Named target statement, if specified in the source.
   SourceLoc m_targetLoc;
   LabeledStmt *m_target = nullptr;
};

/// FallthroughStmt - The keyword "fallthrough".
class FallthroughStmt : public Stmt
{
public:
   FallthroughStmt(SourceLoc loc, std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Fallthrough, getDefaultImplicitFlag(implicit, loc)),
        m_loc(loc),
        m_fallthroughSource(nullptr),
        m_fallthroughDest(nullptr)
   {}

   SourceLoc getLoc() const
   {
      return m_loc;
   }

   SourceRange getSourceRange() const
   {
      return m_loc;
   }

   /// Get the CaseStmt block from which the fallthrough transfers control.
   /// Set during Sema. (May stay null if fallthrough is invalid.)
   CaseStmt *getFallthroughSource() const
   {
      return m_fallthroughSource;
   }

   void setFallthroughSource(CaseStmt *context)
   {
      assert(!FallthroughSource && "fallthrough source already set?!");
      m_fallthroughSource = context;
   }

   /// Get the CaseStmt block to which the fallthrough transfers control.
   /// Set during Sema.
   CaseStmt *getFallthroughDest() const
   {
      assert(m_fallthroughDest && "fallthrough dest is not set until Sema");
      return m_fallthroughDest;
   }

   void setFallthroughDest(CaseStmt *context)
   {
      assert(!m_fallthroughDest && "fallthrough dest already set?!");
      m_fallthroughDest = context;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Fallthrough;
   }
private:
   SourceLoc m_loc;
   CaseStmt *m_fallthroughSource;
   CaseStmt *m_fallthroughDest;
};

/// FailStmt - A statement that indicates a failable, which is currently
/// spelled as "return nil" and can only be used within failable initializers.
class FailStmt : public Stmt
{
public:
   FailStmt(SourceLoc returnLoc, SourceLoc nilLoc,
            std::optional<bool> implicit = std::nullopt)
      : Stmt(StmtKind::Fail, getDefaultImplicitFlag(implicit, returnLoc)),
        m_returnLoc(returnLoc),
        m_nilLoc(nilLoc)
   {}

   SourceLoc getLoc() const
   {
      return m_returnLoc;
   }

   SourceRange getSourceRange() const
   {
      return SourceRange(m_returnLoc, m_nilLoc);
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Fail;
   }
private:
   SourceLoc m_returnLoc;
   SourceLoc m_nilLoc;
};

/// ThrowStmt - Throws an error.
class ThrowStmt : public Stmt
{
public:
   explicit ThrowStmt(SourceLoc throwLoc, Expr *subExpr)
      : Stmt(StmtKind::Throw, /*implicit=*/false),
        m_subExpr(subExpr),
        m_throwLoc(throwLoc)
   {}

   SourceLoc getThrowLoc() const
   {
      return m_throwLoc;
   }

   SourceLoc getStartLoc() const
   {
      return m_throwLoc;
   }

   SourceLoc getEndLoc() const;
   SourceRange getSourceRange() const
   {
      return SourceRange(m_throwLoc, getEndLoc());
   }

   Expr *getSubExpr() const
   {
      return m_subExpr;
   }

   void setSubExpr(Expr *subExpr)
   {
      m_subExpr = subExpr;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::Throw;
   }
private:
   Expr *m_subExpr;
   SourceLoc m_throwLoc;
};

/// PoundAssertStmt - Asserts that a condition is true, at compile time.
class PoundAssertStmt : public Stmt
{
public:
   PoundAssertStmt(SourceRange range, Expr *condition, StringRef message)
      : Stmt(StmtKind::PoundAssert, /*implicit=*/false),
        m_range(Range),
        m_condition(condition),
        m_message(message) {}

   SourceRange getSourceRange() const
   {
      return m_range;
   }

   Expr *getCondition() const
   {
      return m_condition;
   }

   StringRef getMessage() const
   {
      return m_message;
   }

   void setCondition(Expr *condition)
   {
      m_condition = condition;
   }

   static bool classof(const Stmt *stmt)
   {
      return stmt->getKind() == StmtKind::PoundAssert;
   }
private:
   SourceRange m_range;
   Expr *m_condition;
   StringRef m_message;
};

} // polar::ast

#endif // POLARPHP_AST_STMT_H
