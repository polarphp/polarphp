//===--- ParsePIL.cpp - PIL File Parsing logic ----------------------------===//
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

#include "polarphp/pil/parser/internal/PILParserFunctionBuilder.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/NameLookupRequests.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/basic/Timer.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/llparser/Lexer.h"
#include "polarphp/llparser/ParsePILSupport.h"
#include "polarphp/llparser/Parser.h"
#include "polarphp/llparser/TokenKindsDef.h"
#include "polarphp/llparser/SyntaxKinds.h"
#include "polarphp/pil/lang/AbstractionPattern.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILUndef.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "polarphp/global/Subsystems.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/SaveAndRestore.h"

using namespace polar;
using namespace polar::llparser;

//===----------------------------------------------------------------------===//
// PILParserState implementation
//===----------------------------------------------------------------------===//

namespace polar {

// This has to be in the 'polar' namespace because it's forward-declared for
// PILParserState.
class PILParserTUState : public PILParserTUStateBase {
public:
   explicit PILParserTUState(PILModule &M) : M(M) {}
   ~PILParserTUState();

   PILModule &M;

   /// This is all of the forward referenced functions with
   /// the location for where the reference is.
   llvm::DenseMap<Identifier,
      std::pair<PILFunction*, SourceLoc>> ForwardRefFns;
   /// A list of all functions forward-declared by a pil_scope.
   llvm::DenseSet<PILFunction *> PotentialZombieFns;

   /// A map from textual .sil scope number to PILDebugScopes.
   llvm::DenseMap<unsigned, PILDebugScope *> ScopeSlots;

   /// Did we parse a pil_stage for this module?
   bool DidParsePILStage = false;

   bool parseDeclPIL(Parser &P) override;
   bool parseDeclPILStage(Parser &P) override;
   bool parsePILVTable(Parser &P) override;
   bool parsePILGlobal(Parser &P) override;
   bool parsePILWitnessTable(Parser &P) override;
   bool parsePILDefaultWitnessTable(Parser &P) override;
   bool parsePILCoverageMap(Parser &P) override;
   bool parsePILProperty(Parser &P) override;
   bool parsePILScope(Parser &P) override;
};
} // end namespace polar

PILParserTUState::~PILParserTUState() {
   if (!ForwardRefFns.empty()) {
      for (auto Entry : ForwardRefFns) {
         if (Entry.second.second.isValid()) {
            M.getAstContext().Diags.diagnose(Entry.second.second,
                                             diag::pil_use_of_undefined_value,
                                             Entry.first.str());
         }
      }
   }

   // Turn any debug-info-only function declarations into zombies.
   for (auto *Fn : PotentialZombieFns)
      if (Fn->isExternalDeclaration()) {
         Fn->setInlined();
         M.eraseFunction(Fn);
      }
}

PILParserState::PILParserState(PILModule *M)
   : Impl(M ? std::make_unique<PILParserTUState>(*M) : nullptr) {}

PILParserState::~PILParserState() = default;

void PrettyStackTraceParser::print(llvm::raw_ostream &out) const {
   out << "With parser at source location: ";
   P.Tok.getLoc().print(out, P.Context.SourceMgr);
   out << '\n';
}

static bool parseIntoSourceFileImpl(SourceFile &SF,
                                    unsigned BufferID,
                                    bool *Done,
                                    PILParserState *PIL,
                                    PersistentParserState *PersistentState,
                                    bool FullParse,
                                    bool DelayBodyParsing) {
   assert((!FullParse || (SF.canBeParsedInFull() && !PIL)) &&
          "cannot parse in full with the given parameters!");
   // @todo
//   std::shared_ptr<SyntaxTreeCreator> STreeCreator;
//   if (SF.shouldBuildSyntaxTree()) {
//      STreeCreator = std::make_shared<SyntaxTreeCreator>(
//         SF.getAstContext().SourceMgr, BufferID,
//         SF.SyntaxParsingCache, SF.getAstContext().getSyntaxArena());
//   }

   // Not supported right now.
   if (SF.Kind == SourceFileKind::REPL)
      DelayBodyParsing = false;
   if (SF.hasInterfaceHash())
      DelayBodyParsing = false;
   if (SF.shouldCollectToken())
      DelayBodyParsing = false;
   if (SF.shouldBuildSyntaxTree())
      DelayBodyParsing = false;
   if (PIL)
      DelayBodyParsing = false;

   FrontendStatsTracer tracer(SF.getAstContext().Stats, "Parsing");
   Parser P(BufferID, SF, PIL ? PIL->Impl.get() : nullptr,
            PersistentState, DelayBodyParsing);
   PrettyStackTraceParser StackTrace(P);

   llvm::SaveAndRestore<bool> S(P.IsParsingInterfaceTokens,
                                SF.hasInterfaceHash());

   bool FoundSideEffects = false;
   do {
      bool hasSideEffects = P.parseTopLevel();
      FoundSideEffects = FoundSideEffects || hasSideEffects;
      *Done = P.Tok.is(tok::eof);
   } while (FullParse && !*Done);

//   if (STreeCreator) {
//      auto rawNode = P.finalizeSyntaxTree();
//      STreeCreator->acceptSyntaxRoot(rawNode, SF);
//   }

   return FoundSideEffects;
}

bool polar::parseIntoSourceFile(SourceFile &SF,
                                unsigned BufferID,
                                bool *Done,
                                PILParserState *PIL,
                                PersistentParserState *PersistentState,
                                bool DelayBodyParsing) {
   return parseIntoSourceFileImpl(SF, BufferID, Done, PIL,
                                  PersistentState,
      /*FullParse=*/SF.shouldBuildSyntaxTree(),
                                  DelayBodyParsing);
}

bool polar::parseIntoSourceFileFull(SourceFile &SF, unsigned BufferID,
                                    PersistentParserState *PersistentState,
                                    bool DelayBodyParsing) {
   bool Done = false;
   return parseIntoSourceFileImpl(SF, BufferID, &Done, /*PIL=*/nullptr,
                                  PersistentState, /*FullParse=*/true,
                                  DelayBodyParsing);
}


//===----------------------------------------------------------------------===//
// PILParser
//===----------------------------------------------------------------------===//

namespace {
struct ParsedSubstitution {
   SourceLoc loc;
   Type replacement;
};

struct ParsedSpecAttr {
   ArrayRef<RequirementRepr> requirements;
   bool exported;
   PILSpecializeAttr::SpecializationKind kind;
};

enum class ConformanceContext {
   /// A normal conformance parse.
      Ordinary,

   /// We're parsing this for a PIL witness table.
   /// Leave any generic parameter clauses in scope, and use an explicit
   /// self-conformance instead of an abstract one.
      WitnessTable,
};

class PILParser {
   friend PILParserTUState;
public:
   Parser &P;
   PILModule &PILMod;
   PILParserTUState &TUState;
   PILFunction *F = nullptr;
   GenericEnvironment *ContextGenericEnv = nullptr;

private:
   /// HadError - Have we seen an error parsing this function?
   bool HadError = false;

   /// Data structures used to perform name lookup of basic blocks.
   llvm::DenseMap<Identifier, PILBasicBlock*> BlocksByName;
   llvm::DenseMap<PILBasicBlock*,
      std::pair<SourceLoc, Identifier>> UndefinedBlocks;

   /// Data structures used to perform name lookup for local values.
   llvm::StringMap<ValueBase*> LocalValues;
   llvm::StringMap<SourceLoc> ForwardRefLocalValues;

   /// A callback to be invoked every time a type was deserialized.
   std::function<void(Type)> ParsedTypeCallback;

   bool performTypeLocChecking(TypeLoc &T, bool IsPILType,
                               GenericEnvironment *GenericEnv = nullptr,
                               DeclContext *DC = nullptr);

   void convertRequirements(PILFunction *F, ArrayRef<RequirementRepr> From,
                            SmallVectorImpl<Requirement> &To);

   InterfaceConformanceRef parseInterfaceConformanceHelper(
      InterfaceDecl *&proto, GenericEnvironment *GenericEnv,
      ConformanceContext context, InterfaceDecl *defaultForProto);

public:
   PILParser(Parser &P)
      : P(P), PILMod(static_cast<PILParserTUState *>(P.PIL)->M),
        TUState(*static_cast<PILParserTUState *>(P.PIL)),
        ParsedTypeCallback([](Type ty) {}) {}

   /// diagnoseProblems - After a function is fully parse, emit any diagnostics
   /// for errors and return true if there were any.
   bool diagnoseProblems();

   /// getGlobalNameForReference - Given a reference to a global name, look it
   /// up and return an appropriate PIL function.
   PILFunction *getGlobalNameForReference(Identifier Name,
                                          CanPILFunctionType Ty,
                                          SourceLoc Loc,
                                          bool IgnoreFwdRef = false);
   /// getGlobalNameForDefinition - Given a definition of a global name, look
   /// it up and return an appropriate PIL function.
   PILFunction *getGlobalNameForDefinition(Identifier Name,
                                           CanPILFunctionType Ty,
                                           SourceLoc Loc);

   /// getBBForDefinition - Return the PILBasicBlock for a definition of the
   /// specified block.
   PILBasicBlock *getBBForDefinition(Identifier Name, SourceLoc Loc);

   /// getBBForReference - return the PILBasicBlock of the specified name.  The
   /// source location is used to diagnose a failure if the block ends up never
   /// being defined.
   PILBasicBlock *getBBForReference(Identifier Name, SourceLoc Loc);

   struct UnresolvedValueName {
      StringRef Name;
      SourceLoc NameLoc;

      bool isUndef() const { return Name == "undef"; }
   };

   /// getLocalValue - Get a reference to a local value with the specified name
   /// and type.
   PILValue getLocalValue(UnresolvedValueName Name, PILType Type,
                          PILLocation L, PILBuilder &B);

   /// setLocalValue - When an instruction or block argument is defined, this
   /// method is used to register it and update our symbol table.
   void setLocalValue(ValueBase *Value, StringRef Name, SourceLoc NameLoc);

   PILDebugLocation getDebugLoc(PILBuilder & B, PILLocation Loc) {
      return PILDebugLocation(Loc, F->getDebugScope());
   }

   /// @{ Primitive parsing.

   /// \verbatim
   ///   pil-identifier ::= [A-Za-z_0-9]+
   /// \endverbatim
   bool parsePILIdentifier(Identifier &Result, SourceLoc &Loc,
                           const Diagnostic &D);

   template<typename ...DiagArgTypes, typename ...ArgTypes>
   bool parsePILIdentifier(Identifier &Result, Diag<DiagArgTypes...> ID,
                           ArgTypes... Args) {
      SourceLoc L;
      return parsePILIdentifier(Result, L, Diagnostic(ID, Args...));
   }

   template <typename T, typename... DiagArgTypes, typename... ArgTypes>
   bool parsePILIdentifierSwitch(T &Result, ArrayRef<StringRef> Strings,
                                 Diag<DiagArgTypes...> ID, ArgTypes... Args) {
      Identifier TmpResult;
      SourceLoc L;
      if (parsePILIdentifier(TmpResult, L, Diagnostic(ID, Args...))) {
         return true;
      }

      auto Iter = std::find(Strings.begin(), Strings.end(), TmpResult.str());
      if (Iter == Strings.end()) {
         P.diagnose(P.Tok, Diagnostic(ID, Args...));
         return true;
      }

      Result = T(*Iter);
      return false;
   }

   template<typename ...DiagArgTypes, typename ...ArgTypes>
   bool parsePILIdentifier(Identifier &Result, SourceLoc &L,
                           Diag<DiagArgTypes...> ID, ArgTypes... Args) {
      return parsePILIdentifier(Result, L, Diagnostic(ID, Args...));
   }

   bool parseVerbatim(StringRef identifier);

   template <typename T>
   bool parseInteger(T &Result, const Diagnostic &D) {
      if (!P.Tok.is(tok::integer_literal)) {
         P.diagnose(P.Tok, D);
         return true;
      }
      bool error = parseIntegerLiteral(P.Tok.getText(), 0, Result);
      P.consumeToken(tok::integer_literal);
      return error;
   }

   template <typename T>
   bool parseIntegerLiteral(StringRef text, unsigned radix, T &result) {
      text = prepareIntegerLiteralForParsing(text);
      return text.getAsInteger(radix, result);
   }

   StringRef prepareIntegerLiteralForParsing(StringRef text) {
      // tok::integer_literal can contain characters that the library
      // parsing routines don't expect.
      if (text.contains('_'))
         text = P.copyAndStripUnderscores(text);
      return text;
   }

   /// @}

   /// @{ Type parsing.
   bool parseASTType(CanType &result,
                     GenericEnvironment *environment = nullptr);
   bool parseASTType(CanType &result, SourceLoc &TypeLoc) {
      TypeLoc = P.Tok.getLoc();
      return parseASTType(result);
   }
   bool parseASTType(CanType &result,
                     SourceLoc &TypeLoc,
                     GenericEnvironment *env) {
      TypeLoc = P.Tok.getLoc();
      return parseASTType(result, env);
   }
   bool parsePILOwnership(ValueOwnershipKind &OwnershipKind) {
      // We parse here @ <identifier>.
      if (!P.consumeIf(tok::at_sign)) {
         // If we fail, we must have @any ownership. We check elsewhere in the
         // parser that this matches what the function signature wants.
         OwnershipKind = ValueOwnershipKind::None;
         return false;
      }

      StringRef AllOwnershipKinds[3] = {"unowned", "owned",
                                        "guaranteed"};
      return parsePILIdentifierSwitch(OwnershipKind, AllOwnershipKinds,
                                      diag::expected_pil_value_ownership_kind);
   }
   bool parsePILType(PILType &Result,
                     GenericEnvironment *&parsedGenericEnv,
                     bool IsFuncDecl = false,
                     GenericEnvironment *parentGenericEnv = nullptr);
   bool parsePILType(PILType &Result) {
      GenericEnvironment *IgnoredEnv;
      return parsePILType(Result, IgnoredEnv);
   }
   bool parsePILType(PILType &Result, SourceLoc &TypeLoc) {
      TypeLoc = P.Tok.getLoc();
      return parsePILType(Result);
   }
   bool parsePILType(PILType &Result, SourceLoc &TypeLoc,
                     GenericEnvironment *&parsedGenericEnv,
                     GenericEnvironment *parentGenericEnv = nullptr) {
      TypeLoc = P.Tok.getLoc();
      return parsePILType(Result, parsedGenericEnv, false, parentGenericEnv);
   }
   /// @}

   bool parsePILDottedPath(ValueDecl *&Decl,
                           SmallVectorImpl<ValueDecl *> &values);
   bool parsePILDottedPath(ValueDecl *&Decl) {
      SmallVector<ValueDecl *, 4> values;
      return parsePILDottedPath(Decl, values);
   }
   bool parsePILDottedPathWithoutPound(ValueDecl *&Decl,
                                       SmallVectorImpl<ValueDecl *> &values);
   bool parsePILDottedPathWithoutPound(ValueDecl *&Decl) {
      SmallVector<ValueDecl *, 4> values;
      return parsePILDottedPathWithoutPound(Decl, values);
   }
   /// At the time of calling this function, we may not have the type of the
   /// Decl yet. So we return a PILDeclRef on the first lookup result and also
   /// return all the lookup results. After parsing the expected type, the
   /// caller of this function can choose the one that has the expected type.
   bool parsePILDeclRef(PILDeclRef &Result,
                        SmallVectorImpl<ValueDecl *> &values);
   bool parsePILDeclRef(PILDeclRef &Result) {
      SmallVector<ValueDecl *, 4> values;
      return parsePILDeclRef(Result, values);
   }
   bool parsePILDeclRef(PILDeclRef &Member, bool FnTypeRequired);
   bool parseGlobalName(Identifier &Name);
   bool parseValueName(UnresolvedValueName &Name);
   bool parseValueRef(PILValue &Result, PILType Ty, PILLocation Loc,
                      PILBuilder &B);
   bool parseTypedValueRef(PILValue &Result, SourceLoc &Loc, PILBuilder &B);
   bool parseTypedValueRef(PILValue &Result, PILBuilder &B) {
      SourceLoc Tmp;
      return parseTypedValueRef(Result, Tmp, B);
   }
   bool parsePILOpcode(PILInstructionKind &Opcode, SourceLoc &OpcodeLoc,
                       StringRef &OpcodeName);
   bool parsePILDebugVar(PILDebugVariable &Var);

   /// Parses the basic block arguments as part of branch instruction.
   bool parsePILBBArgsAtBranch(SmallVector<PILValue, 6> &Args, PILBuilder &B);

   bool parsePILLocation(PILLocation &L);
   bool parseScopeRef(PILDebugScope *&DS);
   bool parsePILDebugLocation(PILLocation &L, PILBuilder &B,
                              bool parsedComma = false);
   bool parsePILInstruction(PILBuilder &B);
   bool parseCallInstruction(PILLocation InstLoc,
                             PILInstructionKind Opcode, PILBuilder &B,
                             PILInstruction *&ResultVal);
   bool parsePILFunctionRef(PILLocation InstLoc, PILFunction *&ResultFn);

   bool parsePILBasicBlock(PILBuilder &B);
   bool parseKeyPathPatternComponent(KeyPathPatternComponent &component,
                                     SmallVectorImpl<PILType> &operandTypes,
                                     SourceLoc componentLoc,
                                     Identifier componentKind,
                                     PILLocation InstLoc,
                                     GenericEnvironment *patternEnv);
   bool isStartOfPILInstruction();

   bool parseSubstitutions(SmallVectorImpl<ParsedSubstitution> &parsed,
                           GenericEnvironment *GenericEnv=nullptr,
                           InterfaceDecl *defaultForProto = nullptr);

   InterfaceConformanceRef parseInterfaceConformance(
      InterfaceDecl *&proto, GenericEnvironment *&genericEnv,
      ConformanceContext context, InterfaceDecl *defaultForProto);
   InterfaceConformanceRef
   parseInterfaceConformance(InterfaceDecl *defaultForProto,
                             ConformanceContext context) {
      InterfaceDecl *dummy;
      GenericEnvironment *env;
      return parseInterfaceConformance(dummy, env, context, defaultForProto);
   }

   Optional<llvm::coverage::Counter>
   parsePILCoverageExpr(llvm::coverage::CounterExpressionBuilder &Builder);

   template <class T>
   struct ParsedEnum {
      Optional<T> Value;
      StringRef Name;
      SourceLoc Loc;

      bool isSet() const { return Value.hasValue(); }
      T operator*() const { return *Value; }
   };

   template <class T>
   void setEnum(ParsedEnum<T> &existing,
                T value, StringRef name, SourceLoc loc) {
      if (existing.Value) {
         if (*existing.Value == value) {
            P.diagnose(loc, diag::duplicate_attribute, /*modifier*/ 1);
         } else {
            P.diagnose(loc, diag::mutually_exclusive_attrs, name,
                       existing.Name, /*modifier*/ 1);
         }
         P.diagnose(existing.Loc, diag::previous_attribute, /*modifier*/ 1);
      }
      existing.Value = value;
      existing.Name = name;
      existing.Loc = loc;
   }

   template <class T>
   void maybeSetEnum(bool allowed, ParsedEnum<T> &existing,
                     T value, StringRef name, SourceLoc loc) {
      if (allowed)
         setEnum(existing, value, name, loc);
      else
         P.diagnose(loc, diag::unknown_attribute, name);
   }
};
} // end anonymous namespace

bool PILParser::parsePILIdentifier(Identifier &Result, SourceLoc &Loc,
                                   const Diagnostic &D) {
   switch (P.Tok.getKind()) {
      case tok::identifier:
      case tok::dollarident:
         Result = P.Context.getIdentifier(P.Tok.getText());
         break;
      case tok::string_literal: {
         // Drop the double quotes.
         StringRef rawString = P.Tok.getText().drop_front().drop_back();
         Result = P.Context.getIdentifier(rawString);
         break;
      }
      case tok::oper_binary_unspaced:  // fixme?
      case tok::oper_binary_spaced:
      case tok::kw_init:
         // A binary operator or `init` can be part of a PILDeclRef.
         Result = P.Context.getIdentifier(P.Tok.getText());
         break;
      default:
         // If it's some other keyword, grab an identifier for it.
         if (P.Tok.isKeyword()) {
            Result = P.Context.getIdentifier(P.Tok.getText());
            break;
         }
         P.diagnose(P.Tok, D);
         return true;
   }

   Loc = P.Tok.getLoc();
   P.consumeToken();
   return false;
}

bool PILParser::parseVerbatim(StringRef name) {
   Identifier tok;
   SourceLoc loc;

   if (parsePILIdentifier(tok, loc, diag::expected_tok_in_pil_instr, name)) {
      return true;
   }
   if (tok.str() != name) {
      P.diagnose(loc, diag::expected_tok_in_pil_instr, name);
      return true;
   }
   return false;
}

/// diagnoseProblems - After a function is fully parse, emit any diagnostics
/// for errors and return true if there were any.
bool PILParser::diagnoseProblems() {
   // Check for any uses of basic blocks that were not defined.
   if (!UndefinedBlocks.empty()) {
      // FIXME: These are going to come out in nondeterministic order.
      for (auto Entry : UndefinedBlocks)
         P.diagnose(Entry.second.first, diag::pil_undefined_basicblock_use,
                    Entry.second.second);

      HadError = true;
   }

   if (!ForwardRefLocalValues.empty()) {
      // FIXME: These are going to come out in nondeterministic order.
      for (auto &Entry : ForwardRefLocalValues)
         P.diagnose(Entry.second, diag::pil_use_of_undefined_value,
                    Entry.first());
      HadError = true;
   }

   return HadError;
}

/// getGlobalNameForDefinition - Given a definition of a global name, look
/// it up and return an appropriate PIL function.
PILFunction *PILParser::getGlobalNameForDefinition(Identifier name,
                                                   CanPILFunctionType ty,
                                                   SourceLoc sourceLoc) {
   PILParserFunctionBuilder builder(PILMod);
   auto silLoc = RegularLocation(sourceLoc);

   // Check to see if a function of this name has been forward referenced.  If so
   // complete the forward reference.
   auto iter = TUState.ForwardRefFns.find(name);
   if (iter != TUState.ForwardRefFns.end()) {
      PILFunction *fn = iter->second.first;

      // Verify that the types match up.
      if (fn->getLoweredFunctionType() != ty) {
         P.diagnose(sourceLoc, diag::pil_value_use_type_mismatch, name.str(),
                    fn->getLoweredFunctionType(), ty);
         P.diagnose(iter->second.second, diag::pil_prior_reference);
         fn = builder.createFunctionForForwardReference("" /*name*/, ty, silLoc);
      }

      assert(fn->isExternalDeclaration() && "Forward defns cannot have bodies!");
      TUState.ForwardRefFns.erase(iter);

      // Move the function to this position in the module.
      //
      // FIXME: Should we move this functionality into PILParserFunctionBuilder?
      PILMod.getFunctionList().remove(fn);
      PILMod.getFunctionList().push_back(fn);

      return fn;
   }

   // If we don't have a forward reference, make sure the function hasn't been
   // defined already.
   if (PILMod.lookUpFunction(name.str()) != nullptr) {
      P.diagnose(sourceLoc, diag::pil_value_redefinition, name.str());
      return builder.createFunctionForForwardReference("" /*name*/, ty, silLoc);
   }

   // Otherwise, this definition is the first use of this name.
   return builder.createFunctionForForwardReference(name.str(), ty, silLoc);
}

/// getGlobalNameForReference - Given a reference to a global name, look it
/// up and return an appropriate PIL function.
PILFunction *PILParser::getGlobalNameForReference(Identifier name,
                                                  CanPILFunctionType funcTy,
                                                  SourceLoc sourceLoc,
                                                  bool ignoreFwdRef) {
   PILParserFunctionBuilder builder(PILMod);
   auto silLoc = RegularLocation(sourceLoc);

   // Check to see if we have a function by this name already.
   if (PILFunction *fn = PILMod.lookUpFunction(name.str())) {
      // If so, check for matching types.
      if (fn->getLoweredFunctionType() == funcTy) {
         return fn;
      }

      P.diagnose(sourceLoc, diag::pil_value_use_type_mismatch, name.str(),
                 fn->getLoweredFunctionType(), funcTy);

      return builder.createFunctionForForwardReference("" /*name*/, funcTy,
                                                       silLoc);
   }

   // If we didn't find a function, create a new one - it must be a forward
   // reference.
   auto *fn =
      builder.createFunctionForForwardReference(name.str(), funcTy, silLoc);
   TUState.ForwardRefFns[name] = {fn, ignoreFwdRef ? SourceLoc() : sourceLoc};
   return fn;
}


/// getBBForDefinition - Return the PILBasicBlock for a definition of the
/// specified block.
PILBasicBlock *PILParser::getBBForDefinition(Identifier Name, SourceLoc Loc) {
   // If there was no name specified for this block, just create a new one.
   if (Name.empty())
      return F->createBasicBlock();

   PILBasicBlock *&BB = BlocksByName[Name];
   // If the block has never been named yet, just create it.
   if (BB == nullptr)
      return BB = F->createBasicBlock();

   // If it already exists, it was either a forward reference or a redefinition.
   // If it is a forward reference, it should be in our undefined set.
   if (!UndefinedBlocks.erase(BB)) {
      // If we have a redefinition, return a new BB to avoid inserting
      // instructions after the terminator.
      P.diagnose(Loc, diag::pil_basicblock_redefinition, Name);
      HadError = true;
      return F->createBasicBlock();
   }

   // FIXME: Splice the block to the end of the function so they come out in the
   // right order.
   return BB;
}

/// getBBForReference - return the PILBasicBlock of the specified name.  The
/// source location is used to diagnose a failure if the block ends up never
/// being defined.
PILBasicBlock *PILParser::getBBForReference(Identifier Name, SourceLoc Loc) {
   // If the block has already been created, use it.
   PILBasicBlock *&BB = BlocksByName[Name];
   if (BB != nullptr)
      return BB;

   // Otherwise, create it and remember that this is a forward reference so
   // that we can diagnose use without definition problems.
   BB = F->createBasicBlock();
   UndefinedBlocks[BB] = {Loc, Name};
   return BB;
}

///   pil-global-name:
///     '@' identifier
bool PILParser::parseGlobalName(Identifier &Name) {
   return P.parseToken(tok::at_sign, diag::expected_pil_value_name) ||
          parsePILIdentifier(Name, diag::expected_pil_value_name);
}

/// getLocalValue - Get a reference to a local value with the specified name
/// and type.
PILValue PILParser::getLocalValue(UnresolvedValueName Name, PILType Type,
                                  PILLocation Loc, PILBuilder &B) {
   if (Name.isUndef())
      return PILUndef::get(Type, B.getFunction());

   // Check to see if this is already defined.
   ValueBase *&Entry = LocalValues[Name.Name];

   if (Entry) {
      // If this value is already defined, check it to make sure types match.
      PILType EntryTy = Entry->getType();

      if (EntryTy != Type) {
         HadError = true;
         P.diagnose(Name.NameLoc, diag::pil_value_use_type_mismatch, Name.Name,
                    EntryTy.getAstType(), Type.getAstType());
         // Make sure to return something of the requested type.
         return new (PILMod) GlobalAddrInst(getDebugLoc(B, Loc), Type);
      }

      return PILValue(Entry);
   }

   // Otherwise, this is a forward reference.  Create a dummy node to represent
   // it until we see a real definition.
   ForwardRefLocalValues[Name.Name] = Name.NameLoc;

   Entry = new (PILMod) GlobalAddrInst(getDebugLoc(B, Loc), Type);
   return Entry;
}

/// setLocalValue - When an instruction or block argument is defined, this
/// method is used to register it and update our symbol table.
void PILParser::setLocalValue(ValueBase *Value, StringRef Name,
                              SourceLoc NameLoc) {
   ValueBase *&Entry = LocalValues[Name];

   // If this value was already defined, it is either a redefinition, or a
   // specification for a forward referenced value.
   if (Entry) {
      if (!ForwardRefLocalValues.erase(Name)) {
         P.diagnose(NameLoc, diag::pil_value_redefinition, Name);
         HadError = true;
         return;
      }

      // If the forward reference was of the wrong type, diagnose this now.
      if (Entry->getType() != Value->getType()) {
         P.diagnose(NameLoc, diag::pil_value_def_type_mismatch, Name,
                    Entry->getType().getAstType(),
                    Value->getType().getAstType());
         HadError = true;
      } else {
         // Forward references only live here if they have a single result.
         Entry->replaceAllUsesWith(Value);
      }
      Entry = Value;
      return;
   }

   // Otherwise, just store it in our map.
   Entry = Value;
}


//===----------------------------------------------------------------------===//
// PIL Parsing Logic
//===----------------------------------------------------------------------===//

/// parsePILLinkage - Parse a linkage specifier if present.
///   pil-linkage:
///     /*empty*/          // default depends on whether this is a definition
///     'public'
///     'hidden'
///     'shared'
///     'private'
///     'public_external'
///     'hidden_external'
///     'private_external'
static bool parsePILLinkage(Optional<PILLinkage> &Result, Parser &P) {
   // Begin by initializing result to our base value of None.
   Result = None;

   // Unfortunate collision with access control keywords.
   if (P.Tok.is(tok::kw_public)) {
      Result = PILLinkage::Public;
      P.consumeToken();
      return false;
   }

   // Unfortunate collision with access control keywords.
   if (P.Tok.is(tok::kw_private)) {
      Result = PILLinkage::Private;
      P.consumeToken();
      return false;
   }

   // If we do not have an identifier, bail. All PILLinkages that we are parsing
   // are identifiers.
   if (P.Tok.isNot(tok::identifier))
      return false;

   // Then use a string switch to try and parse the identifier.
   Result = llvm::StringSwitch<Optional<PILLinkage>>(P.Tok.getText())
      .Case("non_abi", PILLinkage::PublicNonABI)
      .Case("hidden", PILLinkage::Hidden)
      .Case("shared", PILLinkage::Shared)
      .Case("public_external", PILLinkage::PublicExternal)
      .Case("hidden_external", PILLinkage::HiddenExternal)
      .Case("shared_external", PILLinkage::SharedExternal)
      .Case("private_external", PILLinkage::PrivateExternal)
      .Default(None);

   // If we succeed, consume the token.
   if (Result) {
      P.consumeToken(tok::identifier);
   }

   return false;
}

/// Given whether it's known to be a definition, resolve an optional
/// PIL linkage to a real one.
static PILLinkage resolvePILLinkage(Optional<PILLinkage> linkage,
                                    bool isDefinition) {
   if (linkage.hasValue()) {
      return linkage.getValue();
   } else if (isDefinition) {
      return PILLinkage::DefaultForDefinition;
   } else {
      return PILLinkage::DefaultForDeclaration;
   }
}

static bool parsePILOptional(StringRef &Result, SourceLoc &Loc, PILParser &SP) {
   if (SP.P.consumeIf(tok::l_square)) {
      Identifier Id;
      SP.parsePILIdentifier(Id, Loc, diag::expected_in_attribute_list);
      SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
      Result = Id.str();
      return true;
   }
   return false;
}

static bool parsePILOptional(StringRef &Result, PILParser &SP) {
   SourceLoc Loc;
   return parsePILOptional(Result, Loc, SP);
}

/// Parse an option attribute ('[' Expected ']')?
static bool parsePILOptional(bool &Result, PILParser &SP, StringRef Expected) {
   StringRef Optional;
   if (parsePILOptional(Optional, SP)) {
      if (Optional != Expected)
         return true;
      Result = true;
   }
   return false;
}

namespace {
/// A helper class to perform lookup of IdentTypes in the
/// current parser scope.
class IdentTypeReprLookup : public AstWalker {
   Parser &P;
public:
   IdentTypeReprLookup(Parser &P) : P(P) {}

   bool walkToTypeReprPre(TypeRepr *Ty) override {
      auto *T = dyn_cast_or_null<IdentTypeRepr>(Ty);
      auto Comp = T->getComponentRange().front();
      if (auto Entry = P.lookupInScope(Comp->getIdentifier()))
         if (auto *TD = dyn_cast<TypeDecl>(Entry)) {
            Comp->setValue(TD, nullptr);
            return false;
         }
      return true;
   }
};
} // end anonymous namespace

/// Remap RequirementReps to Requirements.
void PILParser::convertRequirements(PILFunction *F,
                                    ArrayRef<RequirementRepr> From,
                                    SmallVectorImpl<Requirement> &To) {
   if (From.empty()) {
      To.clear();
      return;
   }

   auto *GenericEnv = F->getGenericEnvironment();
   assert(GenericEnv);
   (void)GenericEnv;

   IdentTypeReprLookup PerformLookup(P);
   // Use parser lexical scopes to resolve references
   // to the generic parameters.
   auto ResolveToInterfaceType = [&](TypeLoc Ty) -> Type {
      Ty.getTypeRepr()->walk(PerformLookup);
      performTypeLocChecking(Ty, /* IsPIL */ false);
      assert(Ty.getType());
      return Ty.getType()->mapTypeOutOfContext();
   };

   for (auto &Req : From) {
      if (Req.getKind() == RequirementReprKind::SameType) {
         auto FirstType = ResolveToInterfaceType(Req.getFirstTypeLoc());
         auto SecondType = ResolveToInterfaceType(Req.getSecondTypeLoc());
         Requirement ConvertedRequirement(RequirementKind::SameType, FirstType,
                                          SecondType);
         To.push_back(ConvertedRequirement);
         continue;
      }

      if (Req.getKind() == RequirementReprKind::TypeConstraint) {
         auto Subject = ResolveToInterfaceType(Req.getSubjectLoc());
         auto Constraint = ResolveToInterfaceType(Req.getConstraintLoc());
         Requirement ConvertedRequirement(RequirementKind::Conformance, Subject,
                                          Constraint);
         To.push_back(ConvertedRequirement);
         continue;
      }

      if (Req.getKind() == RequirementReprKind::LayoutConstraint) {
         auto Subject = ResolveToInterfaceType(Req.getSubjectLoc());
         Requirement ConvertedRequirement(RequirementKind::Layout, Subject,
                                          Req.getLayoutConstraint());
         To.push_back(ConvertedRequirement);
         continue;
      }
      llvm_unreachable("Unsupported requirement kind");
   }
}

static bool parseDeclPILOptional(bool *isTransparent,
                                 IsSerialized_t *isSerialized,
                                 bool *isCanonical,
                                 bool *hasOwnershipSSA,
                                 IsThunk_t *isThunk,
                                 IsDynamicallyReplaceable_t *isDynamic,
                                 IsExactSelfClass_t *isExactSelfClass,
                                 PILFunction **dynamicallyReplacedFunction,
                                 Identifier *objCReplacementFor,
                                 bool *isGlobalInit,
                                 Inline_t *inlineStrategy,
                                 OptimizationMode *optimizationMode,
                                 bool *isLet,
                                 bool *isWeakImported,
                                 AvailabilityContext *availability,
                                 bool *isWithoutActuallyEscapingThunk,
                                 SmallVectorImpl<std::string> *Semantics,
                                 SmallVectorImpl<ParsedSpecAttr> *SpecAttrs,
                                 ValueDecl **ClangDecl,
                                 EffectsKind *MRK, PILParser &SP,
                                 PILModule &M) {
   while (SP.P.consumeIf(tok::l_square)) {
      if (isLet && SP.P.Tok.is(tok::kw_let)) {
         *isLet = true;
         SP.P.consumeToken(tok::kw_let);
         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      }
      else if (SP.P.Tok.isNot(tok::identifier)) {
         SP.P.diagnose(SP.P.Tok, diag::expected_in_attribute_list);
         return true;
      } else if (isTransparent && SP.P.Tok.getText() == "transparent")
         *isTransparent = true;
      else if (isSerialized && SP.P.Tok.getText() == "serialized")
         *isSerialized = IsSerialized;
      else if (isDynamic && SP.P.Tok.getText() == "dynamically_replacable")
         *isDynamic = IsDynamic;
      else if (isExactSelfClass && SP.P.Tok.getText() == "exact_self_class")
         *isExactSelfClass = IsExactSelfClass;
      else if (isSerialized && SP.P.Tok.getText() == "serializable")
         *isSerialized = IsSerializable;
      else if (isCanonical && SP.P.Tok.getText() == "canonical")
         *isCanonical = true;
      else if (hasOwnershipSSA && SP.P.Tok.getText() == "ossa")
         *hasOwnershipSSA = true;
      else if (isThunk && SP.P.Tok.getText() == "thunk")
         *isThunk = IsThunk;
      else if (isThunk && SP.P.Tok.getText() == "signature_optimized_thunk")
         *isThunk = IsSignatureOptimizedThunk;
      else if (isThunk && SP.P.Tok.getText() == "reabstraction_thunk")
         *isThunk = IsReabstractionThunk;
      else if (isWithoutActuallyEscapingThunk
               && SP.P.Tok.getText() == "without_actually_escaping")
         *isWithoutActuallyEscapingThunk = true;
      else if (isGlobalInit && SP.P.Tok.getText() == "global_init")
         *isGlobalInit = true;
      else if (isWeakImported && SP.P.Tok.getText() == "weak_imported") {
         if (M.getAstContext().LangOpts.Target.isOSBinFormatCOFF())
            SP.P.diagnose(SP.P.Tok, diag::attr_unsupported_on_target,
                          SP.P.Tok.getText(),
                          M.getAstContext().LangOpts.Target.str());
         else
            *isWeakImported = true;
      } else if (availability && SP.P.Tok.getText() == "available") {
         SP.P.consumeToken(tok::identifier);

         SourceRange range;
         llvm::VersionTuple version;
         if (SP.P.parseVersionTuple(version, range,
                                    diag::pil_availability_expected_version))
            return true;

         *availability = AvailabilityContext(VersionRange::allGTE(version));

         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      } else if (inlineStrategy && SP.P.Tok.getText() == "noinline")
         *inlineStrategy = NoInline;
      else if (optimizationMode && SP.P.Tok.getText() == "Onone")
         *optimizationMode = OptimizationMode::NoOptimization;
      else if (optimizationMode && SP.P.Tok.getText() == "Ospeed")
         *optimizationMode = OptimizationMode::ForSpeed;
      else if (optimizationMode && SP.P.Tok.getText() == "Osize")
         *optimizationMode = OptimizationMode::ForSize;
      else if (inlineStrategy && SP.P.Tok.getText() == "always_inline")
         *inlineStrategy = AlwaysInline;
      else if (MRK && SP.P.Tok.getText() == "readnone")
         *MRK = EffectsKind::ReadNone;
      else if (MRK && SP.P.Tok.getText() == "readonly")
         *MRK = EffectsKind::ReadOnly;
      else if (MRK && SP.P.Tok.getText() == "readwrite")
         *MRK = EffectsKind::ReadWrite;
      else if (MRK && SP.P.Tok.getText() == "releasenone")
         *MRK = EffectsKind::ReleaseNone;
      else if  (dynamicallyReplacedFunction && SP.P.Tok.getText() == "dynamic_replacement_for") {
         SP.P.consumeToken(tok::identifier);
         if (SP.P.Tok.getKind() != tok::string_literal) {
            SP.P.diagnose(SP.P.Tok, diag::expected_in_attribute_list);
            return true;
         }
         // Drop the double quotes.
         StringRef replacedFunc = SP.P.Tok.getText().drop_front().drop_back();
         PILFunction *Func = M.lookUpFunction(replacedFunc.str());
         if (!Func) {
            Identifier Id = SP.P.Context.getIdentifier(replacedFunc);
            SP.P.diagnose(SP.P.Tok, diag::pil_dynamically_replaced_func_not_found,
                          Id);
            return true;
         }
         *dynamicallyReplacedFunction = Func;
         SP.P.consumeToken(tok::string_literal);

         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      } else if (objCReplacementFor &&
                 SP.P.Tok.getText() == "objc_replacement_for") {
         SP.P.consumeToken(tok::identifier);
         if (SP.P.Tok.getKind() != tok::string_literal) {
            SP.P.diagnose(SP.P.Tok, diag::expected_in_attribute_list);
            return true;
         }
         // Drop the double quotes.
         StringRef replacedFunc = SP.P.Tok.getText().drop_front().drop_back();
         *objCReplacementFor = SP.P.Context.getIdentifier(replacedFunc);
         SP.P.consumeToken(tok::string_literal);

         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      } else if (Semantics && SP.P.Tok.getText() == "_semantics") {
         SP.P.consumeToken(tok::identifier);
         if (SP.P.Tok.getKind() != tok::string_literal) {
            SP.P.diagnose(SP.P.Tok, diag::expected_in_attribute_list);
            return true;
         }

         // Drop the double quotes.
         StringRef rawString = SP.P.Tok.getText().drop_front().drop_back();
         Semantics->push_back(rawString);
         SP.P.consumeToken(tok::string_literal);

         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      } else if (SpecAttrs && SP.P.Tok.getText() == "_specialize") {
         SourceLoc AtLoc = SP.P.Tok.getLoc();
         SourceLoc Loc(AtLoc);

         // Parse a _specialized attribute, building a parsed substitution list
         // and pushing a new ParsedSpecAttr on the SpecAttrs list. Conformances
         // cannot be generated until the function declaration is fully parsed so
         // that the function's generic signature can be consulted.
         ParsedSpecAttr SpecAttr;
         SpecAttr.requirements = {};
         SpecAttr.exported = false;
         SpecAttr.kind = PILSpecializeAttr::SpecializationKind::Full;
         SpecializeAttr *Attr;

         if (!SP.P.parseSpecializeAttribute(tok::r_square, AtLoc, Loc, Attr))
            return true;

         // Convert SpecializeAttr into ParsedSpecAttr.
         SpecAttr.requirements = Attr->getTrailingWhereClause()->getRequirements();
         SpecAttr.kind = Attr->getSpecializationKind() ==
                         polar::SpecializeAttr::SpecializationKind::Full
                         ? PILSpecializeAttr::SpecializationKind::Full
                         : PILSpecializeAttr::SpecializationKind::Partial;
         SpecAttr.exported = Attr->isExported();
         SpecAttrs->emplace_back(SpecAttr);
         continue;
      }
      else if (ClangDecl && SP.P.Tok.getText() == "clang") {
         SP.P.consumeToken(tok::identifier);
         if (SP.parsePILDottedPathWithoutPound(*ClangDecl))
            return true;

         SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         continue;
      }
      else {
         SP.P.diagnose(SP.P.Tok, diag::expected_in_attribute_list);
         return true;
      }
      SP.P.consumeToken(tok::identifier);
      SP.P.parseToken(tok::r_square, diag::expected_in_attribute_list);
   }
   return false;
}

bool PILParser::performTypeLocChecking(TypeLoc &T, bool IsPILType,
                                       GenericEnvironment *GenericEnv,
                                       DeclContext *DC) {
   // Do some type checking / name binding for the parsed type.
   assert(P.SF.AstStage == SourceFile::Parsing &&
          "Unexpected stage during parsing!");

   if (GenericEnv == nullptr)
      GenericEnv = ContextGenericEnv;

   if (!DC)
      DC = &P.SF;
   else if (!GenericEnv)
      GenericEnv = DC->getGenericEnvironmentOfContext();

   return polar::performTypeLocChecking(P.Context, T,
      /*isPILMode=*/true, IsPILType,
                                        GenericEnv, DC);
}

/// Find the top-level ValueDecl or Module given a name.
static llvm::PointerUnion<ValueDecl *, ModuleDecl *>
lookupTopDecl(Parser &P, DeclBaseName Name, bool typeLookup) {
   // Use UnqualifiedLookup to look through all of the imports.
   // We have to lie and say we're done with parsing to make this happen.
   assert(P.SF.AstStage == SourceFile::Parsing &&
          "Unexpected stage during parsing!");
   llvm::SaveAndRestore<SourceFile::AstStageType> AstStage(P.SF.AstStage,
                                                         SourceFile::Parsed);

   UnqualifiedLookupOptions options;
   if (typeLookup)
      options |= UnqualifiedLookupFlags::TypeLookup;

   auto &ctx = P.SF.getAstContext();
   auto descriptor = UnqualifiedLookupDescriptor(Name, &P.SF);
   auto lookup = evaluateOrDefault(ctx.evaluator,
                                   UnqualifiedLookupRequest{descriptor}, {});
   assert(lookup.size() == 1);
   return lookup.back().getValueDecl();
}

/// Find the ValueDecl given an interface type and a member name.
static ValueDecl *lookupMember(Parser &P, Type Ty, DeclBaseName Name,
                               SourceLoc Loc,
                               SmallVectorImpl<ValueDecl *> &Lookup,
                               bool ExpectMultipleResults) {
   Type CheckTy = Ty;
   if (auto MetaTy = CheckTy->getAs<AnyMetatypeType>())
      CheckTy = MetaTy->getInstanceType();

   if (auto nominal = CheckTy->getAnyNominal()) {
      if (Name == DeclBaseName::createDestructor() &&
          isa<ClassDecl>(nominal)) {
         auto *classDecl = cast<ClassDecl>(nominal);
         Lookup.push_back(classDecl->getDestructor());
      } else {
         auto found = nominal->lookupDirect(Name);
         Lookup.append(found.begin(), found.end());
      }
   } else if (auto moduleTy = CheckTy->getAs<ModuleType>()) {
      moduleTy->getModule()->lookupValue(Name, NLKind::QualifiedLookup, Lookup);
   } else {
      P.diagnose(Loc, diag::pil_member_lookup_bad_type, Name, Ty);
      return nullptr;
   }

   if (Lookup.empty() || (!ExpectMultipleResults && Lookup.size() != 1)) {
      P.diagnose(Loc, diag::pil_named_member_decl_not_found, Name, Ty);
      return nullptr;
   }
   return Lookup[0];
}

bool PILParser::parseASTType(CanType &result, GenericEnvironment *env) {
   ParserResult<TypeRepr> parsedType = P.parseType();
   if (parsedType.isNull()) return true;
   TypeLoc loc = parsedType.get();
   if (performTypeLocChecking(loc, /*IsPILType=*/ false, env))
      return true;

   if (env)
      result = loc.getType()->mapTypeOutOfContext()->getCanonicalType();
   else
      result = loc.getType()->getCanonicalType();

   // Invoke the callback on the parsed type.
   ParsedTypeCallback(loc.getType());
   return false;
}

///   pil-type:
///     '$' '*'? attribute-list (generic-params)? type
///
bool PILParser::parsePILType(PILType &Result,
                             GenericEnvironment *&ParsedGenericEnv,
                             bool IsFuncDecl,
                             GenericEnvironment *OuterGenericEnv) {
   ParsedGenericEnv = nullptr;

   if (P.parseToken(tok::pil_dollar, diag::expected_pil_type))
      return true;

   // If we have a '*', then this is an address type.
   PILValueCategory category = PILValueCategory::Object;
   if (P.Tok.isAnyOperator() && P.Tok.getText().startswith("*")) {
      category = PILValueCategory::Address;
      P.consumeStartingCharacterOfCurrentToken();
   }

   // Parse attributes.
   ParamDecl::Specifier specifier;
   SourceLoc specifierLoc;
   TypeAttributes attrs;
   P.parseTypeAttributeList(specifier, specifierLoc, attrs);

   // Global functions are implicitly @convention(thin) if not specified otherwise.
   if (IsFuncDecl && !attrs.has(TAK_convention)) {
      // Use a random location.
      attrs.setAttr(TAK_convention, P.PreviousLoc);
      attrs.convention = "thin";
   }

   ParserResult<TypeRepr> TyR = P.parseType(diag::expected_pil_type,
      /*handleCodeCompletion*/ true,
      /*isPILFuncDecl*/ IsFuncDecl);

   if (TyR.isNull())
      return true;

   // Resolve the generic environments for parsed generic function and box types.
   class HandlePILGenericParamsWalker : public AstWalker {
      SourceFile *SF;
   public:
      HandlePILGenericParamsWalker(SourceFile *SF) : SF(SF) {}

      bool walkToTypeReprPre(TypeRepr *T) override {
         if (auto fnType = dyn_cast<FunctionTypeRepr>(T)) {
            if (auto generics = fnType->getGenericParams()) {
               auto env = handlePILGenericParams(generics, SF);
               fnType->setGenericEnvironment(env);
            }
         }
         if (auto boxType = dyn_cast<PILBoxTypeRepr>(T)) {
            if (auto generics = boxType->getGenericParams()) {
               auto env = handlePILGenericParams(generics, SF);
               boxType->setGenericEnvironment(env);
            }
         }
         return true;
      }
   };

   TyR.get()->walk(HandlePILGenericParamsWalker(&P.SF));

   // Save the top-level function generic environment if there was one.
   if (auto fnType = dyn_cast<FunctionTypeRepr>(TyR.get()))
      if (auto env = fnType->getGenericEnvironment())
         ParsedGenericEnv = env;

   // Apply attributes to the type.
   TypeLoc Ty = P.applyAttributeToType(TyR.get(), attrs, specifier, specifierLoc);

   if (performTypeLocChecking(Ty, /*IsPILType=*/true, OuterGenericEnv))
      return true;

   Result = PILType::getPrimitiveType(Ty.getType()->getCanonicalType(),
                                      category);

   // Invoke the callback on the parsed type.
   ParsedTypeCallback(Ty.getType());

   return false;
}

bool PILParser::parsePILDottedPath(ValueDecl *&Decl,
                                   SmallVectorImpl<ValueDecl *> &values) {
   if (P.parseToken(tok::pound, diag::expected_pil_constant))
      return true;
   return parsePILDottedPathWithoutPound(Decl, values);
}

bool PILParser::parsePILDottedPathWithoutPound(ValueDecl *&Decl,
                                               SmallVectorImpl<ValueDecl *> &values) {
   // Handle pil-dotted-path.
   Identifier Id;
   SmallVector<DeclBaseName, 4> FullName;
   SmallVector<SourceLoc, 4> Locs;
   do {
      Locs.push_back(P.Tok.getLoc());
      switch (P.Tok.getKind()) {
         case tok::kw_subscript:
            P.consumeToken();
            FullName.push_back(DeclBaseName::createSubscript());
            break;
         case tok::kw_init:
            P.consumeToken();
            FullName.push_back(DeclBaseName::createConstructor());
            break;
         case tok::kw_deinit:
            P.consumeToken();
            FullName.push_back(DeclBaseName::createDestructor());
            break;
         default:
            if (parsePILIdentifier(Id, diag::expected_pil_constant))
               return true;
            FullName.push_back(Id);
            break;
      }
   } while (P.consumeIf(tok::period));

   // Look up ValueDecl from a dotted path. If there are multiple components,
   // the first one must be a type declaration.
   ValueDecl *VD;
   llvm::PointerUnion<ValueDecl*, ModuleDecl *> Res = lookupTopDecl(
      P, FullName[0], /*typeLookup=*/FullName.size() > 1);
   // It is possible that the last member lookup can return multiple lookup
   // results. One example is the overloaded member functions.
   if (Res.is<ModuleDecl*>()) {
      assert(FullName.size() > 1 &&
             "A single module is not a full path to PILDeclRef");
      auto Mod = Res.get<ModuleDecl*>();
      values.clear();
      VD = lookupMember(P, ModuleType::get(Mod), FullName[1], Locs[1], values,
                        FullName.size() == 2/*ExpectMultipleResults*/);
      for (unsigned I = 2, E = FullName.size(); I < E; I++) {
         values.clear();
         VD = lookupMember(P, VD->getInterfaceType(), FullName[I], Locs[I], values,
                           I == FullName.size() - 1/*ExpectMultipleResults*/);
      }
   } else {
      VD = Res.get<ValueDecl*>();
      for (unsigned I = 1, E = FullName.size(); I < E; I++) {
         values.clear();
         VD = lookupMember(P, VD->getInterfaceType(), FullName[I], Locs[I], values,
                           I == FullName.size() - 1/*ExpectMultipleResults*/);
      }
   }
   Decl = VD;
   return false;
}

static Optional<AccessorKind> getAccessorKind(StringRef ident) {
   return llvm::StringSwitch<Optional<AccessorKind>>(ident)
      .Case("getter", AccessorKind::Get)
      .Case("setter", AccessorKind::Set)
      .Case("addressor", AccessorKind::Address)
      .Case("mutableAddressor", AccessorKind::MutableAddress)
      .Case("read", AccessorKind::Read)
      .Case("modify", AccessorKind::Modify)
      .Default(None);
}

///  pil-decl-ref ::= '#' pil-identifier ('.' pil-identifier)* pil-decl-subref?
///  pil-decl-subref ::= '!' pil-decl-subref-part ('.' pil-decl-uncurry-level)?
///                      ('.' pil-decl-lang)?
///  pil-decl-subref ::= '!' pil-decl-uncurry-level ('.' pil-decl-lang)?
///  pil-decl-subref ::= '!' pil-decl-lang
///  pil-decl-subref-part ::= 'getter'
///  pil-decl-subref-part ::= 'setter'
///  pil-decl-subref-part ::= 'allocator'
///  pil-decl-subref-part ::= 'initializer'
///  pil-decl-subref-part ::= 'enumelt'
///  pil-decl-subref-part ::= 'destroyer'
///  pil-decl-subref-part ::= 'globalaccessor'
///  pil-decl-uncurry-level ::= [0-9]+
///  pil-decl-lang ::= 'foreign'
bool PILParser::parsePILDeclRef(PILDeclRef &Result,
                                SmallVectorImpl<ValueDecl *> &values) {
   ValueDecl *VD;
   if (parsePILDottedPath(VD, values))
      return true;

   // Initialize Kind, uncurryLevel and IsObjC.
   PILDeclRef::Kind Kind = PILDeclRef::Kind::Func;
   unsigned uncurryLevel = 0;
   bool IsObjC = false;

   if (!P.consumeIf(tok::pil_exclamation)) {
      // Construct PILDeclRef.
      Result = PILDeclRef(VD, Kind, /*isCurried=*/false, IsObjC);
      if (uncurryLevel < Result.getParameterListCount() - 1)
         Result = Result.asCurried();
      return false;
   }

   // Handle pil-constant-kind-and-uncurry-level.
   // ParseState indicates the value we just handled.
   // 1 means we just handled Kind, 2 means we just handled uncurryLevel.
   // We accept func|getter|setter|...|foreign or an integer when ParseState is
   // 0; accept foreign or an integer when ParseState is 1; accept foreign when
   // ParseState is 2.
   unsigned ParseState = 0;
   Identifier Id;
   do {
      if (P.Tok.is(tok::identifier)) {
         auto IdLoc = P.Tok.getLoc();
         if (parsePILIdentifier(Id, diag::expected_pil_constant))
            return true;
         Optional<AccessorKind> accessorKind;
         if (!ParseState && Id.str() == "func") {
            Kind = PILDeclRef::Kind::Func;
            ParseState = 1;
         } else if (!ParseState &&
                    (accessorKind = getAccessorKind(Id.str())).hasValue()) {
            // Drill down to the corresponding accessor for each declaration,
            // compacting away decls that lack it.
            size_t destI = 0;
            for (size_t srcI = 0, e = values.size(); srcI != e; ++srcI) {
               if (auto storage = dyn_cast<AbstractStorageDecl>(values[srcI]))
                  if (auto accessor = storage->getOpaqueAccessor(*accessorKind))
                     values[destI++] = accessor;
            }
            values.resize(destI);

            // Complain if none of the decls had a corresponding accessor.
            if (destI == 0) {
               P.diagnose(IdLoc, diag::referenced_value_no_accessor, 0);
               return true;
            }

            Kind = PILDeclRef::Kind::Func;
            VD = values[0];
            ParseState = 1;
         } else if (!ParseState && Id.str() == "allocator") {
            Kind = PILDeclRef::Kind::Allocator;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "initializer") {
            Kind = PILDeclRef::Kind::Initializer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "enumelt") {
            Kind = PILDeclRef::Kind::EnumElement;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "destroyer") {
            Kind = PILDeclRef::Kind::Destroyer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "deallocator") {
            Kind = PILDeclRef::Kind::Deallocator;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "globalaccessor") {
            Kind = PILDeclRef::Kind::GlobalAccessor;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "ivardestroyer") {
            Kind = PILDeclRef::Kind::IVarDestroyer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "ivarinitializer") {
            Kind = PILDeclRef::Kind::IVarInitializer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "defaultarg") {
            Kind = PILDeclRef::Kind::IVarInitializer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "propertyinit") {
            Kind = PILDeclRef::Kind::StoredPropertyInitializer;
            ParseState = 1;
         } else if (!ParseState && Id.str() == "backinginit") {
            Kind = PILDeclRef::Kind::PropertyWrapperBackingInitializer;
            ParseState = 1;
         } else if (Id.str() == "foreign") {
            IsObjC = true;
            break;
         } else
            break;
      } else if (ParseState < 2 && P.Tok.is(tok::integer_literal)) {
         parseIntegerLiteral(P.Tok.getText(), 0, uncurryLevel);
         P.consumeToken(tok::integer_literal);
         ParseState = 2;
      } else
         break;

   } while (P.consumeIf(tok::period));

   // Construct PILDeclRef.
   Result = PILDeclRef(VD, Kind, /*isCurried=*/false, IsObjC);
   if (uncurryLevel < Result.getParameterListCount() - 1)
      Result = Result.asCurried();
   return false;
}

/// parseValueName - Parse a value name without a type available yet.
///
///     pil-value-name:
///       pil-local-name
///       'undef'
///
bool PILParser::parseValueName(UnresolvedValueName &Result) {
   Result.Name = P.Tok.getText();

   if (P.Tok.is(tok::kw_undef)) {
      Result.NameLoc = P.consumeToken(tok::kw_undef);
      return false;
   }

   // Parse the local-name.
   if (P.parseToken(tok::pil_local_name, Result.NameLoc,
                    diag::expected_pil_value_name))
      return true;

   return false;
}

/// parseValueRef - Parse a value, given a contextual type.
///
///     pil-value-ref:
///       pil-local-name
///
bool PILParser::parseValueRef(PILValue &Result, PILType Ty,
                              PILLocation Loc, PILBuilder &B) {
   UnresolvedValueName Name;
   if (parseValueName(Name)) return true;
   Result = getLocalValue(Name, Ty, Loc, B);
   return false;
}

/// parseTypedValueRef - Parse a type/value reference pair.
///
///    pil-typed-valueref:
///       pil-value-ref ':' pil-type
///
bool PILParser::parseTypedValueRef(PILValue &Result, SourceLoc &Loc,
                                   PILBuilder &B) {
   Loc = P.Tok.getLoc();

   UnresolvedValueName Name;
   PILType Ty;
   if (parseValueName(Name) ||
       P.parseToken(tok::colon, diag::expected_pil_colon_value_ref) ||
       parsePILType(Ty))
      return true;

   Result = getLocalValue(Name, Ty, RegularLocation(Loc), B);
   return false;
}

/// Look up whether the given string corresponds to a PIL opcode.
static Optional<PILInstructionKind> getOpcodeByName(StringRef OpcodeName) {
   return llvm::StringSwitch<Optional<PILInstructionKind>>(OpcodeName)
   #define FULL_INST(Id, TextualName, Parent, MemBehavior, MayRelease) \
    .Case(#TextualName, PILInstructionKind::Id)
   #include "polarphp/pil/lang/PILNodesDef.h"
      .Default(None);
}

/// getInstructionKind - This method maps the string form of a PIL instruction
/// opcode to an enum.
bool PILParser::parsePILOpcode(PILInstructionKind &Opcode, SourceLoc &OpcodeLoc,
                               StringRef &OpcodeName) {
   OpcodeLoc = P.Tok.getLoc();
   OpcodeName = P.Tok.getText();
   // Parse this textually to avoid Swift keywords (like 'return') from
   // interfering with opcode recognition.
   auto MaybeOpcode = getOpcodeByName(OpcodeName);
   if (!MaybeOpcode) {
      P.diagnose(OpcodeLoc, diag::expected_pil_instr_opcode);
      return true;
   }

   Opcode = MaybeOpcode.getValue();
   P.consumeToken();
   return false;
}

static bool peekPILDebugLocation(Parser &P) {
   auto T = P.peekToken().getText();
   return P.Tok.is(tok::comma) && (T == "loc" || T == "scope");
}

bool PILParser::parsePILDebugVar(PILDebugVariable &Var) {
   while (P.Tok.is(tok::comma) && !peekPILDebugLocation(P)) {
      P.consumeToken();
      StringRef Key = P.Tok.getText();
      if (Key == "name") {
         P.consumeToken();
         if (P.Tok.getKind() != tok::string_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "string");
            return true;
         }
         // Drop the double quotes.
         StringRef Val = P.Tok.getText().drop_front().drop_back();
         Var.Name = Val;
      } else if (Key == "argno") {
         P.consumeToken();
         if (P.Tok.getKind() != tok::integer_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "integer");
            return true;
         }
         uint16_t ArgNo;
         if (parseIntegerLiteral(P.Tok.getText(), 0, ArgNo))
            return true;
         Var.ArgNo = ArgNo;
      } else if (Key == "let") {
         Var.Constant = true;
      } else if (Key == "var") {
         Var.Constant = false;
      } else if (Key == "loc") {
         Var.Constant = false;
      } else {
         P.diagnose(P.Tok, diag::pil_dbg_unknown_key, Key);
         return true;
      }
      P.consumeToken();
   }
   return false;
}

bool PILParser::parsePILBBArgsAtBranch(SmallVector<PILValue, 6> &Args,
                                       PILBuilder &B) {
   if (P.Tok.is(tok::l_paren)) {
      SourceLoc LParenLoc = P.consumeToken(tok::l_paren);
      SourceLoc RParenLoc;

      if (P.parseList(tok::r_paren, LParenLoc, RParenLoc,
         /*AllowSepAfterLast=*/false,
                      diag::pil_basicblock_arg_rparen,
                      SyntaxKind::Unknown,
                      [&]() -> ParserStatus {
                         PILValue Arg;
                         SourceLoc ArgLoc;
                         if (parseTypedValueRef(Arg, ArgLoc, B))
                            return makeParserError();
                         Args.push_back(Arg);
                         return makeParserSuccess();
                      }).isError())
         return true;
   }
   return false;
}

/// Bind any unqualified 'Self' references to the given protocol's 'Self'
/// generic parameter.
///
/// FIXME: This is a hack to work around the lack of a DeclContext for
/// witness tables.
static void bindInterfaceSelfInTypeRepr(TypeLoc &TL, InterfaceDecl *proto) {
   if (auto typeRepr = TL.getTypeRepr()) {
      // AST walker to update 'Self' references.
      class BindInterfaceSelf : public AstWalker {
         InterfaceDecl *proto;
         GenericTypeParamDecl *selfParam;
         Identifier selfId;

      public:
         BindInterfaceSelf(InterfaceDecl *proto)
            : proto(proto),
              selfParam(proto->getInterfaceSelfType()->getDecl()),
              selfId(proto->getAstContext().Id_Self) {
         }

         virtual bool walkToTypeReprPre(TypeRepr *T) override {
            if (auto ident = dyn_cast<IdentTypeRepr>(T)) {
               auto firstComponent = ident->getComponentRange().front();
               if (firstComponent->getIdentifier() == selfId)
                  firstComponent->setValue(selfParam, proto);
            }

            return true;
         }
      };

      typeRepr->walk(BindInterfaceSelf(proto));
   }
}

/// Parse the substitution list for an apply instruction or
/// specialized protocol conformance.
bool PILParser::parseSubstitutions(SmallVectorImpl<ParsedSubstitution> &parsed,
                                   GenericEnvironment *GenericEnv,
                                   InterfaceDecl *defaultForProto) {
   // Check for an opening '<' bracket.
   if (!P.Tok.isContextualPunctuator("<"))
      return false;

   P.consumeToken();

   // Parse a list of Substitutions.
   do {
      SourceLoc Loc = P.Tok.getLoc();

      // Parse substitution as AST type.
      ParserResult<TypeRepr> TyR = P.parseType();
      if (TyR.isNull())
         return true;
      TypeLoc Ty = TyR.get();
      if (defaultForProto)
         bindInterfaceSelfInTypeRepr(Ty, defaultForProto);
      if (performTypeLocChecking(Ty, /*IsPILType=*/ false, GenericEnv,
                                 defaultForProto))
         return true;
      parsed.push_back({Loc, Ty.getType()});
   } while (P.consumeIf(tok::comma));

   // Consume the closing '>'.
   if (!P.Tok.isContextualPunctuator(">")) {
      P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, ">");
      return true;
   }
   P.consumeToken();

   return false;
}

/// Collect conformances by looking up the conformance from replacement
/// type and protocol decl.
static bool getConformancesForSubstitution(Parser &P,
                                           ExistentialLayout::InterfaceTypeArrayRef protocols,
                                           Type subReplacement,
                                           SourceLoc loc,
                                           SmallVectorImpl<InterfaceConformanceRef> &conformances) {
   auto M = P.SF.getParentModule();

   for (auto protoTy : protocols) {
      auto conformance = M->lookupConformance(subReplacement,
                                              protoTy->getDecl());
      if (conformance.isInvalid()) {
         P.diagnose(loc, diag::pil_substitution_mismatch, subReplacement, protoTy);
         return true;
      }
      conformances.push_back(conformance);
   }

   return false;
}

/// Reconstruct an AST substitution map from parsed substitutions.
SubstitutionMap getApplySubstitutionsFromParsed(
   PILParser &SP,
   GenericEnvironment *env,
   ArrayRef<ParsedSubstitution> parses) {
   if (parses.empty()) {
      assert(!env);
      return SubstitutionMap();
   }

   assert(env);

   auto loc = parses[0].loc;

   // Ensure that we have the right number of type arguments.
   auto genericSig = env->getGenericSignature();
   if (parses.size() != genericSig->getGenericParams().size()) {
      bool hasTooFew = parses.size() < genericSig->getGenericParams().size();
      SP.P.diagnose(loc,
                    hasTooFew ? diag::pil_missing_substitutions
                              : diag::pil_too_many_substitutions);
      return SubstitutionMap();
   }

   bool failed = false;
   auto subMap = SubstitutionMap::get(
      genericSig,
      [&](SubstitutableType *type) -> Type {
         auto genericParam = dyn_cast<GenericTypeParamType>(type);
         if (!genericParam)
            return nullptr;

         auto index = genericSig->getGenericParamOrdinal(genericParam);
         assert(index < genericSig->getGenericParams().size());
         assert(index < parses.size());

         // Provide the replacement type.
         return parses[index].replacement;
      },
      [&](CanType dependentType, Type replacementType,
          InterfaceDecl *proto) -> InterfaceConformanceRef {
         auto M = SP.P.SF.getParentModule();
         if (auto conformance = M->lookupConformance(replacementType, proto))
            return conformance;

         SP.P.diagnose(loc, diag::pil_substitution_mismatch, replacementType,
                       proto->getDeclaredType());
         failed = true;

         return InterfaceConformanceRef(proto);
      });

   return failed ? SubstitutionMap() : subMap;
}

static ArrayRef<InterfaceConformanceRef>
collectExistentialConformances(Parser &P, CanType conformingType, SourceLoc loc,
                               CanType protocolType) {
   auto layout = protocolType.getExistentialLayout();

   if (layout.requiresClass()) {
      if (!conformingType->mayHaveSuperclass() &&
          !conformingType->isObjCExistentialType()) {
         P.diagnose(loc, diag::pil_not_class, conformingType);
      }
   }

   // FIXME: Check superclass also.

   auto protocols = layout.getInterfaces();
   if (protocols.empty())
      return {};

   SmallVector<InterfaceConformanceRef, 2> conformances;
   getConformancesForSubstitution(P, protocols, conformingType,
                                  loc, conformances);

   return P.Context.AllocateCopy(conformances);
}

/// pil-loc ::= 'loc' string-literal ':' [0-9]+ ':' [0-9]+
bool PILParser::parsePILLocation(PILLocation &Loc) {
   PILLocation::DebugLoc L;
   if (parseVerbatim("loc"))
      return true;

   if (P.Tok.getKind() != tok::string_literal) {
      P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "string");
      return true;
   }
   // Drop the double quotes.
   StringRef File = P.Tok.getText().drop_front().drop_back();
   L.Filename = P.Context.getIdentifier(File).str().data();
   P.consumeToken(tok::string_literal);
   if (P.parseToken(tok::colon, diag::expected_colon_in_pil_location))
      return true;
   if (parseInteger(L.Line, diag::pil_invalid_line_in_pil_location))
      return true;
   if (P.parseToken(tok::colon, diag::expected_colon_in_pil_location))
      return true;
   if (parseInteger(L.Column, diag::pil_invalid_column_in_pil_location))
      return true;

   Loc.setDebugInfoLoc(L);
   return false;
}

bool PILParser::parseScopeRef(PILDebugScope *&DS) {
   unsigned Slot;
   SourceLoc SlotLoc = P.Tok.getLoc();
   if (parseInteger(Slot, diag::pil_invalid_scope_slot))
      return true;

   DS = TUState.ScopeSlots[Slot];
   if (!DS) {
      P.diagnose(SlotLoc, diag::pil_scope_undeclared, Slot);
      return true;
   }
   return false;
}

///  (',' pil-loc)? (',' pil-scope-ref)?
bool PILParser::parsePILDebugLocation(PILLocation &L, PILBuilder &B,
                                      bool parsedComma) {
   // Parse the debug information, if any.
   if (P.Tok.is(tok::comma)) {
      P.consumeToken();
      parsedComma = true;
   }
   if (!parsedComma)
      return false;

   bool requireScope = false;
   if (P.Tok.getText() == "loc") {
      if (parsePILLocation(L))
         return true;

      if (P.Tok.is(tok::comma)) {
         P.consumeToken();
         requireScope = true;
      }
   }
   if (P.Tok.getText() == "scope" || requireScope) {
      parseVerbatim("scope");
      PILDebugScope *DS = nullptr;
      if (parseScopeRef(DS))
         return true;
      if (DS)
         B.setCurrentDebugScope(DS);
   }
   return false;
}

static bool parseLoadOwnershipQualifier(LoadOwnershipQualifier &Result,
                                        PILParser &P) {
   StringRef Str;
   // If we do not parse '[' ... ']', we have unqualified. Set value and return.
   if (!parsePILOptional(Str, P)) {
      Result = LoadOwnershipQualifier::Unqualified;
      return false;
   }

   // Then try to parse one of our other qualifiers. We do not support parsing
   // unqualified here so we use that as our fail value.
   auto Tmp = llvm::StringSwitch<LoadOwnershipQualifier>(Str)
      .Case("take", LoadOwnershipQualifier::Take)
      .Case("copy", LoadOwnershipQualifier::Copy)
      .Case("trivial", LoadOwnershipQualifier::Trivial)
      .Default(LoadOwnershipQualifier::Unqualified);

   // Thus return true (following the conventions in this file) if we fail.
   if (Tmp == LoadOwnershipQualifier::Unqualified)
      return true;

   // Otherwise, assign Result and return false.
   Result = Tmp;
   return false;
}

static bool parseStoreOwnershipQualifier(StoreOwnershipQualifier &Result,
                                         PILParser &P) {
   StringRef Str;
   // If we do not parse '[' ... ']', we have unqualified. Set value and return.
   if (!parsePILOptional(Str, P)) {
      Result = StoreOwnershipQualifier::Unqualified;
      return false;
   }

   // Then try to parse one of our other qualifiers. We do not support parsing
   // unqualified here so we use that as our fail value.
   auto Tmp = llvm::StringSwitch<StoreOwnershipQualifier>(Str)
      .Case("init", StoreOwnershipQualifier::Init)
      .Case("assign", StoreOwnershipQualifier::Assign)
      .Case("trivial", StoreOwnershipQualifier::Trivial)
      .Default(StoreOwnershipQualifier::Unqualified);

   // Thus return true (following the conventions in this file) if we fail.
   if (Tmp == StoreOwnershipQualifier::Unqualified)
      return true;

   // Otherwise, assign Result and return false.
   Result = Tmp;
   return false;
}

static bool parseAssignOwnershipQualifier(AssignOwnershipQualifier &Result,
                                          PILParser &P) {
   StringRef Str;
   // If we do not parse '[' ... ']', we have unknown. Set value and return.
   if (!parsePILOptional(Str, P)) {
      Result = AssignOwnershipQualifier::Unknown;
      return false;
   }

   // Then try to parse one of our other initialization kinds. We do not support
   // parsing unknown here so we use that as our fail value.
   auto Tmp = llvm::StringSwitch<AssignOwnershipQualifier>(Str)
      .Case("reassign", AssignOwnershipQualifier::Reassign)
      .Case("reinit", AssignOwnershipQualifier::Reinit)
      .Case("init", AssignOwnershipQualifier::Init)
      .Default(AssignOwnershipQualifier::Unknown);

   // Thus return true (following the conventions in this file) if we fail.
   if (Tmp == AssignOwnershipQualifier::Unknown)
      return true;

   // Otherwise, assign Result and return false.
   Result = Tmp;
   return false;
}

bool PILParser::parsePILDeclRef(PILDeclRef &Member, bool FnTypeRequired) {
   SourceLoc TyLoc;
   SmallVector<ValueDecl *, 4> values;
   if (parsePILDeclRef(Member, values))
      return true;

   // : ( or : < means that what follows is function type.
   if (!P.Tok.is(tok::colon))
      return false;

   if (FnTypeRequired &&
       !P.peekToken().is(tok::l_paren) &&
       !P.peekToken().isContextualPunctuator("<"))
      return false;

   // Type of the PILDeclRef is optional to be compatible with the old format.
   if (!P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":")) {
      // Parse the type for PILDeclRef.
      Optional<Scope> GenericsScope;
      GenericsScope.emplace(&P, ScopeKind::Generics);
      ParserResult<TypeRepr> TyR = P.parseType();
      GenericsScope.reset();
      if (TyR.isNull())
         return true;
      TypeLoc Ty = TyR.get();

      // The type can be polymorphic.
      GenericEnvironment *genericEnv = nullptr;
      if (auto fnType = dyn_cast<FunctionTypeRepr>(TyR.get())) {
         if (auto generics = fnType->getGenericParams()) {
            assert(!Ty.wasValidated() && Ty.getType().isNull());

            genericEnv = handlePILGenericParams(generics, &P.SF);
            fnType->setGenericEnvironment(genericEnv);
         }
      }

      if (performTypeLocChecking(Ty, /*IsPILType=*/ false, genericEnv))
         return true;

      // Pick the ValueDecl that has the right type.
      ValueDecl *TheDecl = nullptr;
      auto declTy = Ty.getType()->getCanonicalType();
      for (unsigned I = 0, E = values.size(); I < E; I++) {
         auto *decl = values[I];

         auto lookupTy =
            decl->getInterfaceType()
               ->removeArgumentLabels(decl->getNumCurryLevels());
         if (declTy == lookupTy->getCanonicalType()) {
            TheDecl = decl;
            // Update PILDeclRef to point to the right Decl.
            Member.loc = decl;
            break;
         }
         if (values.size() == 1 && !TheDecl) {
            P.diagnose(TyLoc, diag::pil_member_decl_type_mismatch, declTy,
                       lookupTy);
            return true;
         }
      }
      if (!TheDecl) {
         P.diagnose(TyLoc, diag::pil_member_decl_not_found);
         return true;
      }
   }
   return false;
}

bool
PILParser::parseKeyPathPatternComponent(KeyPathPatternComponent &component,
                                        SmallVectorImpl<PILType> &operandTypes,
                                        SourceLoc componentLoc,
                                        Identifier componentKind,
                                        PILLocation InstLoc,
                                        GenericEnvironment *patternEnv) {
   auto parseComponentIndices =
      [&](SmallVectorImpl<KeyPathPatternComponent::Index> &indexes) -> bool {
         while (true) {
            unsigned index;
            CanType formalTy;
            PILType loweredTy;
            if (P.parseToken(tok::oper_prefix,
                             diag::expected_tok_in_pil_instr, "%")
                || P.parseToken(tok::pil_dollar,
                                diag::expected_tok_in_pil_instr, "$"))
               return true;

            if (!P.Tok.is(tok::integer_literal)
                || parseIntegerLiteral(P.Tok.getText(), 0, index))
               return true;

            P.consumeToken(tok::integer_literal);

            SourceLoc formalTyLoc;
            SourceLoc loweredTyLoc;
            GenericEnvironment *ignoredParsedEnv;
            if (P.parseToken(tok::colon,
                             diag::expected_tok_in_pil_instr, ":")
                || P.parseToken(tok::pil_dollar,
                                diag::expected_tok_in_pil_instr, "$")
                || parseASTType(formalTy, formalTyLoc, patternEnv)
                || P.parseToken(tok::colon,
                                diag::expected_tok_in_pil_instr, ":")
                || parsePILType(loweredTy, loweredTyLoc,
                                ignoredParsedEnv, patternEnv))
               return true;

            if (patternEnv)
               loweredTy = PILType::getPrimitiveType(
                  loweredTy.getAstType()->mapTypeOutOfContext()
                     ->getCanonicalType(),
                  loweredTy.getCategory());

            // Formal type must be hashable.
            auto proto = P.Context.getInterface(KnownInterfaceKind::Hashable);
            Type contextFormalTy = formalTy;
            if (patternEnv)
               contextFormalTy = patternEnv->mapTypeIntoContext(formalTy);
            auto lookup = P.SF.getParentModule()->lookupConformance(
               contextFormalTy, proto);
            if (lookup.isInvalid()) {
               P.diagnose(formalTyLoc,
                          diag::pil_keypath_index_not_hashable,
                          formalTy);
               return true;
            }
            auto conformance = InterfaceConformanceRef(lookup);

            indexes.push_back({index, formalTy, loweredTy, conformance});

            if (operandTypes.size() <= index)
               operandTypes.resize(index+1);
            if (operandTypes[index] && operandTypes[index] != loweredTy) {
               P.diagnose(loweredTyLoc,
                          diag::pil_keypath_index_operand_type_conflict,
                          index,
                          operandTypes[index].getAstType(),
                          loweredTy.getAstType());
               return true;
            }
            operandTypes[index] = loweredTy;

            if (P.consumeIf(tok::comma))
               continue;
            if (P.consumeIf(tok::r_square))
               break;
            return true;
         }
         return false;
      };

   if (componentKind.str() == "stored_property") {
      ValueDecl *prop;
      CanType ty;
      if (parsePILDottedPath(prop)
          || P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":")
          || P.parseToken(tok::pil_dollar,
                          diag::expected_tok_in_pil_instr, "$")
          || parseASTType(ty, patternEnv))
         return true;
      component =
         KeyPathPatternComponent::forStoredProperty(cast<VarDecl>(prop), ty);
      return false;
   } else if (componentKind.str() == "gettable_property"
              || componentKind.str() == "settable_property") {
      bool isSettable = componentKind.str()[0] == 's';

      CanType componentTy;
      if (P.parseToken(tok::pil_dollar,diag::expected_tok_in_pil_instr,"$")
          || parseASTType(componentTy, patternEnv)
          || P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
         return true;

      PILFunction *idFn = nullptr;
      PILDeclRef idDecl;
      VarDecl *idProperty = nullptr;
      PILFunction *getter = nullptr;
      PILFunction *setter = nullptr;
      PILFunction *equals = nullptr;
      PILFunction *hash = nullptr;
      AbstractStorageDecl *externalDecl = nullptr;
      SubstitutionMap externalSubs;
      SmallVector<KeyPathPatternComponent::Index, 4> indexes;
      while (true) {
         Identifier subKind;
         SourceLoc subKindLoc;
         if (parsePILIdentifier(subKind, subKindLoc,
                                diag::pil_keypath_expected_component_kind))
            return true;

         if (subKind.str() == "id") {
            // The identifier can be either a function ref, a PILDeclRef
            // to a class or protocol method, or a decl ref to a property:
            // @static_fn_ref : $...
            // #Type.method!whatever : (T) -> ...
            // ##Type.property
            if (P.Tok.is(tok::at_sign)) {
               if (parsePILFunctionRef(InstLoc, idFn))
                  return true;
            } else if (P.Tok.is(tok::pound)) {
               if (P.peekToken().is(tok::pound)) {
                  ValueDecl *propertyValueDecl;
                  P.consumeToken(tok::pound);
                  if (parsePILDottedPath(propertyValueDecl))
                     return true;
                  idProperty = cast<VarDecl>(propertyValueDecl);
               } else if (parsePILDeclRef(idDecl, /*fnType*/ true))
                  return true;
            } else {
               P.diagnose(subKindLoc, diag::expected_tok_in_pil_instr, "# or @");
               return true;
            }
         } else if (subKind.str() == "getter" || subKind.str() == "setter") {
            bool isSetter = subKind.str()[0] == 's';
            if (parsePILFunctionRef(InstLoc, isSetter ? setter : getter))
               return true;
         } else if (subKind.str() == "indices") {
            if (P.parseToken(tok::l_square,
                             diag::expected_tok_in_pil_instr, "[")
                || parseComponentIndices(indexes))
               return true;
         } else if (subKind.str() == "indices_equals") {
            if (parsePILFunctionRef(InstLoc, equals))
               return true;
         } else if (subKind.str() == "indices_hash") {
            if (parsePILFunctionRef(InstLoc, hash))
               return true;
         } else if (subKind.str() == "external") {
            ValueDecl *parsedExternalDecl;
            SmallVector<ParsedSubstitution, 4> parsedSubs;

            if (parsePILDottedPath(parsedExternalDecl)
                || parseSubstitutions(parsedSubs, patternEnv))
               return true;

            externalDecl = cast<AbstractStorageDecl>(parsedExternalDecl);

            if (!parsedSubs.empty()) {
               auto genericEnv = externalDecl->getInnermostDeclContext()
                  ->getGenericEnvironmentOfContext();
               if (!genericEnv) {
                  P.diagnose(P.Tok,
                             diag::pil_substitutions_on_non_polymorphic_type);
                  return true;
               }
               externalSubs = getApplySubstitutionsFromParsed(*this, genericEnv,
                                                              parsedSubs);
               if (!externalSubs) return true;

               // Map the substitutions out of the pattern context so that they
               // use interface types.
               externalSubs =
                  externalSubs.mapReplacementTypesOutOfContext().getCanonical();
            }

         } else {
            P.diagnose(subKindLoc, diag::pil_keypath_unknown_component_kind,
                       subKind);
            return true;
         }

         if (!P.consumeIf(tok::comma))
            break;
      }

      if ((idFn == nullptr && idDecl.isNull() && idProperty == nullptr)
          || getter == nullptr
          || (isSettable && setter == nullptr)) {
         P.diagnose(componentLoc,
                    diag::pil_keypath_computed_property_missing_part,
                    isSettable);
         return true;
      }

      if ((idFn != nullptr) + (!idDecl.isNull()) + (idProperty != nullptr)
          != 1) {
         P.diagnose(componentLoc,
                    diag::pil_keypath_computed_property_missing_part,
                    isSettable);
         return true;
      }

      KeyPathPatternComponent::ComputedPropertyId id;
      if (idFn)
         id = idFn;
      else if (!idDecl.isNull())
         id = idDecl;
      else if (idProperty)
         id = idProperty;
      else
         llvm_unreachable("no id?!");

      auto indexesCopy = P.Context.AllocateCopy(indexes);

      if (!indexes.empty() && (!equals || !hash)) {
         P.diagnose(componentLoc,
                    diag::pil_keypath_computed_property_missing_part,
                    isSettable);
      }

      if (isSettable) {
         component = KeyPathPatternComponent::forComputedSettableProperty(
            id, getter, setter,
            indexesCopy, equals, hash,
            externalDecl, externalSubs, componentTy);
      } else {
         component = KeyPathPatternComponent::forComputedGettableProperty(
            id, getter,
            indexesCopy, equals, hash,
            externalDecl, externalSubs, componentTy);
      }
      return false;
   } else if (componentKind.str() == "optional_wrap"
              || componentKind.str() == "optional_chain"
              || componentKind.str() == "optional_force") {
      CanType ty;
      if (P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":")
          || P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$")
          || parseASTType(ty, patternEnv))
         return true;
      KeyPathPatternComponent::Kind kind;

      if (componentKind.str() == "optional_wrap") {
         kind = KeyPathPatternComponent::Kind::OptionalWrap;
      } else if (componentKind.str() == "optional_chain") {
         kind = KeyPathPatternComponent::Kind::OptionalChain;
      } else if (componentKind.str() == "optional_force") {
         kind = KeyPathPatternComponent::Kind::OptionalForce;
      } else {
         llvm_unreachable("unpossible");
      }

      component = KeyPathPatternComponent::forOptional(kind, ty);
      return false;
   } else if (componentKind.str() == "tuple_element") {
      unsigned tupleIndex;
      CanType ty;

      if (P.parseToken(tok::pound, diag::expected_pil_constant)
          || parseInteger(tupleIndex, diag::expected_pil_tuple_index)
          || P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":")
          || P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$")
          || parseASTType(ty, patternEnv))
         return true;

      component = KeyPathPatternComponent::forTupleElement(tupleIndex, ty);
      return false;
   } else {
      P.diagnose(componentLoc, diag::pil_keypath_unknown_component_kind,
                 componentKind);
      return true;
   }
}

/// pil-instruction-result ::= pil-value-name '='
/// pil-instruction-result ::= '(' pil-value-name? ')'
/// pil-instruction-result ::= '(' pil-value-name (',' pil-value-name)* ')'
/// pil-instruction-source-info ::= (',' pil-scope-ref)? (',' pil-loc)?
/// pil-instruction-def ::=
///   (pil-instruction-result '=')? pil-instruction pil-instruction-source-info
bool PILParser::parsePILInstruction(PILBuilder &B) {
   // We require PIL instructions to be at the start of a line to assist
   // recovery.
   if (!P.Tok.isAtStartOfLine()) {
      P.diagnose(P.Tok, diag::expected_pil_instr_start_of_line);
      return true;
   }

   SmallVector<std::pair<StringRef, SourceLoc>, 4> resultNames;
   SourceLoc resultClauseBegin;

   // If the instruction has a name '%foo =', parse it.
   if (P.Tok.is(tok::pil_local_name)) {
      resultClauseBegin = P.Tok.getLoc();
      resultNames.push_back(std::make_pair(P.Tok.getText(), P.Tok.getLoc()));
      P.consumeToken(tok::pil_local_name);

      // If the instruction has a '(%foo, %bar) = ', parse it.
   } else if (P.consumeIf(tok::l_paren)) {
      resultClauseBegin = P.PreviousLoc;

      if (!P.consumeIf(tok::r_paren)) {
         while (true) {
            if (!P.Tok.is(tok::pil_local_name)) {
               P.diagnose(P.Tok, diag::expected_pil_value_name);
               return true;
            }

            resultNames.push_back(std::make_pair(P.Tok.getText(), P.Tok.getLoc()));
            P.consumeToken(tok::pil_local_name);

            if (P.consumeIf(tok::comma))
               continue;
            if (P.consumeIf(tok::r_paren))
               break;

            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, ",");
            return true;
         }
      }
   }

   if (resultClauseBegin.isValid()) {
      if (P.parseToken(tok::equal, diag::expected_equal_in_pil_instr))
         return true;
   }

   PILInstructionKind Opcode;
   SourceLoc OpcodeLoc;
   StringRef OpcodeName;

   // Parse the opcode name.
   if (parsePILOpcode(Opcode, OpcodeLoc, OpcodeName))
      return true;

   SmallVector<PILValue, 4> OpList;
   PILValue Val;
   PILType Ty;
   PILLocation InstLoc = RegularLocation(OpcodeLoc);

   auto parseFormalTypeAndValue = [&](CanType &formalType,
                                      PILValue &value) -> bool {
      return (parseASTType(formalType) || parseVerbatim("in")
              || parseTypedValueRef(value, B));
   };

   OpenedExistentialAccess AccessKind;
   auto parseOpenExistAddrKind = [&]() -> bool {
      Identifier accessKindToken;
      SourceLoc accessKindLoc;
      if (parsePILIdentifier(accessKindToken, accessKindLoc,
                             diag::expected_tok_in_pil_instr,
                             "opened existential access kind")) {
         return true;
      }
      auto kind =
         llvm::StringSwitch<Optional<OpenedExistentialAccess>>(
            accessKindToken.str())
            .Case("mutable_access", OpenedExistentialAccess::Mutable)
            .Case("immutable_access", OpenedExistentialAccess::Immutable)
            .Default(None);

      if (kind) {
         AccessKind = kind.getValue();
         return false;
      }
      P.diagnose(accessKindLoc, diag::expected_tok_in_pil_instr,
                 "opened existential access kind");
      return true;
   };

   CanType SourceType, TargetType;
   PILValue SourceAddr, DestAddr;
   auto parseSourceAndDestAddress = [&] {
      return parseFormalTypeAndValue(SourceType, SourceAddr)
             || parseVerbatim("to")
             || parseFormalTypeAndValue(TargetType, DestAddr);
   };

   Identifier SuccessBBName, FailureBBName;
   SourceLoc SuccessBBLoc, FailureBBLoc;
   auto parseConditionalBranchDestinations = [&] {
      return P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",")
             || parsePILIdentifier(SuccessBBName, SuccessBBLoc,
                                   diag::expected_pil_block_name)
             || P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",")
             || parsePILIdentifier(FailureBBName, FailureBBLoc,
                                   diag::expected_pil_block_name)
             || parsePILDebugLocation(InstLoc, B);
   };

   // Validate the opcode name, and do opcode-specific parsing logic based on the
   // opcode we find.
   PILInstruction *ResultVal;
   switch (Opcode) {
      case PILInstructionKind::AllocBoxInst: {
         bool hasDynamicLifetime = false;
         if (parsePILOptional(hasDynamicLifetime, *this, "dynamic_lifetime"))
            return true;

         PILType Ty;
         if (parsePILType(Ty)) return true;
         PILDebugVariable VarInfo;
         if (parsePILDebugVar(VarInfo))
            return true;
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createAllocBox(InstLoc, Ty.castTo<PILBoxType>(), VarInfo,
                                      hasDynamicLifetime);
         break;
      }
      case PILInstructionKind::ApplyInst:
      case PILInstructionKind::BeginApplyInst:
      case PILInstructionKind::PartialApplyInst:
      case PILInstructionKind::TryApplyInst:
         if (parseCallInstruction(InstLoc, Opcode, B, ResultVal))
            return true;
         break;
      case PILInstructionKind::AbortApplyInst:
      case PILInstructionKind::EndApplyInst: {
         UnresolvedValueName argName;
         if (parseValueName(argName)) return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         PILType expectedTy = PILType::getPILTokenType(P.Context);
         PILValue op = getLocalValue(argName, expectedTy, InstLoc, B);

         if (Opcode == PILInstructionKind::AbortApplyInst) {
            ResultVal = B.createAbortApply(InstLoc, op);
         } else {
            ResultVal = B.createEndApply(InstLoc, op);
         }
         break;
      }
      case PILInstructionKind::IntegerLiteralInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         bool Negative = false;
         if (P.Tok.isAnyOperator() && P.Tok.getText() == "-") {
            Negative = true;
            P.consumeToken();
         }
         if (P.Tok.getKind() != tok::integer_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "integer");
            return true;
         }

         auto intTy = Ty.getAs<AnyBuiltinIntegerType>();
         if (!intTy) {
            P.diagnose(P.Tok, diag::pil_integer_literal_not_integer_type);
            return true;
         }

         StringRef text = prepareIntegerLiteralForParsing(P.Tok.getText());

         bool error;
         APInt value = intTy->getWidth().parse(text, 0, Negative, &error);
         if (error) {
            P.diagnose(P.Tok, diag::pil_integer_literal_not_well_formed, intTy);
            return true;
         }

         P.consumeToken(tok::integer_literal);
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createIntegerLiteral(InstLoc, Ty, value);
         break;
      }
      case PILInstructionKind::FloatLiteralInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         // The value is expressed as bits.
         if (P.Tok.getKind() != tok::integer_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "integer");
            return true;
         }

         auto floatTy = Ty.getAs<BuiltinFloatType>();
         if (!floatTy) {
            P.diagnose(P.Tok, diag::pil_float_literal_not_float_type);
            return true;
         }

         StringRef text = prepareIntegerLiteralForParsing(P.Tok.getText());

         APInt bits(floatTy->getBitWidth(), 0);
         bool error = text.getAsInteger(0, bits);
         assert(!error && "float_literal token did not parse as APInt?!");
         (void)error;

         if (bits.getBitWidth() != floatTy->getBitWidth())
            bits = bits.zextOrTrunc(floatTy->getBitWidth());

         APFloat value(floatTy->getAPFloatSemantics(), bits);
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createFloatLiteral(InstLoc, Ty, value);
         P.consumeToken(tok::integer_literal);
         break;
      }
      case PILInstructionKind::StringLiteralInst: {
         if (P.Tok.getKind() != tok::identifier) {
            P.diagnose(P.Tok, diag::pil_string_no_encoding);
            return true;
         }

         StringLiteralInst::Encoding encoding;
         if (P.Tok.getText() == "utf8") {
            encoding = StringLiteralInst::Encoding::UTF8;
         } else if (P.Tok.getText() == "utf16") {
            encoding = StringLiteralInst::Encoding::UTF16;
         } else if (P.Tok.getText() == "objc_selector") {
            encoding = StringLiteralInst::Encoding::ObjCSelector;
         } else if (P.Tok.getText() == "bytes") {
            encoding = StringLiteralInst::Encoding::Bytes;
         } else {
            P.diagnose(P.Tok, diag::pil_string_invalid_encoding, P.Tok.getText());
            return true;
         }
         P.consumeToken(tok::identifier);

         if (P.Tok.getKind() != tok::string_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "string");
            return true;
         }

         // Parse the string.
         SmallVector<Lexer::StringSegment, 1> segments;
         P.L->getStringLiteralSegments(P.Tok, segments);
         assert(segments.size() == 1);

         P.consumeToken(tok::string_literal);
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         SmallVector<char, 128> stringBuffer;

         if (encoding == StringLiteralInst::Encoding::Bytes) {
            // Decode hex bytes.
            CharSourceRange rawStringRange(segments.front().Loc,
                                           segments.front().Length);
            StringRef rawString = P.SourceMgr.extractText(rawStringRange);
            if (rawString.size() & 1) {
               P.diagnose(P.Tok, diag::expected_tok_in_pil_instr,
                          "even number of hex bytes");
               return true;
            }
            while (!rawString.empty()) {
               unsigned byte1 = llvm::hexDigitValue(rawString[0]);
               unsigned byte2 = llvm::hexDigitValue(rawString[1]);
               if (byte1 == -1U || byte2 == -1U) {
                  P.diagnose(P.Tok, diag::expected_tok_in_pil_instr,
                             "hex bytes should contain 0-9, a-f, A-F only");
                  return true;
               }
               stringBuffer.push_back((unsigned char)(byte1 << 4) | byte2);
               rawString = rawString.drop_front(2);
            }

            ResultVal = B.createStringLiteral(InstLoc, stringBuffer, encoding);
            break;
         }

         StringRef string = P.L->getEncodedStringSegment(segments.front(),
                                                         stringBuffer);
         ResultVal = B.createStringLiteral(InstLoc, string, encoding);
         break;
      }

      case PILInstructionKind::CondFailInst: {

         if (parseTypedValueRef(Val, B))
            return true;

         SmallVector<char, 128> stringBuffer;
         StringRef message;
         if (P.consumeIf(tok::comma)) {
            // Parse the string.
            if (P.Tok.getKind() != tok::string_literal) {
               P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "string");
               return true;
            }
            SmallVector<Lexer::StringSegment, 1> segments;
            P.L->getStringLiteralSegments(P.Tok, segments);
            assert(segments.size() == 1);

            P.consumeToken(tok::string_literal);
            message = P.L->getEncodedStringSegment(segments.front(), stringBuffer);
         }
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createCondFail(InstLoc, Val, message);
         break;
      }

      case PILInstructionKind::AllocValueBufferInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             parseVerbatim("in") ||
             parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createAllocValueBuffer(InstLoc, Ty, Val);
         break;
      }
      case PILInstructionKind::ProjectValueBufferInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             parseVerbatim("in") ||
             parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createProjectValueBuffer(InstLoc, Ty, Val);
         break;
      }
      case PILInstructionKind::DeallocValueBufferInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             parseVerbatim("in") ||
             parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createDeallocValueBuffer(InstLoc, Ty, Val);
         break;
      }

      case PILInstructionKind::ProjectBoxInst: {
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         if (!P.Tok.is(tok::integer_literal)) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "integer");
            return true;
         }

         unsigned Index;
         bool error = parseIntegerLiteral(P.Tok.getText(), 0, Index);
         assert(!error && "project_box index did not parse as integer?!");
         (void)error;

         P.consumeToken(tok::integer_literal);
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createProjectBox(InstLoc, Val, Index);
         break;
      }

      case PILInstructionKind::ProjectExistentialBoxInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             parseVerbatim("in") ||
             parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createProjectExistentialBox(InstLoc, Ty, Val);
         break;
      }

      case PILInstructionKind::FunctionRefInst: {
         PILFunction *Fn;
         if (parsePILFunctionRef(InstLoc, Fn) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createFunctionRef(InstLoc, Fn);
         break;
      }
      case PILInstructionKind::DynamicFunctionRefInst: {
         PILFunction *Fn;
         if (parsePILFunctionRef(InstLoc, Fn) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         // Set a forward reference's dynamic property for the first time.
         if (!Fn->isDynamicallyReplaceable()) {
            if (!Fn->empty()) {
               P.diagnose(P.Tok, diag::expected_dynamic_func_attr);
               return true;
            }
            Fn->setIsDynamic();
         }
         ResultVal = B.createDynamicFunctionRef(InstLoc, Fn);
         break;
      }
      case PILInstructionKind::PreviousDynamicFunctionRefInst: {
         PILFunction *Fn;
         if (parsePILFunctionRef(InstLoc, Fn) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createPreviousDynamicFunctionRef(InstLoc, Fn);
         break;
      }
      case PILInstructionKind::BuiltinInst: {
         if (P.Tok.getKind() != tok::string_literal) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr,"builtin name");
            return true;
         }
         StringRef Str = P.Tok.getText();
         Identifier Id = P.Context.getIdentifier(Str.substr(1, Str.size()-2));
         P.consumeToken(tok::string_literal);

         // Find the builtin in the Builtin module
         SmallVector<ValueDecl*, 2> foundBuiltins;
         P.Context.TheBuiltinModule->lookupMember(foundBuiltins,
                                                  P.Context.TheBuiltinModule, Id,
                                                  Identifier());
         if (foundBuiltins.empty()) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr,"builtin name");
            return true;
         }
         assert(foundBuiltins.size() == 1 && "ambiguous builtin name?!");

         auto *builtinFunc = cast<FuncDecl>(foundBuiltins[0]);
         GenericEnvironment *genericEnv = builtinFunc->getGenericEnvironment();

         SmallVector<ParsedSubstitution, 4> parsedSubs;
         SubstitutionMap subMap;
         if (parseSubstitutions(parsedSubs))
            return true;

         if (!parsedSubs.empty()) {
            if (!genericEnv) {
               P.diagnose(P.Tok, diag::pil_substitutions_on_non_polymorphic_type);
               return true;
            }
            subMap = getApplySubstitutionsFromParsed(*this, genericEnv, parsedSubs);
            if (!subMap)
               return true;
         }

         if (P.Tok.getKind() != tok::l_paren) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "(");
            return true;
         }
         P.consumeToken(tok::l_paren);

         SmallVector<PILValue, 4> Args;
         while (true) {
            if (P.consumeIf(tok::r_paren))
               break;

            PILValue Val;
            if (parseTypedValueRef(Val, B))
               return true;
            Args.push_back(Val);
            if (P.consumeIf(tok::comma))
               continue;
            if (P.consumeIf(tok::r_paren))
               break;
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, ")' or ',");
            return true;
         }

         if (P.Tok.getKind() != tok::colon) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, ":");
            return true;
         }
         P.consumeToken(tok::colon);

         PILType ResultTy;
         if (parsePILType(ResultTy))
            return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createBuiltin(InstLoc, Id, ResultTy, subMap, Args);
         break;
      }
      case PILInstructionKind::OpenExistentialAddrInst:
         if (parseOpenExistAddrKind() || parseTypedValueRef(Val, B)
             || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createOpenExistentialAddr(InstLoc, Val, Ty, AccessKind);
         break;

      case PILInstructionKind::OpenExistentialBoxInst:
         if (parseTypedValueRef(Val, B) || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createOpenExistentialBox(InstLoc, Val, Ty);
         break;

      case PILInstructionKind::OpenExistentialBoxValueInst:
         if (parseTypedValueRef(Val, B) || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createOpenExistentialBoxValue(InstLoc, Val, Ty);
         break;

      case PILInstructionKind::OpenExistentialMetatypeInst:
         if (parseTypedValueRef(Val, B) || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createOpenExistentialMetatype(InstLoc, Val, Ty);
         break;

      case PILInstructionKind::OpenExistentialRefInst:
         if (parseTypedValueRef(Val, B) || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createOpenExistentialRef(InstLoc, Val, Ty);
         break;

      case PILInstructionKind::OpenExistentialValueInst:
         if (parseTypedValueRef(Val, B) || parseVerbatim("to") || parsePILType(Ty)
             || parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createOpenExistentialValue(InstLoc, Val, Ty);
         break;

#define UNARY_INSTRUCTION(ID) \
  case PILInstructionKind::ID##Inst:                   \
    if (parseTypedValueRef(Val, B)) return true; \
    if (parsePILDebugLocation(InstLoc, B)) return true; \
    ResultVal = B.create##ID(InstLoc, Val);   \
    break;

#define REFCOUNTING_INSTRUCTION(ID)                                            \
  case PILInstructionKind::ID##Inst: {                                                  \
    Atomicity atomicity = Atomicity::Atomic;                                   \
    StringRef Optional;                                                        \
    if (parsePILOptional(Optional, *this)) {                                   \
      if (Optional == "nonatomic") {                                           \
        atomicity = Atomicity::NonAtomic;                                      \
      } else {                                                                 \
        return true;                                                           \
      }                                                                        \
    }                                                                          \
    if (parseTypedValueRef(Val, B))                                            \
      return true;                                                             \
    if (parsePILDebugLocation(InstLoc, B))                                     \
      return true;                                                             \
    ResultVal = B.create##ID(InstLoc, Val, atomicity);                         \
  } break;

      UNARY_INSTRUCTION(ClassifyBridgeObject)
      UNARY_INSTRUCTION(ValueToBridgeObject)
      UNARY_INSTRUCTION(FixLifetime)
      UNARY_INSTRUCTION(EndLifetime)
      UNARY_INSTRUCTION(CopyBlock)
      UNARY_INSTRUCTION(IsUnique)
      UNARY_INSTRUCTION(DestroyAddr)
      UNARY_INSTRUCTION(CopyValue)
      UNARY_INSTRUCTION(DestroyValue)
      UNARY_INSTRUCTION(EndBorrow)
      UNARY_INSTRUCTION(DestructureStruct)
      UNARY_INSTRUCTION(DestructureTuple)
      REFCOUNTING_INSTRUCTION(UnmanagedReleaseValue)
      REFCOUNTING_INSTRUCTION(UnmanagedRetainValue)
      REFCOUNTING_INSTRUCTION(UnmanagedAutoreleaseValue)
      REFCOUNTING_INSTRUCTION(StrongRetain)
      REFCOUNTING_INSTRUCTION(StrongRelease)
      REFCOUNTING_INSTRUCTION(AutoreleaseValue)
      REFCOUNTING_INSTRUCTION(SetDeallocating)
      REFCOUNTING_INSTRUCTION(ReleaseValue)
      REFCOUNTING_INSTRUCTION(RetainValue)
      REFCOUNTING_INSTRUCTION(ReleaseValueAddr)
      REFCOUNTING_INSTRUCTION(RetainValueAddr)
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  UNARY_INSTRUCTION(StrongCopy##Name##Value)
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  REFCOUNTING_INSTRUCTION(StrongRetain##Name)                                  \
  REFCOUNTING_INSTRUCTION(Name##Retain)                                        \
  REFCOUNTING_INSTRUCTION(Name##Release)                                       \
  UNARY_INSTRUCTION(StrongCopy##Name##Value)
#include "polarphp/ast/ReferenceStorageDef.h"
#undef UNARY_INSTRUCTION
#undef REFCOUNTING_INSTRUCTION

      case PILInstructionKind::IsEscapingClosureInst: {
         bool IsObjcVerifcationType = false;
         if (parsePILOptional(IsObjcVerifcationType, *this, "objc"))
            return true;
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createIsEscapingClosure(
            InstLoc, Val,
            IsObjcVerifcationType ? IsEscapingClosureInst::ObjCEscaping
                                  : IsEscapingClosureInst::WithoutActuallyEscaping);
         break;
      }

      case PILInstructionKind::DebugValueInst:
      case PILInstructionKind::DebugValueAddrInst: {
         PILDebugVariable VarInfo;
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugVar(VarInfo) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         if (Opcode == PILInstructionKind::DebugValueInst)
            ResultVal = B.createDebugValue(InstLoc, Val, VarInfo);
         else
            ResultVal = B.createDebugValueAddr(InstLoc, Val, VarInfo);
         break;
      }

         // unchecked_ownership_conversion <reg> : <type>, <ownership> to <ownership>
      case PILInstructionKind::UncheckedOwnershipConversionInst: {
         ValueOwnershipKind LHSKind = ValueOwnershipKind::None;
         ValueOwnershipKind RHSKind = ValueOwnershipKind::None;
         SourceLoc Loc;

         if (parseTypedValueRef(Val, Loc, B) ||
             P.parseToken(tok::comma, diag::expected_pil_colon,
                          "unchecked_ownership_conversion value ownership kind "
                          "conversion specification") ||
             parsePILOwnership(LHSKind) || parseVerbatim("to") ||
             parsePILOwnership(RHSKind) || parsePILDebugLocation(InstLoc, B)) {
            return true;
         }

         if (Val.getOwnershipKind() != LHSKind) {
            return true;
         }

         ResultVal = B.createUncheckedOwnershipConversion(InstLoc, Val, RHSKind);
         break;
      }

      case PILInstructionKind::LoadInst: {
         LoadOwnershipQualifier Qualifier;
         SourceLoc AddrLoc;

         if (parseLoadOwnershipQualifier(Qualifier, *this) ||
             parseTypedValueRef(Val, AddrLoc, B) || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createLoad(InstLoc, Val, Qualifier);
         break;
      }

      case PILInstructionKind::LoadBorrowInst: {
         SourceLoc AddrLoc;

         if (parseTypedValueRef(Val, AddrLoc, B) || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createLoadBorrow(InstLoc, Val);
         break;
      }

      case PILInstructionKind::BeginBorrowInst: {
         SourceLoc AddrLoc;

         if (parseTypedValueRef(Val, AddrLoc, B) || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createBeginBorrow(InstLoc, Val);
         break;
      }

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Load##Name##Inst: { \
    bool isTake = false; \
    SourceLoc addrLoc; \
    if (parsePILOptional(isTake, *this, "take") || \
        parseTypedValueRef(Val, addrLoc, B) || \
        parsePILDebugLocation(InstLoc, B)) \
      return true; \
    if (!Val->getType().is<Name##StorageType>()) { \
      P.diagnose(addrLoc, diag::pil_operand_not_ref_storage_address, \
                 "source", OpcodeName, ReferenceOwnership::Name); \
    } \
    ResultVal = B.createLoad##Name(InstLoc, Val, IsTake_t(isTake)); \
    break; \
  }
#include "polarphp/ast/ReferenceStorageDef.h"

      case PILInstructionKind::CopyBlockWithoutEscapingInst: {
         PILValue Closure;
         if (parseTypedValueRef(Val, B) ||
             parseVerbatim("withoutEscaping") ||
             parseTypedValueRef(Closure, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createCopyBlockWithoutEscaping(InstLoc, Val, Closure);
         break;
      }

      case PILInstructionKind::MarkDependenceInst: {
         PILValue Base;
         if (parseTypedValueRef(Val, B) ||
             parseVerbatim("on") ||
             parseTypedValueRef(Base, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createMarkDependence(InstLoc, Val, Base);
         break;
      }

      case PILInstructionKind::KeyPathInst: {
         SmallVector<KeyPathPatternComponent, 4> components;
         PILType Ty;
         if (parsePILType(Ty)
             || P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         GenericParamList *generics = nullptr;
         GenericEnvironment *patternEnv = nullptr;
         CanType rootType;
         StringRef objcString;
         SmallVector<PILType, 4> operandTypes;
         {
            Scope genericsScope(&P, ScopeKind::Generics);
            generics = P.maybeParseGenericParams().getPtrOrNull();
            patternEnv = handlePILGenericParams(generics, &P.SF);

            if (P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
               return true;

            while (true) {
               Identifier componentKind;
               SourceLoc componentLoc;
               if (parsePILIdentifier(componentKind, componentLoc,
                                      diag::pil_keypath_expected_component_kind))
                  return true;


               if (componentKind.str() == "root") {
                  if (P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$")
                      || parseASTType(rootType, patternEnv))
                     return true;
               } else if (componentKind.str() == "objc") {
                  auto tok = P.Tok;
                  if (P.parseToken(tok::string_literal, diag::expected_tok_in_pil_instr,
                                   "string literal"))
                     return true;

                  auto objcStringValue = tok.getText().drop_front().drop_back();
                  objcString = StringRef(
                     P.Context.AllocateCopy<char>(objcStringValue.begin(),
                                                  objcStringValue.end()),
                     objcStringValue.size());
               } else {
                  KeyPathPatternComponent component;
                  if (parseKeyPathPatternComponent(component, operandTypes,
                                                   componentLoc, componentKind,
                                                   InstLoc, patternEnv))
                     return true;
                  components.push_back(component);
               }

               if (!P.consumeIf(tok::semi))
                  break;
            }

            if (P.parseToken(tok::r_paren, diag::expected_tok_in_pil_instr, ")") ||
                parsePILDebugLocation(InstLoc, B))
               return true;
         }

         if (rootType.isNull())
            P.diagnose(InstLoc.getSourceLoc(), diag::pil_keypath_no_root);

         SmallVector<ParsedSubstitution, 4> parsedSubs;
         if (parseSubstitutions(parsedSubs, ContextGenericEnv))
            return true;

         SubstitutionMap subMap;
         if (!parsedSubs.empty()) {
            if (!patternEnv) {
               P.diagnose(InstLoc.getSourceLoc(),
                          diag::pil_substitutions_on_non_polymorphic_type);
               return true;
            }

            subMap = getApplySubstitutionsFromParsed(*this, patternEnv, parsedSubs);
            if (!subMap)
               return true;
         }

         SmallVector<PILValue, 4> operands;

         if (P.consumeIf(tok::l_paren)) {
            while (true) {
               PILValue v;

               if (operands.size() >= operandTypes.size()
                   || !operandTypes[operands.size()]) {
                  P.diagnose(P.Tok, diag::pil_keypath_no_use_of_operand_in_pattern,
                             operands.size());
                  return true;
               }

               auto ty = operandTypes[operands.size()].subst(PILMod, subMap);

               if (parseValueRef(v, ty, RegularLocation(P.Tok.getLoc()), B))
                  return true;
               operands.push_back(v);

               if (P.consumeIf(tok::comma))
                  continue;
               if (P.consumeIf(tok::r_paren))
                  break;
               return true;
            }
         }

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         CanGenericSignature canSig = CanGenericSignature();
         if (patternEnv && patternEnv->getGenericSignature()) {
            canSig = patternEnv->getGenericSignature()->getCanonicalSignature();
         }
         CanType leafType;
         if (!components.empty())
            leafType = components.back().getComponentType();
         else
            leafType = rootType;
         auto pattern = KeyPathPattern::get(B.getModule(), canSig,
                                            rootType, leafType,
                                            components, objcString);

         ResultVal = B.createKeyPath(InstLoc, pattern, subMap, operands, Ty);
         break;
      }

         // Conversion instructions.
      case PILInstructionKind::UncheckedRefCastInst:
      case PILInstructionKind::UncheckedAddrCastInst:
      case PILInstructionKind::UncheckedTrivialBitCastInst:
      case PILInstructionKind::UncheckedBitwiseCastInst:
      case PILInstructionKind::UpcastInst:
      case PILInstructionKind::AddressToPointerInst:
      case PILInstructionKind::BridgeObjectToRefInst:
      case PILInstructionKind::BridgeObjectToWordInst:
      case PILInstructionKind::RefToRawPointerInst:
      case PILInstructionKind::RawPointerToRefInst:
#define LOADABLE_REF_STORAGE(Name, ...) \
  case PILInstructionKind::RefTo##Name##Inst: \
  case PILInstructionKind::Name##ToRefInst:
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::ThinFunctionToPointerInst:
      case PILInstructionKind::PointerToThinFunctionInst:
      case PILInstructionKind::ThinToThickFunctionInst:
//      case PILInstructionKind::ThickToObjCMetatypeInst:
//      case PILInstructionKind::ObjCToThickMetatypeInst:
      case PILInstructionKind::ConvertFunctionInst:
      case PILInstructionKind::ConvertEscapeToNoEscapeInst:
//      case PILInstructionKind::ObjCExistentialMetatypeToObjectInst:
//      case PILInstructionKind::ObjCMetatypeToObjectInst:
      {
         PILType Ty;
         Identifier ToToken;
         SourceLoc ToLoc;
         bool not_guaranteed = false;
         bool without_actually_escaping = false;
         if (Opcode == PILInstructionKind::ConvertEscapeToNoEscapeInst) {
            StringRef attrName;
            if (parsePILOptional(attrName, *this)) {
               if (attrName.equals("not_guaranteed"))
                  not_guaranteed = true;
               else
                  return true;
            }
         }
         if (parseTypedValueRef(Val, B)
             || parsePILIdentifier(ToToken, ToLoc, diag::expected_tok_in_pil_instr,
                                   "to"))
            return true;

         if (ToToken.str() != "to") {
            P.diagnose(ToLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }
         if (Opcode == PILInstructionKind::ConvertFunctionInst) {
            StringRef attrName;
            if (parsePILOptional(attrName, *this)) {
               if (attrName.equals("without_actually_escaping"))
                  without_actually_escaping = true;
               else
                  return true;
            }
         }
         if (parsePILType(Ty) || parsePILDebugLocation(InstLoc, B))
            return true;

         switch (Opcode) {
            default: llvm_unreachable("Out of sync with parent switch");
            case PILInstructionKind::UncheckedRefCastInst:
               ResultVal = B.createUncheckedRefCast(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::UncheckedAddrCastInst:
               ResultVal = B.createUncheckedAddrCast(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::UncheckedTrivialBitCastInst:
               ResultVal = B.createUncheckedTrivialBitCast(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::UncheckedBitwiseCastInst:
               ResultVal = B.createUncheckedBitwiseCast(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::UpcastInst:
               ResultVal = B.createUpcast(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::ConvertFunctionInst:
               ResultVal =
                  B.createConvertFunction(InstLoc, Val, Ty, without_actually_escaping);
               break;
            case PILInstructionKind::ConvertEscapeToNoEscapeInst:
               ResultVal =
                  B.createConvertEscapeToNoEscape(InstLoc, Val, Ty, !not_guaranteed);
               break;
            case PILInstructionKind::AddressToPointerInst:
               ResultVal = B.createAddressToPointer(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::BridgeObjectToRefInst:
               ResultVal = B.createBridgeObjectToRef(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::BridgeObjectToWordInst:
               ResultVal = B.createBridgeObjectToWord(InstLoc, Val);
               break;
            case PILInstructionKind::RefToRawPointerInst:
               ResultVal = B.createRefToRawPointer(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::RawPointerToRefInst:
               ResultVal = B.createRawPointerToRef(InstLoc, Val, Ty);
               break;
#define LOADABLE_REF_STORAGE(Name, ...) \
    case PILInstructionKind::RefTo##Name##Inst: \
      ResultVal = B.createRefTo##Name(InstLoc, Val, Ty); \
      break; \
    case PILInstructionKind::Name##ToRefInst: \
      ResultVal = B.create##Name##ToRef(InstLoc, Val, Ty); \
      break;
#include "polarphp/ast/ReferenceStorageDef.h"
            case PILInstructionKind::ThinFunctionToPointerInst:
               ResultVal = B.createThinFunctionToPointer(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::PointerToThinFunctionInst:
               ResultVal = B.createPointerToThinFunction(InstLoc, Val, Ty);
               break;
            case PILInstructionKind::ThinToThickFunctionInst:
               ResultVal = B.createThinToThickFunction(InstLoc, Val, Ty);
               break;
            // @todo
//            case PILInstructionKind::ThickToObjCMetatypeInst:
//               ResultVal = B.createThickToObjCMetatype(InstLoc, Val, Ty);
//               break;
//            case PILInstructionKind::ObjCToThickMetatypeInst:
//               ResultVal = B.createObjCToThickMetatype(InstLoc, Val, Ty);
//               break;
//            case PILInstructionKind::ObjCMetatypeToObjectInst:
//               ResultVal = B.createObjCMetatypeToObject(InstLoc, Val, Ty);
//               break;
//            case PILInstructionKind::ObjCExistentialMetatypeToObjectInst:
//               ResultVal = B.createObjCExistentialMetatypeToObject(InstLoc, Val, Ty);
//               break;
         }
         break;
      }
      case PILInstructionKind::PointerToAddressInst: {
         PILType Ty;
         Identifier ToToken;
         SourceLoc ToLoc;
         StringRef attr;
         if (parseTypedValueRef(Val, B) ||
             parsePILIdentifier(ToToken, ToLoc,
                                diag::expected_tok_in_pil_instr, "to"))
            return true;
         if (parsePILOptional(attr, *this) && attr.empty())
            return true;
         if (parsePILType(Ty) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         bool isStrict = attr.equals("strict");
         bool isInvariant = attr.equals("invariant");

         if (ToToken.str() != "to") {
            P.diagnose(ToLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }

         ResultVal = B.createPointerToAddress(InstLoc, Val, Ty,
                                              isStrict, isInvariant);
         break;
      }
      case PILInstructionKind::RefToBridgeObjectInst: {
         PILValue BitsVal;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(BitsVal, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createRefToBridgeObject(InstLoc, Val, BitsVal);
         break;
      }

      case PILInstructionKind::CheckedCastAddrBranchInst: {
         Identifier consumptionKindToken;
         SourceLoc consumptionKindLoc;
         if (parsePILIdentifier(consumptionKindToken, consumptionKindLoc,
                                diag::expected_tok_in_pil_instr,
                                "cast consumption kind")) {
            return true;
         }
         // NOTE: BorrowAlways is not a supported cast kind for address types, so we
         // purposely do not parse it here.
         auto kind = llvm::StringSwitch<Optional<CastConsumptionKind>>(
            consumptionKindToken.str())
            .Case("take_always", CastConsumptionKind::TakeAlways)
            .Case("take_on_success", CastConsumptionKind::TakeOnSuccess)
            .Case("copy_on_success", CastConsumptionKind::CopyOnSuccess)
            .Default(None);

         if (!kind) {
            P.diagnose(consumptionKindLoc, diag::expected_tok_in_pil_instr,
                       "cast consumption kind");
            return true;
         }
         auto consumptionKind = kind.getValue();

         if (parseSourceAndDestAddress() || parseConditionalBranchDestinations()
             || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createCheckedCastAddrBranch(
            InstLoc, consumptionKind, SourceAddr, SourceType, DestAddr, TargetType,
            getBBForReference(SuccessBBName, SuccessBBLoc),
            getBBForReference(FailureBBName, FailureBBLoc));
         break;
      }
      case PILInstructionKind::UncheckedRefCastAddrInst:
         if (parseSourceAndDestAddress() || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createUncheckedRefCastAddr(InstLoc, SourceAddr, SourceType,
                                                  DestAddr, TargetType);
         break;

      case PILInstructionKind::UnconditionalCheckedCastAddrInst:
         if (parseSourceAndDestAddress() || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createUnconditionalCheckedCastAddr(
            InstLoc, SourceAddr, SourceType, DestAddr, TargetType);
         break;

      case PILInstructionKind::UnconditionalCheckedCastValueInst: {
         if (parseASTType(SourceType)
             || parseVerbatim("in")
             || parseTypedValueRef(Val, B)
             || parseVerbatim("to")
             || parseASTType(TargetType)
             || parsePILDebugLocation(InstLoc, B))
            return true;

         auto opaque = lowering::AbstractionPattern::getOpaque();
         ResultVal =
            B.createUnconditionalCheckedCastValue(InstLoc,
                                                  Val, SourceType,
                                                  F->getLoweredType(opaque, TargetType),
                                                  TargetType);
         break;
      }

      case PILInstructionKind::UnconditionalCheckedCastInst: {
         if (parseTypedValueRef(Val, B) || parseVerbatim("to")
             || parseASTType(TargetType))
            return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         auto opaque = lowering::AbstractionPattern::getOpaque();
         ResultVal =
            B.createUnconditionalCheckedCast(InstLoc, Val,
                                             F->getLoweredType(opaque, TargetType),
                                             TargetType);
         break;
      }

      case PILInstructionKind::CheckedCastBranchInst: {
         bool isExact = false;
         if (Opcode == PILInstructionKind::CheckedCastBranchInst &&
             parsePILOptional(isExact, *this, "exact"))
            return true;

         if (parseTypedValueRef(Val, B) || parseVerbatim("to")
             || parseASTType(TargetType)
             || parseConditionalBranchDestinations())
            return true;

         auto opaque = lowering::AbstractionPattern::getOpaque();
         ResultVal = B.createCheckedCastBranch(
            InstLoc, isExact, Val,
            F->getLoweredType(opaque, TargetType), TargetType,
            getBBForReference(SuccessBBName, SuccessBBLoc),
            getBBForReference(FailureBBName, FailureBBLoc));
         break;
      }
      case PILInstructionKind::CheckedCastValueBranchInst: {
         if (parseASTType(SourceType)
             || parseVerbatim("in")
             || parseTypedValueRef(Val, B)
             || parseVerbatim("to")
             || parseASTType(TargetType)
             || parseConditionalBranchDestinations())
            return true;

         auto opaque = lowering::AbstractionPattern::getOpaque();
         ResultVal = B.createCheckedCastValueBranch(
            InstLoc, Val, SourceType,
            F->getLoweredType(opaque, TargetType), TargetType,
            getBBForReference(SuccessBBName, SuccessBBLoc),
            getBBForReference(FailureBBName, FailureBBLoc));
         break;
      }

      case PILInstructionKind::MarkUninitializedInst: {
         if (P.parseToken(tok::l_square, diag::expected_tok_in_pil_instr, "["))
            return true;

         Identifier KindId;
         SourceLoc KindLoc = P.Tok.getLoc();
         if (P.consumeIf(tok::kw_var))
            KindId = P.Context.getIdentifier("var");
         else if (P.parseIdentifier(KindId, KindLoc,
                                    diag::expected_tok_in_pil_instr, "kind"))
            return true;

         if (P.parseToken(tok::r_square, diag::expected_tok_in_pil_instr, "]"))
            return true;

         MarkUninitializedInst::Kind Kind;
         if (KindId.str() == "var")
            Kind = MarkUninitializedInst::Var;
         else if (KindId.str() == "rootself")
            Kind = MarkUninitializedInst::RootSelf;
         else if (KindId.str() == "crossmodulerootself")
            Kind = MarkUninitializedInst::CrossModuleRootSelf;
         else if (KindId.str() == "derivedself")
            Kind = MarkUninitializedInst::DerivedSelf;
         else if (KindId.str() == "derivedselfonly")
            Kind = MarkUninitializedInst::DerivedSelfOnly;
         else if (KindId.str() == "delegatingself")
            Kind = MarkUninitializedInst::DelegatingSelf;
         else if (KindId.str() == "delegatingselfallocated")
            Kind = MarkUninitializedInst::DelegatingSelfAllocated;
         else {
            P.diagnose(KindLoc, diag::expected_tok_in_pil_instr,
                       "var, rootself, crossmodulerootself, derivedself, "
                       "derivedselfonly, delegatingself, or delegatingselfallocated");
            return true;
         }

         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createMarkUninitialized(InstLoc, Val, Kind);
         break;
      }

      case PILInstructionKind::MarkFunctionEscapeInst: {
         SmallVector<PILValue, 4> OpList;
         do {
            if (parseTypedValueRef(Val, B)) return true;
            OpList.push_back(Val);
         } while (!peekPILDebugLocation(P) && P.consumeIf(tok::comma));

         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createMarkFunctionEscape(InstLoc, OpList);
         break;
      }

      case PILInstructionKind::AssignInst:
      case PILInstructionKind::StoreInst: {
         UnresolvedValueName From;
         SourceLoc ToLoc, AddrLoc;
         Identifier ToToken;
         PILValue AddrVal;
         StoreOwnershipQualifier StoreQualifier;
         AssignOwnershipQualifier AssignQualifier;
         bool IsStore = Opcode == PILInstructionKind::StoreInst;
         bool IsAssign = Opcode == PILInstructionKind::AssignInst;
         if (parseValueName(From) ||
             parsePILIdentifier(ToToken, ToLoc, diag::expected_tok_in_pil_instr,
                                "to"))
            return true;

         if (IsStore && parseStoreOwnershipQualifier(StoreQualifier, *this))
            return true;

         if (IsAssign && parseAssignOwnershipQualifier(AssignQualifier, *this))
            return true;

         if (parseTypedValueRef(AddrVal, AddrLoc, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (ToToken.str() != "to") {
            P.diagnose(ToLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }

         if (!AddrVal->getType().isAddress()) {
            P.diagnose(AddrLoc, diag::pil_operand_not_address, "destination",
                       OpcodeName);
            return true;
         }

         PILType ValType = AddrVal->getType().getObjectType();

         if (IsStore) {
            ResultVal = B.createStore(InstLoc,
                                      getLocalValue(From, ValType, InstLoc, B),
                                      AddrVal, StoreQualifier);
         } else {
            assert(IsAssign);

            ResultVal = B.createAssign(InstLoc,
                                       getLocalValue(From, ValType, InstLoc, B),
                                       AddrVal, AssignQualifier);
         }

         break;
      }

      case PILInstructionKind::AssignByWrapperInst: {
         PILValue Src, DestAddr, InitFn, SetFn;
         SourceLoc DestLoc;
         AssignOwnershipQualifier AssignQualifier;
         if (parseTypedValueRef(Src,  B) ||
             parseVerbatim("to") ||
             parseAssignOwnershipQualifier(AssignQualifier, *this) ||
             parseTypedValueRef(DestAddr, DestLoc, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("init") ||
             parseTypedValueRef(InitFn, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("set") ||
             parseTypedValueRef(SetFn, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (!DestAddr->getType().isAddress()) {
            P.diagnose(DestLoc, diag::pil_operand_not_address, "destination",
                       OpcodeName);
            return true;
         }

         ResultVal = B.createAssignByWrapper(InstLoc, Src, DestAddr, InitFn, SetFn,
                                             AssignQualifier);
         break;
      }

      case PILInstructionKind::BeginAccessInst:
      case PILInstructionKind::BeginUnpairedAccessInst:
      case PILInstructionKind::EndAccessInst:
      case PILInstructionKind::EndUnpairedAccessInst: {
         ParsedEnum<PILAccessKind> kind;
         ParsedEnum<PILAccessEnforcement> enforcement;
         ParsedEnum<bool> aborting;
         ParsedEnum<bool> noNestedConflict;
         ParsedEnum<bool> fromBuiltin;

         bool isBeginAccess = (Opcode == PILInstructionKind::BeginAccessInst ||
                               Opcode == PILInstructionKind::BeginUnpairedAccessInst);
         bool wantsEnforcement = (isBeginAccess ||
                                  Opcode == PILInstructionKind::EndUnpairedAccessInst);

         while (P.consumeIf(tok::l_square)) {
            Identifier ident;
            SourceLoc identLoc;
            if (parsePILIdentifier(ident, identLoc,
                                   diag::expected_in_attribute_list)) {
               if (P.consumeIf(tok::r_square)) {
                  continue;
               } else {
                  return true;
               }
            }
            StringRef attr = ident.str();

            auto setEnforcement = [&](PILAccessEnforcement value) {
               maybeSetEnum(wantsEnforcement, enforcement, value, attr, identLoc);
            };
            auto setKind = [&](PILAccessKind value) {
               maybeSetEnum(isBeginAccess, kind, value, attr, identLoc);
            };
            auto setAborting = [&](bool value) {
               maybeSetEnum(!isBeginAccess, aborting, value, attr, identLoc);
            };
            auto setNoNestedConflict = [&](bool value) {
               maybeSetEnum(isBeginAccess, noNestedConflict, value, attr, identLoc);
            };
            auto setFromBuiltin = [&](bool value) {
               maybeSetEnum(Opcode != PILInstructionKind::EndAccessInst, fromBuiltin,
                            value, attr, identLoc);
            };

            if (attr == "unknown") {
               setEnforcement(PILAccessEnforcement::Unknown);
            } else if (attr == "static") {
               setEnforcement(PILAccessEnforcement::Static);
            } else if (attr == "dynamic") {
               setEnforcement(PILAccessEnforcement::Dynamic);
            } else if (attr == "unsafe") {
               setEnforcement(PILAccessEnforcement::Unsafe);
            } else if (attr == "init") {
               setKind(PILAccessKind::Init);
            } else if (attr == "read") {
               setKind(PILAccessKind::Read);
            } else if (attr == "modify") {
               setKind(PILAccessKind::Modify);
            } else if (attr == "deinit") {
               setKind(PILAccessKind::Deinit);
            } else if (attr == "abort") {
               setAborting(true);
            } else if (attr == "no_nested_conflict") {
               setNoNestedConflict(true);
            } else if (attr == "builtin") {
               setFromBuiltin(true);
            } else {
               P.diagnose(identLoc, diag::unknown_attribute, attr);
            }

            if (!P.consumeIf(tok::r_square))
               return true;
         }

         if (isBeginAccess && !kind.isSet()) {
            P.diagnose(OpcodeLoc, diag::pil_expected_access_kind, OpcodeName);
            kind.Value = PILAccessKind::Read;
         }

         if (wantsEnforcement && !enforcement.isSet()) {
            P.diagnose(OpcodeLoc, diag::pil_expected_access_enforcement, OpcodeName);
            enforcement.Value = PILAccessEnforcement::Unsafe;
         }

         if (!isBeginAccess && !aborting.isSet())
            aborting.Value = false;

         if (isBeginAccess && !noNestedConflict.isSet())
            noNestedConflict.Value = false;

         if (!fromBuiltin.isSet())
            fromBuiltin.Value = false;

         PILValue addrVal;
         SourceLoc addrLoc;
         if (parseTypedValueRef(addrVal, addrLoc, B))
            return true;

         PILValue bufferVal;
         SourceLoc bufferLoc;
         if (Opcode == PILInstructionKind::BeginUnpairedAccessInst &&
             (P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
              parseTypedValueRef(bufferVal, bufferLoc, B)))
            return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         if (!addrVal->getType().isAddress()) {
            P.diagnose(addrLoc, diag::pil_operand_not_address, "operand",
                       OpcodeName);
            return true;
         }

         if (Opcode == PILInstructionKind::BeginAccessInst) {
            ResultVal =
               B.createBeginAccess(InstLoc, addrVal, *kind, *enforcement,
                                   *noNestedConflict, *fromBuiltin);
         } else if (Opcode == PILInstructionKind::EndAccessInst) {
            ResultVal = B.createEndAccess(InstLoc, addrVal, *aborting);
         } else if (Opcode == PILInstructionKind::BeginUnpairedAccessInst) {
            ResultVal = B.createBeginUnpairedAccess(InstLoc, addrVal, bufferVal,
                                                    *kind, *enforcement,
                                                    *noNestedConflict, *fromBuiltin);
         } else {
            ResultVal = B.createEndUnpairedAccess(InstLoc, addrVal, *enforcement,
                                                  *aborting, *fromBuiltin);
         }
         break;
      }

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Store##Name##Inst:
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::StoreBorrowInst: {
         UnresolvedValueName from;
         bool isRefStorage = false;
#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
    isRefStorage |= Opcode == PILInstructionKind::Store##Name##Inst;
#include "polarphp/ast/ReferenceStorageDef.h"

         SourceLoc toLoc, addrLoc;
         Identifier toToken;
         PILValue addrVal;
         bool isInit = false;
         if (parseValueName(from) ||
             parsePILIdentifier(toToken, toLoc,
                                diag::expected_tok_in_pil_instr, "to") ||
             (isRefStorage && parsePILOptional(isInit, *this, "initialization")) ||
             parseTypedValueRef(addrVal, addrLoc, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (toToken.str() != "to") {
            P.diagnose(toLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }

         if (!addrVal->getType().isAddress()) {
            P.diagnose(addrLoc, diag::pil_operand_not_address,
                       "destination", OpcodeName);
            return true;
         }

         if (Opcode == PILInstructionKind::StoreBorrowInst) {
            PILType valueTy = addrVal->getType().getObjectType();
            ResultVal = B.createStoreBorrow(
               InstLoc, getLocalValue(from, valueTy, InstLoc, B), addrVal);
            break;
         }

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
    if (Opcode == PILInstructionKind::Store##Name##Inst) { \
      auto refType = addrVal->getType().getAs<Name##StorageType>(); \
      if (!refType) { \
        P.diagnose(addrLoc, diag::pil_operand_not_ref_storage_address, \
                   "destination", OpcodeName, ReferenceOwnership::Name); \
        return true; \
      } \
      auto valueTy =PILType::getPrimitiveObjectType(refType.getReferentType());\
      ResultVal = B.createStore##Name(InstLoc, \
                                      getLocalValue(from, valueTy, InstLoc, B),\
                                      addrVal, IsInitialization_t(isInit)); \
      break; \
    }
#include "polarphp/ast/ReferenceStorageDef.h"

         break;
      }
      case PILInstructionKind::AllocStackInst:
      case PILInstructionKind::MetatypeInst: {

         bool hasDynamicLifetime = false;
         if (Opcode == PILInstructionKind::AllocStackInst &&
             parsePILOptional(hasDynamicLifetime, *this, "dynamic_lifetime"))
            return true;

         PILType Ty;
         if (parsePILType(Ty))
            return true;

         if (Opcode == PILInstructionKind::AllocStackInst) {
            PILDebugVariable VarInfo;
            if (parsePILDebugVar(VarInfo) ||
                parsePILDebugLocation(InstLoc, B))
               return true;
            ResultVal = B.createAllocStack(InstLoc, Ty, VarInfo, hasDynamicLifetime);
         } else {
            assert(Opcode == PILInstructionKind::MetatypeInst);
            if (parsePILDebugLocation(InstLoc, B))
               return true;
            ResultVal = B.createMetatype(InstLoc, Ty);
         }
         break;
      }
      case PILInstructionKind::AllocRefInst:
      case PILInstructionKind::AllocRefDynamicInst: {
         bool IsObjC = false;
         bool OnStack = false;
         SmallVector<PILType, 2> ElementTypes;
         SmallVector<PILValue, 2> ElementCounts;
         while (P.consumeIf(tok::l_square)) {
            Identifier Id;
            parsePILIdentifier(Id, diag::expected_in_attribute_list);
            StringRef Optional = Id.str();
            if (Optional == "objc") {
               IsObjC = true;
            } else if (Optional == "stack") {
               OnStack = true;
            } else if (Optional == "tail_elems") {
               PILType ElemTy;
               if (parsePILType(ElemTy) ||
                   !P.Tok.isAnyOperator() ||
                   P.Tok.getText() != "*")
                  return true;
               P.consumeToken();

               PILValue ElemCount;
               if (parseTypedValueRef(ElemCount, B))
                  return true;

               ElementTypes.push_back(ElemTy);
               ElementCounts.push_back(ElemCount);
            } else {
               return true;
            }
            P.parseToken(tok::r_square, diag::expected_in_attribute_list);
         }
         PILValue Metadata;
         if (Opcode == PILInstructionKind::AllocRefDynamicInst) {
            if (parseTypedValueRef(Metadata, B) ||
                P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
               return true;
         }

         PILType ObjectType;
         if (parsePILType(ObjectType))
            return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         if (IsObjC && !ElementTypes.empty()) {
            P.diagnose(P.Tok, diag::pil_objc_with_tail_elements);
            return true;
         }
         if (Opcode == PILInstructionKind::AllocRefDynamicInst) {
            if (OnStack)
               return true;

            ResultVal = B.createAllocRefDynamic(InstLoc, Metadata, ObjectType,
                                                IsObjC, ElementTypes, ElementCounts);
         } else {
            ResultVal = B.createAllocRef(InstLoc, ObjectType, IsObjC, OnStack,
                                         ElementTypes, ElementCounts);
         }
         break;
      }

      case PILInstructionKind::DeallocStackInst:
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createDeallocStack(InstLoc, Val);
         break;
      case PILInstructionKind::DeallocRefInst: {
         bool OnStack = false;
         if (parsePILOptional(OnStack, *this, "stack"))
            return true;

         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createDeallocRef(InstLoc, Val, OnStack);
         break;
      }
      case PILInstructionKind::DeallocPartialRefInst: {
         PILValue Metatype, Instance;
         if (parseTypedValueRef(Instance, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(Metatype, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createDeallocPartialRef(InstLoc, Instance, Metatype);
         break;
      }
      case PILInstructionKind::DeallocBoxInst:
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createDeallocBox(InstLoc, Val);
         break;
      case PILInstructionKind::ValueMetatypeInst:
      case PILInstructionKind::ExistentialMetatypeInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         switch (Opcode) {
            default: llvm_unreachable("Out of sync with parent switch");
            case PILInstructionKind::ValueMetatypeInst:
               ResultVal = B.createValueMetatype(InstLoc, Ty, Val);
               break;
            case PILInstructionKind::ExistentialMetatypeInst:
               ResultVal = B.createExistentialMetatype(InstLoc, Ty, Val);
               break;
            case PILInstructionKind::DeallocBoxInst:
               ResultVal = B.createDeallocBox(InstLoc, Val);
               break;
         }
         break;
      }
      case PILInstructionKind::DeallocExistentialBoxInst: {
         CanType ConcreteTy;
         if (parseTypedValueRef(Val, B)
             || P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",")
             || P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$")
             || parseASTType(ConcreteTy)
             || parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createDeallocExistentialBox(InstLoc, ConcreteTy, Val);
         break;
      }
      case PILInstructionKind::TupleInst: {
         // Tuple instructions have two different syntaxes, one for simple tuple
         // types, one for complicated ones.
         if (P.Tok.isNot(tok::pil_dollar)) {
            // If there is no type, parse the simple form.
            if (P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
               return true;

            // TODO: Check for a type here.  This is how tuples with "interesting"
            // types are described.

            // This form is used with tuples that have elements with no names or
            // default values.
            SmallVector<TupleTypeElt, 4> TypeElts;
            if (P.Tok.isNot(tok::r_paren)) {
               do {
                  if (parseTypedValueRef(Val, B)) return true;
                  OpList.push_back(Val);
                  TypeElts.push_back(Val->getType().getAstType());
               } while (P.consumeIf(tok::comma));
            }
            HadError |= P.parseToken(tok::r_paren,
                                     diag::expected_tok_in_pil_instr,")");

            auto Ty = TupleType::get(TypeElts, P.Context);
            auto Ty2 = PILType::getPrimitiveObjectType(Ty->getCanonicalType());
            if (parsePILDebugLocation(InstLoc, B))
               return true;
            ResultVal = B.createTuple(InstLoc, Ty2, OpList);
            break;
         }

         // Otherwise, parse the fully general form.
         PILType Ty;
         if (parsePILType(Ty) ||
             P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
            return true;

         TupleType *TT = Ty.getAs<TupleType>();
         if (TT == nullptr) {
            P.diagnose(OpcodeLoc, diag::expected_tuple_type_in_tuple);
            return true;
         }

         SmallVector<TupleTypeElt, 4> TypeElts;
         if (P.Tok.isNot(tok::r_paren)) {
            do {
               if (TypeElts.size() > TT->getNumElements()) {
                  P.diagnose(P.Tok, diag::pil_tuple_inst_wrong_value_count,
                             TT->getNumElements());
                  return true;
               }
               Type EltTy = TT->getElement(TypeElts.size()).getType();
               if (parseValueRef(Val,
                                 PILType::getPrimitiveObjectType(EltTy->getCanonicalType()),
                                 RegularLocation(P.Tok.getLoc()), B))
                  return true;
               OpList.push_back(Val);
               TypeElts.push_back(Val->getType().getAstType());
            } while (P.consumeIf(tok::comma));
         }
         HadError |= P.parseToken(tok::r_paren,
                                  diag::expected_tok_in_pil_instr,")");

         if (TypeElts.size() != TT->getNumElements()) {
            P.diagnose(OpcodeLoc, diag::pil_tuple_inst_wrong_value_count,
                       TT->getNumElements());
            return true;
         }

         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createTuple(InstLoc, Ty, OpList);
         break;
      }
      case PILInstructionKind::EnumInst: {
         PILType Ty;
         PILDeclRef Elt;
         PILValue Operand;
         if (parsePILType(Ty) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDeclRef(Elt))
            return true;

         if (P.Tok.is(tok::comma) && !peekPILDebugLocation(P)) {
            P.consumeToken(tok::comma);
            if (parseTypedValueRef(Operand, B))
               return true;
         }

         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createEnum(InstLoc, Operand,
                                  cast<EnumElementDecl>(Elt.getDecl()), Ty);
         break;
      }
      case PILInstructionKind::InitEnumDataAddrInst:
      case PILInstructionKind::UncheckedEnumDataInst:
      case PILInstructionKind::UncheckedTakeEnumDataAddrInst: {
         PILValue Operand;
         PILDeclRef EltRef;
         if (parseTypedValueRef(Operand, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDeclRef(EltRef) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         EnumElementDecl *Elt = cast<EnumElementDecl>(EltRef.getDecl());
         auto ResultTy = Operand->getType().getEnumElementType(
            Elt, PILMod, B.getTypeExpansionContext());

         switch (Opcode) {
            case polar::PILInstructionKind::InitEnumDataAddrInst:
               ResultVal = B.createInitEnumDataAddr(InstLoc, Operand, Elt, ResultTy);
               break;
            case polar::PILInstructionKind::UncheckedTakeEnumDataAddrInst:
               ResultVal = B.createUncheckedTakeEnumDataAddr(InstLoc, Operand, Elt,
                                                             ResultTy);
               break;
            case polar::PILInstructionKind::UncheckedEnumDataInst:
               ResultVal = B.createUncheckedEnumData(InstLoc, Operand, Elt, ResultTy);
               break;
            default:
               llvm_unreachable("switch out of sync");
         }
         break;
      }
      case PILInstructionKind::InjectEnumAddrInst: {
         PILValue Operand;
         PILDeclRef EltRef;
         if (parseTypedValueRef(Operand, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDeclRef(EltRef) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         EnumElementDecl *Elt = cast<EnumElementDecl>(EltRef.getDecl());
         ResultVal = B.createInjectEnumAddr(InstLoc, Operand, Elt);
         break;
      }
      case PILInstructionKind::TupleElementAddrInst:
      case PILInstructionKind::TupleExtractInst: {
         SourceLoc NameLoc;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         unsigned Field = 0;
         TupleType *TT = Val->getType().getAs<TupleType>();
         if (P.Tok.isNot(tok::integer_literal) ||
             parseIntegerLiteral(P.Tok.getText(), 10, Field) ||
             Field >= TT->getNumElements()) {
            P.diagnose(P.Tok, diag::pil_tuple_inst_wrong_field);
            return true;
         }
         P.consumeToken(tok::integer_literal);
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         auto ResultTy = TT->getElement(Field).getType()->getCanonicalType();
         if (Opcode == PILInstructionKind::TupleElementAddrInst)
            ResultVal = B.createTupleElementAddr(InstLoc, Val, Field,
                                                 PILType::getPrimitiveAddressType(ResultTy));
         else
            ResultVal = B.createTupleExtract(InstLoc, Val, Field,
                                             PILType::getPrimitiveObjectType(ResultTy));
         break;
      }
      case PILInstructionKind::ReturnInst: {
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createReturn(InstLoc, Val);
         break;
      }
      case PILInstructionKind::ThrowInst: {
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createThrow(InstLoc, Val);
         break;
      }
      case PILInstructionKind::UnwindInst: {
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createUnwind(InstLoc);
         break;
      }
      case PILInstructionKind::YieldInst: {
         SmallVector<PILValue, 6> values;

         // Parse a parenthesized (unless length-1), comma-separated list
         // of yielded values.
         if (P.consumeIf(tok::l_paren)) {
            if (!P.Tok.is(tok::r_paren)) {
               do {
                  if (parseTypedValueRef(Val, B))
                     return true;
                  values.push_back(Val);
               } while (P.consumeIf(tok::comma));
            }

            if (P.parseToken(tok::r_paren, diag::expected_tok_in_pil_instr, ")"))
               return true;

         } else {
            if (parseTypedValueRef(Val, B))
               return true;
            values.push_back(Val);
         }

         Identifier resumeName, unwindName;
         SourceLoc resumeLoc, unwindLoc;
         if (P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("resume") ||
             parsePILIdentifier(resumeName, resumeLoc,
                                diag::expected_pil_block_name) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("unwind") ||
             parsePILIdentifier(unwindName, unwindLoc,
                                diag::expected_pil_block_name) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         auto resumeBB = getBBForReference(resumeName, resumeLoc);
         auto unwindBB = getBBForReference(unwindName, unwindLoc);
         ResultVal = B.createYield(InstLoc, values, resumeBB, unwindBB);
         break;
      }
      case PILInstructionKind::BranchInst: {
         Identifier BBName;
         SourceLoc NameLoc;
         if (parsePILIdentifier(BBName, NameLoc, diag::expected_pil_block_name))
            return true;

         SmallVector<PILValue, 6> Args;
         if (parsePILBBArgsAtBranch(Args, B))
            return true;

         if (parsePILDebugLocation(InstLoc, B))
            return true;

         // Note, the basic block here could be a reference to an undefined
         // basic block, which will be parsed later on.
         ResultVal = B.createBranch(InstLoc, getBBForReference(BBName, NameLoc),
                                    Args);
         break;
      }
      case PILInstructionKind::CondBranchInst: {
         UnresolvedValueName Cond;
         Identifier BBName, BBName2;
         SourceLoc NameLoc, NameLoc2;
         SmallVector<PILValue, 6> Args, Args2;
         if (parseValueName(Cond) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(BBName, NameLoc, diag::expected_pil_block_name) ||
             parsePILBBArgsAtBranch(Args, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(BBName2, NameLoc2,
                                diag::expected_pil_block_name) ||
             parsePILBBArgsAtBranch(Args2, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         auto I1Ty =
            PILType::getBuiltinIntegerType(1, PILMod.getAstContext());
         PILValue CondVal = getLocalValue(Cond, I1Ty, InstLoc, B);
         ResultVal = B.createCondBranch(InstLoc, CondVal,
                                        getBBForReference(BBName, NameLoc),
                                        Args,
                                        getBBForReference(BBName2, NameLoc2),
                                        Args2);
         break;
      }
      case PILInstructionKind::UnreachableInst:
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createUnreachable(InstLoc);
         break;

      case PILInstructionKind::ClassMethodInst:
      case PILInstructionKind::SuperMethodInst:
      case PILInstructionKind::ObjCMethodInst:
      case PILInstructionKind::ObjCSuperMethodInst: {
         PILDeclRef Member;
         PILType MethodTy;
         SourceLoc TyLoc;
         SmallVector<ValueDecl *, 4> values;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;

         if (parsePILDeclRef(Member, true))
            return true;

         if (P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(MethodTy, TyLoc) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         switch (Opcode) {
            default: llvm_unreachable("Out of sync with parent switch");
            case PILInstructionKind::ClassMethodInst:
               ResultVal = B.createClassMethod(InstLoc, Val, Member, MethodTy);
               break;
            case PILInstructionKind::SuperMethodInst:
               ResultVal = B.createSuperMethod(InstLoc, Val, Member, MethodTy);
               break;
            case PILInstructionKind::ObjCMethodInst:
               ResultVal = B.createObjCMethod(InstLoc, Val, Member, MethodTy);
               break;
            case PILInstructionKind::ObjCSuperMethodInst:
               ResultVal = B.createObjCSuperMethod(InstLoc, Val, Member, MethodTy);
               break;
         }
         break;
      }
      case PILInstructionKind::WitnessMethodInst: {
         CanType LookupTy;
         PILDeclRef Member;
         PILType MethodTy;
         SourceLoc TyLoc;
         if (P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$") ||
             parseASTType(LookupTy) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ","))
            return true;
         if (parsePILDeclRef(Member, true))
            return true;
         // Optional operand.
         PILValue Operand;
         if (P.Tok.is(tok::comma)) {
            P.consumeToken(tok::comma);
            if (parseTypedValueRef(Operand, B))
               return true;
         }
         if (P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
             parsePILType(MethodTy, TyLoc) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // If LookupTy is a non-archetype, look up its conformance.
         InterfaceDecl *proto
            = dyn_cast<InterfaceDecl>(Member.getDecl()->getDeclContext());
         if (!proto) {
            P.diagnose(TyLoc, diag::pil_witness_method_not_protocol);
            return true;
         }
         auto conformance = P.SF.getParentModule()->lookupConformance(LookupTy, proto);
         if (conformance.isInvalid()) {
            P.diagnose(TyLoc, diag::pil_witness_method_type_does_not_conform);
            return true;
         }

         ResultVal =
            B.createWitnessMethod(InstLoc, LookupTy, conformance, Member, MethodTy);
         break;
      }
      case PILInstructionKind::CopyAddrInst: {
         bool IsTake = false, IsInit = false;
         UnresolvedValueName SrcLName;
         PILValue DestLVal;
         SourceLoc ToLoc, DestLoc;
         Identifier ToToken;
         if (parsePILOptional(IsTake, *this, "take") || parseValueName(SrcLName) ||
             parsePILIdentifier(ToToken, ToLoc,
                                diag::expected_tok_in_pil_instr, "to") ||
             parsePILOptional(IsInit, *this, "initialization") ||
             parseTypedValueRef(DestLVal, DestLoc, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (ToToken.str() != "to") {
            P.diagnose(ToLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }

         if (!DestLVal->getType().isAddress()) {
            P.diagnose(DestLoc, diag::pil_invalid_instr_operands);
            return true;
         }

         PILValue SrcLVal = getLocalValue(SrcLName, DestLVal->getType(), InstLoc, B);
         ResultVal = B.createCopyAddr(InstLoc, SrcLVal, DestLVal,
                                      IsTake_t(IsTake),
                                      IsInitialization_t(IsInit));
         break;
      }
      case PILInstructionKind::BindMemoryInst: {
         PILValue IndexVal;
         Identifier ToToken;
         SourceLoc ToLoc;
         PILType EltTy;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(IndexVal, B) ||
             parsePILIdentifier(ToToken, ToLoc,
                                diag::expected_tok_in_pil_instr, "to") ||
             parsePILType(EltTy) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (ToToken.str() != "to") {
            P.diagnose(ToLoc, diag::expected_tok_in_pil_instr, "to");
            return true;
         }
         ResultVal = B.createBindMemory(InstLoc, Val, IndexVal, EltTy);
         break;
      }
      case PILInstructionKind::ObjectInst:
      case PILInstructionKind::StructInst: {
         PILType Ty;
         if (parsePILType(Ty) ||
             P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
            return true;

         // Parse a list of PILValue.
         bool OpsAreTailElems = false;
         unsigned NumBaseElems = 0;
         if (P.Tok.isNot(tok::r_paren)) {
            do {
               if (Opcode == PILInstructionKind::ObjectInst) {
                  if (parsePILOptional(OpsAreTailElems, *this, "tail_elems"))
                     return true;
               }
               if (parseTypedValueRef(Val, B)) return true;
               OpList.push_back(Val);
               if (!OpsAreTailElems)
                  NumBaseElems = OpList.size();
            } while (P.consumeIf(tok::comma));
         }
         if (P.parseToken(tok::r_paren,
                          diag::expected_tok_in_pil_instr,")") ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (Opcode == PILInstructionKind::StructInst) {
            ResultVal = B.createStruct(InstLoc, Ty, OpList);
         } else {
            ResultVal = B.createObject(InstLoc, Ty, OpList, NumBaseElems);
         }
         break;
      }
      case PILInstructionKind::StructElementAddrInst:
      case PILInstructionKind::StructExtractInst: {
         ValueDecl *FieldV;
         SourceLoc NameLoc = P.Tok.getLoc();
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDottedPath(FieldV) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         if (!FieldV || !isa<VarDecl>(FieldV)) {
            P.diagnose(NameLoc, diag::pil_struct_inst_wrong_field);
            return true;
         }
         VarDecl *Field = cast<VarDecl>(FieldV);

         // FIXME: substitution means this type should be explicit to improve
         // performance.
         auto ResultTy =
            Val->getType().getFieldType(Field, PILMod, B.getTypeExpansionContext());
         if (Opcode == PILInstructionKind::StructElementAddrInst)
            ResultVal = B.createStructElementAddr(InstLoc, Val, Field,
                                                  ResultTy.getAddressType());
         else
            ResultVal = B.createStructExtract(InstLoc, Val, Field,
                                              ResultTy.getObjectType());
         break;
      }
      case PILInstructionKind::RefElementAddrInst: {
         ValueDecl *FieldV;
         SourceLoc NameLoc;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDottedPath(FieldV) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         if (!FieldV || !isa<VarDecl>(FieldV)) {
            P.diagnose(NameLoc, diag::pil_ref_inst_wrong_field);
            return true;
         }
         VarDecl *Field = cast<VarDecl>(FieldV);
         auto ResultTy =
            Val->getType().getFieldType(Field, PILMod, B.getTypeExpansionContext());
         ResultVal = B.createRefElementAddr(InstLoc, Val, Field, ResultTy);
         break;
      }
      case PILInstructionKind::RefTailAddrInst: {
         SourceLoc NameLoc;
         PILType ResultObjTy;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(ResultObjTy) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         PILType ResultTy = ResultObjTy.getAddressType();
         ResultVal = B.createRefTailAddr(InstLoc, Val, ResultTy);
         break;
      }
      case PILInstructionKind::IndexAddrInst: {
         PILValue IndexVal;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(IndexVal, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createIndexAddr(InstLoc, Val, IndexVal);
         break;
      }
      case PILInstructionKind::TailAddrInst: {
         PILValue IndexVal;
         PILType ResultObjTy;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(IndexVal, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(ResultObjTy) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         PILType ResultTy = ResultObjTy.getAddressType();
         ResultVal = B.createTailAddr(InstLoc, Val, IndexVal, ResultTy);
         break;
      }
      case PILInstructionKind::IndexRawPointerInst: {
         PILValue IndexVal;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseTypedValueRef(IndexVal, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createIndexRawPointer(InstLoc, Val, IndexVal);
         break;
      }
      // @todo
//      case PILInstructionKind::ObjCInterfaceInst: {
//         Identifier InterfaceName;
//         PILType Ty;
//         if (P.parseToken(tok::pound, diag::expected_pil_constant) ||
//             parsePILIdentifier(InterfaceName, diag::expected_pil_constant) ||
//             P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
//             parsePILType(Ty) ||
//             parsePILDebugLocation(InstLoc, B))
//            return true;
//         // Find the decl for the protocol name.
//         ValueDecl *VD;
//         SmallVector<ValueDecl*, 4> CurModuleResults;
//         // Perform a module level lookup on the first component of the
//         // fully-qualified name.
//         P.SF.getParentModule()->lookupValue(InterfaceName,
//                                             NLKind::UnqualifiedLookup,
//                                             CurModuleResults);
//         assert(CurModuleResults.size() == 1);
//         VD = CurModuleResults[0];
//         ResultVal = B.createObjCInterface(InstLoc, cast<InterfaceDecl>(VD), Ty);
//         break;
//      }
      case PILInstructionKind::AllocGlobalInst: {
         Identifier GlobalName;
         SourceLoc IdLoc;
         if (P.parseToken(tok::at_sign, diag::expected_pil_value_name) ||
             parsePILIdentifier(GlobalName, IdLoc, diag::expected_pil_value_name) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Go through list of global variables in the PILModule.
         PILGlobalVariable *global = PILMod.lookUpGlobalVariable(GlobalName.str());
         if (!global) {
            P.diagnose(IdLoc, diag::pil_global_variable_not_found, GlobalName);
            return true;
         }

         ResultVal = B.createAllocGlobal(InstLoc, global);
         break;
      }
      case PILInstructionKind::GlobalAddrInst:
      case PILInstructionKind::GlobalValueInst: {
         Identifier GlobalName;
         SourceLoc IdLoc;
         PILType Ty;
         if (P.parseToken(tok::at_sign, diag::expected_pil_value_name) ||
             parsePILIdentifier(GlobalName, IdLoc, diag::expected_pil_value_name) ||
             P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
             parsePILType(Ty) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Go through list of global variables in the PILModule.
         PILGlobalVariable *global = PILMod.lookUpGlobalVariable(GlobalName.str());
         if (!global) {
            P.diagnose(IdLoc, diag::pil_global_variable_not_found, GlobalName);
            return true;
         }

         PILType expectedType = (Opcode == PILInstructionKind::GlobalAddrInst ?
                                 global->getLoweredType().getAddressType() :
                                 global->getLoweredType());
         if (expectedType != Ty) {
            P.diagnose(IdLoc, diag::pil_value_use_type_mismatch, GlobalName.str(),
                       global->getLoweredType().getAstType(),
                       Ty.getAstType());
            return true;
         }

         if (Opcode == PILInstructionKind::GlobalAddrInst) {
            ResultVal = B.createGlobalAddr(InstLoc, global);
         } else {
            ResultVal = B.createGlobalValue(InstLoc, global);
         }
         break;
      }
      case PILInstructionKind::SelectEnumInst:
      case PILInstructionKind::SelectEnumAddrInst: {
         if (parseTypedValueRef(Val, B))
            return true;

         SmallVector<std::pair<EnumElementDecl*, UnresolvedValueName>, 4>
            CaseValueNames;
         Optional<UnresolvedValueName> DefaultValueName;
         while (P.consumeIf(tok::comma)) {
            Identifier BBName;
            SourceLoc BBLoc;
            // Parse 'default' pil-value.
            UnresolvedValueName tmp;
            if (P.consumeIf(tok::kw_default)) {
               if (parseValueName(tmp))
                  return true;
               DefaultValueName = tmp;
               break;
            }

            // Parse 'case' pil-decl-ref ':' pil-value.
            if (P.consumeIf(tok::kw_case)) {
               PILDeclRef ElemRef;
               if (parsePILDeclRef(ElemRef))
                  return true;
               assert(ElemRef.hasDecl() && isa<EnumElementDecl>(ElemRef.getDecl()));
               P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":");
               parseValueName(tmp);
               CaseValueNames.push_back(std::make_pair(
                  cast<EnumElementDecl>(ElemRef.getDecl()),
                  tmp));
               continue;
            }

            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "case or default");
            return true;
         }

         // Parse the type of the result operands.
         PILType ResultType;
         if (P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":")
             || parsePILType(ResultType) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Resolve the results.
         SmallVector<std::pair<EnumElementDecl*, PILValue>, 4> CaseValues;
         PILValue DefaultValue;
         if (DefaultValueName)
            DefaultValue = getLocalValue(*DefaultValueName, ResultType, InstLoc, B);
         for (auto &caseName : CaseValueNames)
            CaseValues.push_back(std::make_pair(
               caseName.first,
               getLocalValue(caseName.second, ResultType, InstLoc, B)));

         if (Opcode == PILInstructionKind::SelectEnumInst)
            ResultVal = B.createSelectEnum(InstLoc, Val, ResultType,
                                           DefaultValue, CaseValues);
         else
            ResultVal = B.createSelectEnumAddr(InstLoc, Val, ResultType,
                                               DefaultValue, CaseValues);
         break;
      }

      case PILInstructionKind::SwitchEnumInst:
      case PILInstructionKind::SwitchEnumAddrInst: {
         if (parseTypedValueRef(Val, B))
            return true;

         SmallVector<std::pair<EnumElementDecl*, PILBasicBlock*>, 4> CaseBBs;
         PILBasicBlock *DefaultBB = nullptr;
         while (!peekPILDebugLocation(P) && P.consumeIf(tok::comma)) {
            Identifier BBName;
            SourceLoc BBLoc;
            // Parse 'default' pil-identifier.
            if (P.consumeIf(tok::kw_default)) {
               parsePILIdentifier(BBName, BBLoc, diag::expected_pil_block_name);
               DefaultBB = getBBForReference(BBName, BBLoc);
               break;
            }

            // Parse 'case' pil-decl-ref ':' pil-identifier.
            if (P.consumeIf(tok::kw_case)) {
               PILDeclRef ElemRef;
               if (parsePILDeclRef(ElemRef))
                  return true;
               assert(ElemRef.hasDecl() && isa<EnumElementDecl>(ElemRef.getDecl()));
               P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":");
               parsePILIdentifier(BBName, BBLoc, diag::expected_pil_block_name);
               CaseBBs.push_back( {cast<EnumElementDecl>(ElemRef.getDecl()),
                                   getBBForReference(BBName, BBLoc)} );
               continue;
            }

            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "case or default");
            return true;
         }
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         if (Opcode == PILInstructionKind::SwitchEnumInst)
            ResultVal = B.createSwitchEnum(InstLoc, Val, DefaultBB, CaseBBs);
         else
            ResultVal = B.createSwitchEnumAddr(InstLoc, Val, DefaultBB, CaseBBs);
         break;
      }
      case PILInstructionKind::SwitchValueInst: {
         if (parseTypedValueRef(Val, B))
            return true;

         SmallVector<std::pair<PILValue, PILBasicBlock *>, 4> CaseBBs;
         PILBasicBlock *DefaultBB = nullptr;
         while (!peekPILDebugLocation(P) && P.consumeIf(tok::comma)) {
            Identifier BBName;
            SourceLoc BBLoc;
            PILValue CaseVal;

            // Parse 'default' pil-identifier.
            if (P.consumeIf(tok::kw_default)) {
               parsePILIdentifier(BBName, BBLoc, diag::expected_pil_block_name);
               DefaultBB = getBBForReference(BBName, BBLoc);
               break;
            }

            // Parse 'case' value-ref ':' pil-identifier.
            if (P.consumeIf(tok::kw_case)) {
               if (parseValueRef(CaseVal, Val->getType(),
                                 RegularLocation(P.Tok.getLoc()), B)) {
                  // TODO: Issue a proper error message here
                  P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "reference to a value");
                  return true;
               }

               auto intTy = Val->getType().getAs<BuiltinIntegerType>();
               auto functionTy = Val->getType().getAs<PILFunctionType>();
               if (!intTy && !functionTy) {
                  P.diagnose(P.Tok, diag::pil_integer_literal_not_integer_type);
                  return true;
               }

               if (intTy) {
                  // If it is a switch on an integer type, check that all case values
                  // are integer literals or undef.
                  if (!isa<PILUndef>(CaseVal)) {
                     auto *IL = dyn_cast<IntegerLiteralInst>(CaseVal);
                     if (!IL) {
                        P.diagnose(P.Tok, diag::pil_integer_literal_not_integer_type);
                        return true;
                     }
                     APInt CaseValue = IL->getValue();

                     if (CaseValue.getBitWidth() != intTy->getGreatestWidth())
                        CaseVal = B.createIntegerLiteral(
                           IL->getLoc(), Val->getType(),
                           CaseValue.zextOrTrunc(intTy->getGreatestWidth()));
                  }
               }

               if (functionTy) {
                  // If it is a switch on a function type, check that all case values
                  // are function references or undef.
                  if (!isa<PILUndef>(CaseVal)) {
                     auto *FR = dyn_cast<FunctionRefInst>(CaseVal);
                     if (!FR) {
                        if (auto *CF = dyn_cast<ConvertFunctionInst>(CaseVal)) {
                           FR = dyn_cast<FunctionRefInst>(CF->getOperand());
                        }
                     }
                     if (!FR) {
                        P.diagnose(P.Tok, diag::pil_integer_literal_not_integer_type);
                        return true;
                     }
                  }
               }

               P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":");
               parsePILIdentifier(BBName, BBLoc, diag::expected_pil_block_name);
               CaseBBs.push_back({CaseVal, getBBForReference(BBName, BBLoc)});
               continue;
            }

            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "case or default");
            return true;
         }
         if (parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createSwitchValue(InstLoc, Val, DefaultBB, CaseBBs);
         break;
      }
      case PILInstructionKind::SelectValueInst: {
         if (parseTypedValueRef(Val, B))
            return true;

         SmallVector<std::pair<UnresolvedValueName, UnresolvedValueName>, 4>
            CaseValueAndResultNames;
         Optional<UnresolvedValueName> DefaultResultName;
         while (P.consumeIf(tok::comma)) {
            Identifier BBName;
            SourceLoc BBLoc;
            // Parse 'default' pil-value.
            UnresolvedValueName tmp;
            if (P.consumeIf(tok::kw_default)) {
               if (parseValueName(tmp))
                  return true;
               DefaultResultName = tmp;
               break;
            }

            // Parse 'case' pil-decl-ref ':' pil-value.
            if (P.consumeIf(tok::kw_case)) {
               UnresolvedValueName casevalue;
               parseValueName(casevalue);
               P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":");
               parseValueName(tmp);
               CaseValueAndResultNames.push_back(std::make_pair(
                  casevalue,
                  tmp));
               continue;
            }

            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "case or default");
            return true;
         }

         if (!DefaultResultName) {
            P.diagnose(P.Tok, diag::expected_tok_in_pil_instr, "default");
            return true;
         }

         // Parse the type of the result operands.
         PILType ResultType;
         if (P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
             parsePILType(ResultType) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Resolve the results.
         SmallVector<std::pair<PILValue, PILValue>, 4> CaseValues;
         PILValue DefaultValue;
         if (DefaultResultName)
            DefaultValue = getLocalValue(*DefaultResultName, ResultType, InstLoc, B);
         PILType ValType = Val->getType();
         for (auto &caseName : CaseValueAndResultNames)
            CaseValues.push_back(std::make_pair(
               getLocalValue(caseName.first, ValType, InstLoc, B),
               getLocalValue(caseName.second, ResultType, InstLoc, B)));

         ResultVal = B.createSelectValue(InstLoc, Val, ResultType,
                                         DefaultValue, CaseValues);
         break;
      }
      case PILInstructionKind::DeinitExistentialAddrInst: {
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createDeinitExistentialAddr(InstLoc, Val);
         break;
      }
      case PILInstructionKind::DeinitExistentialValueInst: {
         if (parseTypedValueRef(Val, B) || parsePILDebugLocation(InstLoc, B))
            return true;
         ResultVal = B.createDeinitExistentialValue(InstLoc, Val);
         break;
      }
      case PILInstructionKind::InitExistentialAddrInst: {
         CanType Ty;
         SourceLoc TyLoc;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$") ||
             parseASTType(Ty, TyLoc) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Lower the type at the abstraction level of the existential.
         auto archetype
            = OpenedArchetypeType::get(Val->getType().getAstType())
               ->getCanonicalType();

         auto &F = B.getFunction();
         PILType LoweredTy = F.getLoweredType(
               lowering::AbstractionPattern(archetype), Ty)
            .getAddressType();

         // Collect conformances for the type.
         ArrayRef<InterfaceConformanceRef> conformances
            = collectExistentialConformances(P, Ty, TyLoc,
                                             Val->getType().getAstType());

         ResultVal = B.createInitExistentialAddr(InstLoc, Val, Ty, LoweredTy,
                                                 conformances);
         break;
      }
      case PILInstructionKind::InitExistentialValueInst: {
         CanType FormalConcreteTy;
         PILType ExistentialTy;
         SourceLoc TyLoc;

         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$") ||
             parseASTType(FormalConcreteTy, TyLoc) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(ExistentialTy) || parsePILDebugLocation(InstLoc, B))
            return true;

         ArrayRef<InterfaceConformanceRef> conformances =
            collectExistentialConformances(P, FormalConcreteTy, TyLoc,
                                           ExistentialTy.getAstType());

         ResultVal = B.createInitExistentialValue(
            InstLoc, ExistentialTy, FormalConcreteTy, Val, conformances);
         break;
      }
      case PILInstructionKind::AllocExistentialBoxInst: {
         PILType ExistentialTy;
         CanType ConcreteFormalTy;
         SourceLoc TyLoc;

         if (parsePILType(ExistentialTy) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$") ||
             parseASTType(ConcreteFormalTy, TyLoc) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         // Collect conformances for the type.
         ArrayRef<InterfaceConformanceRef> conformances
            = collectExistentialConformances(P, ConcreteFormalTy, TyLoc,
                                             ExistentialTy.getAstType());

         ResultVal = B.createAllocExistentialBox(InstLoc, ExistentialTy,
                                                 ConcreteFormalTy, conformances);

         break;
      }
      case PILInstructionKind::InitExistentialRefInst: {
         CanType FormalConcreteTy;
         PILType ExistentialTy;
         SourceLoc TyLoc;

         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
             P.parseToken(tok::pil_dollar, diag::expected_tok_in_pil_instr, "$") ||
             parseASTType(FormalConcreteTy, TyLoc) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(ExistentialTy) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ArrayRef<InterfaceConformanceRef> conformances
            = collectExistentialConformances(P, FormalConcreteTy, TyLoc,
                                             ExistentialTy.getAstType());

         // FIXME: Conformances in InitExistentialRefInst is currently not included
         // in PIL.rst.
         ResultVal = B.createInitExistentialRef(InstLoc, ExistentialTy,
                                                FormalConcreteTy, Val,
                                                conformances);
         break;
      }
      case PILInstructionKind::InitExistentialMetatypeInst: {
         SourceLoc TyLoc;
         PILType ExistentialTy;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILType(ExistentialTy, TyLoc) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         auto baseExType = ExistentialTy.getAstType();
         auto formalConcreteType = Val->getType().getAstType();
         while (auto instExType = dyn_cast<ExistentialMetatypeType>(baseExType)) {
            baseExType = instExType.getInstanceType();
            formalConcreteType =
               cast<MetatypeType>(formalConcreteType).getInstanceType();
         }

         ArrayRef<InterfaceConformanceRef> conformances
            = collectExistentialConformances(P, formalConcreteType, TyLoc,
                                             baseExType);

         ResultVal = B.createInitExistentialMetatype(InstLoc, Val, ExistentialTy,
                                                     conformances);
         break;
      }
      case PILInstructionKind::DynamicMethodBranchInst: {
         PILDeclRef Member;
         Identifier BBName, BBName2;
         SourceLoc NameLoc, NameLoc2;
         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILDeclRef(Member) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(BBName, NameLoc, diag::expected_pil_block_name) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(BBName2, NameLoc2,
                                diag::expected_pil_block_name) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createDynamicMethodBranch(InstLoc, Val, Member,
                                                 getBBForReference(BBName, NameLoc),
                                                 getBBForReference(BBName2,
                                                                   NameLoc2));
         break;
      }
      case PILInstructionKind::ProjectBlockStorageInst: {
         if (parseTypedValueRef(Val, B) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         ResultVal = B.createProjectBlockStorage(InstLoc, Val);
         break;
      }
      case PILInstructionKind::InitBlockStorageHeaderInst: {
         Identifier invoke, type;
         SourceLoc invokeLoc, typeLoc;

         UnresolvedValueName invokeName;
         PILType invokeTy;
         GenericEnvironment *invokeGenericEnv;

         PILType blockType;
         SmallVector<ParsedSubstitution, 4> parsedSubs;


         if (parseTypedValueRef(Val, B) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(invoke, invokeLoc,
                                diag::expected_tok_in_pil_instr, "invoke") ||
             parseValueName(invokeName) ||
             parseSubstitutions(parsedSubs) ||
             P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
             parsePILType(invokeTy, invokeGenericEnv) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parsePILIdentifier(type, typeLoc,
                                diag::expected_tok_in_pil_instr, "type") ||
             parsePILType(blockType) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         if (invoke.str() != "invoke") {
            P.diagnose(invokeLoc, diag::expected_tok_in_pil_instr, "invoke");
            return true;
         }
         if (type.str() != "type") {
            P.diagnose(invokeLoc, diag::expected_tok_in_pil_instr, "type");
            return true;
         }

         auto invokeVal = getLocalValue(invokeName, invokeTy, InstLoc, B);

         SubstitutionMap subMap;
         if (!parsedSubs.empty()) {
            if (!invokeGenericEnv) {
               P.diagnose(typeLoc, diag::pil_substitutions_on_non_polymorphic_type);
               return true;
            }

            subMap = getApplySubstitutionsFromParsed(*this, invokeGenericEnv,
                                                     parsedSubs);
            if (!subMap)
               return true;
         }

         ResultVal = B.createInitBlockStorageHeader(InstLoc, Val, invokeVal,
                                                    blockType, subMap);
         break;
      }
   }

   // Match the results clause if we had one.
   if (resultClauseBegin.isValid()) {
      auto results = ResultVal->getResults();
      if (results.size() != resultNames.size()) {
         P.diagnose(resultClauseBegin, diag::wrong_result_count_in_pil_instr,
                    results.size());
      } else {
         for (size_t i : indices(results)) {
            setLocalValue(results[i], resultNames[i].first, resultNames[i].second);
         }
      }
   }

   return false;
}

bool PILParser::parseCallInstruction(PILLocation InstLoc,
                                     PILInstructionKind Opcode, PILBuilder &B,
                                     PILInstruction *&ResultVal) {
   UnresolvedValueName FnName;
   SmallVector<UnresolvedValueName, 4> ArgNames;

   auto PartialApplyConvention = ParameterConvention::Direct_Owned;
   bool IsNonThrowingApply = false;
   bool IsNoEscape = false;
   StringRef AttrName;

   while (parsePILOptional(AttrName, *this)) {
      if (AttrName.equals("nothrow"))
         IsNonThrowingApply = true;
      else if (AttrName.equals("callee_guaranteed"))
         PartialApplyConvention = ParameterConvention::Direct_Guaranteed;
      else if (AttrName.equals("on_stack"))
         IsNoEscape = true;
      else
         return true;
   }

   if (parseValueName(FnName))
      return true;
   SmallVector<ParsedSubstitution, 4> parsedSubs;
   if (parseSubstitutions(parsedSubs))
      return true;

   if (P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
      return true;

   if (P.Tok.isNot(tok::r_paren)) {
      do {
         UnresolvedValueName Arg;
         if (parseValueName(Arg)) return true;
         ArgNames.push_back(Arg);
      } while (P.consumeIf(tok::comma));
   }

   PILType Ty;
   SourceLoc TypeLoc;
   GenericEnvironment *GenericEnv = nullptr;
   if (P.parseToken(tok::r_paren, diag::expected_tok_in_pil_instr, ")") ||
       P.parseToken(tok::colon, diag::expected_tok_in_pil_instr, ":") ||
       parsePILType(Ty, TypeLoc, GenericEnv))
      return true;

   auto FTI = Ty.getAs<PILFunctionType>();
   if (!FTI) {
      P.diagnose(TypeLoc, diag::expected_pil_type_kind, "be a function");
      return true;
   }

   SubstitutionMap subs;
   if (!parsedSubs.empty()) {
      if (!GenericEnv) {
         P.diagnose(TypeLoc, diag::pil_substitutions_on_non_polymorphic_type);
         return true;
      }
      subs = getApplySubstitutionsFromParsed(*this, GenericEnv, parsedSubs);
      if (!subs)
         return true;
   }

   PILValue FnVal = getLocalValue(FnName, Ty, InstLoc, B);

   PILType FnTy = FnVal->getType();
   CanPILFunctionType substFTI = FTI;
   if (!subs.empty()) {
      auto silFnTy = FnTy.castTo<PILFunctionType>();
      substFTI =
         silFnTy->substGenericArgs(PILMod, subs, B.getTypeExpansionContext());
      FnTy = PILType::getPrimitiveObjectType(substFTI);
   }
   PILFunctionConventions substConv(substFTI, B.getModule());

   // Validate the operand count.
   if (substConv.getNumPILArguments() != ArgNames.size() &&
       Opcode != PILInstructionKind::PartialApplyInst) {
      P.diagnose(TypeLoc, diag::expected_pil_type_kind,
                 "to have the same number of arg names as arg types");
      return true;
   }

   // Validate the coroutine kind.
   if (Opcode == PILInstructionKind::ApplyInst ||
       Opcode == PILInstructionKind::TryApplyInst) {
      if (FTI->getCoroutineKind() != PILCoroutineKind::None) {
         P.diagnose(TypeLoc, diag::expected_pil_type_kind,
                    "to not be a coroutine");
         return true;
      }
   } else if (Opcode == PILInstructionKind::BeginApplyInst) {
      if (FTI->getCoroutineKind() != PILCoroutineKind::YieldOnce) {
         P.diagnose(TypeLoc, diag::expected_pil_type_kind,
                    "to be a yield_once coroutine");
         return true;
      }
   } else {
      assert(Opcode == PILInstructionKind::PartialApplyInst);
      // partial_apply accepts all kinds of function
   }

   switch (Opcode) {
      default: llvm_unreachable("Unexpected case");
      case PILInstructionKind::ApplyInst : {
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         unsigned ArgNo = 0;
         SmallVector<PILValue, 4> Args;
         for (auto &ArgName : ArgNames) {
            PILType expectedTy = substConv.getPILArgumentType(ArgNo++);
            Args.push_back(getLocalValue(ArgName, expectedTy, InstLoc, B));
         }

         ResultVal = B.createApply(InstLoc, FnVal, subs, Args, IsNonThrowingApply);
         break;
      }
      case PILInstructionKind::BeginApplyInst: {
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         unsigned ArgNo = 0;
         SmallVector<PILValue, 4> Args;
         for (auto &ArgName : ArgNames) {
            PILType expectedTy = substConv.getPILArgumentType(ArgNo++);
            Args.push_back(getLocalValue(ArgName, expectedTy, InstLoc, B));
         }

         ResultVal =
            B.createBeginApply(InstLoc, FnVal, subs, Args, IsNonThrowingApply);
         break;
      }
      case PILInstructionKind::PartialApplyInst: {
         if (parsePILDebugLocation(InstLoc, B))
            return true;

         // Compute the result type of the partial_apply, based on which arguments
         // are getting applied.
         SmallVector<PILValue, 4> Args;
         unsigned ArgNo = substConv.getNumPILArguments() - ArgNames.size();
         for (auto &ArgName : ArgNames) {
            PILType expectedTy = substConv.getPILArgumentType(ArgNo++);
            Args.push_back(getLocalValue(ArgName, expectedTy, InstLoc, B));
         }

         // FIXME: Why the arbitrary order difference in IRBuilder type argument?
         ResultVal = B.createPartialApply(
            InstLoc, FnVal, subs, Args, PartialApplyConvention,
            IsNoEscape ? PartialApplyInst::OnStackKind::OnStack
                       : PartialApplyInst::OnStackKind::NotOnStack);
         break;
      }
      case PILInstructionKind::TryApplyInst: {
         Identifier normalBBName, errorBBName;
         SourceLoc normalBBLoc, errorBBLoc;
         if (P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("normal") ||
             parsePILIdentifier(normalBBName, normalBBLoc,
                                diag::expected_pil_block_name) ||
             P.parseToken(tok::comma, diag::expected_tok_in_pil_instr, ",") ||
             parseVerbatim("error") ||
             parsePILIdentifier(errorBBName, errorBBLoc,
                                diag::expected_pil_block_name) ||
             parsePILDebugLocation(InstLoc, B))
            return true;

         unsigned argNo = 0;
         SmallVector<PILValue, 4> args;
         for (auto &argName : ArgNames) {
            PILType expectedTy = substConv.getPILArgumentType(argNo++);
            args.push_back(getLocalValue(argName, expectedTy, InstLoc, B));
         }

         PILBasicBlock *normalBB = getBBForReference(normalBBName, normalBBLoc);
         PILBasicBlock *errorBB = getBBForReference(errorBBName, errorBBLoc);
         ResultVal = B.createTryApply(InstLoc, FnVal, subs, args, normalBB, errorBB);
         break;
      }
   }
   return false;
}

bool PILParser::parsePILFunctionRef(PILLocation InstLoc,
                                    PILFunction *&ResultFn) {
   Identifier Name;
   PILType Ty;
   SourceLoc Loc = P.Tok.getLoc();
   if (parseGlobalName(Name) ||
       P.parseToken(tok::colon, diag::expected_pil_colon_value_ref) ||
       parsePILType(Ty))
      return true;

   auto FnTy = Ty.getAs<PILFunctionType>();
   if (!FnTy || !Ty.isObject()) {
      P.diagnose(Loc, diag::expected_pil_function_type);
      return true;
   }

   ResultFn = getGlobalNameForReference(Name, FnTy, Loc);
   return false;
}

/// True if the current token sequence looks like the start of a PIL
/// instruction. This can be one of:
///
/// 1. %name
/// 2. ()
/// 3. (%name1
/// 4. identifier | keyword
///   where the identifier is not followed by a ':' or '(', or it is
///   followed by '(' and is an instruction name.  The exceptions here
///   are for recognizing block names.
bool PILParser::isStartOfPILInstruction() {
   if (P.Tok.is(tok::pil_local_name))
      return true;
   if (P.Tok.is(tok::l_paren) &&
       (P.peekToken().is(tok::pil_local_name) || P.peekToken().is(tok::r_paren)))
      return true;
   if (P.Tok.is(tok::identifier) || P.Tok.isKeyword()) {
      auto &peek = P.peekToken();
      if (peek.is(tok::l_paren))
         return getOpcodeByName(P.Tok.getText()).hasValue();
      return !peek.is(tok::colon);
   }
   return false;
}

///   pil-basic-block:
///     pil-instruction+
///     identifier pil-bb-argument-list? ':' pil-instruction+
///   pil-bb-argument-list:
///     '(' pil-typed-valueref (',' pil-typed-valueref)+ ')'
bool PILParser::parsePILBasicBlock(PILBuilder &B) {
   PILBasicBlock *BB;

   // The basic block name is optional.
   if (P.Tok.is(tok::pil_local_name)) {
      BB = getBBForDefinition(Identifier(), SourceLoc());
   } else {
      Identifier BBName;
      SourceLoc NameLoc;
      if (parsePILIdentifier(BBName, NameLoc, diag::expected_pil_block_name))
         return true;

      BB = getBBForDefinition(BBName, NameLoc);
      // For now, since we always assume that PhiArguments have
      // ValueOwnershipKind::None, do not parse or do anything special. Eventually
      // we will parse the convention.
      bool IsEntry = BB->isEntry();

      // If there is a basic block argument list, process it.
      if (P.consumeIf(tok::l_paren)) {
         do {
            PILType Ty;
            ValueOwnershipKind OwnershipKind = ValueOwnershipKind::None;
            SourceLoc NameLoc;
            StringRef Name = P.Tok.getText();
            if (P.parseToken(tok::pil_local_name, NameLoc,
                             diag::expected_pil_value_name) ||
                P.parseToken(tok::colon, diag::expected_pil_colon_value_ref))
               return true;

            // If PILOwnership is enabled and we are not assuming that we are
            // parsing unqualified PIL, look for printed value ownership kinds.
            if (F->hasOwnership() &&
                parsePILOwnership(OwnershipKind))
               return true;

            if (parsePILType(Ty))
               return true;

            PILArgument *Arg;
            if (IsEntry) {
               Arg = BB->createFunctionArgument(Ty);
               // Today, we construct the ownership kind straight from the function
               // type. Make sure they are in sync, otherwise bail. We want this to
               // be an exact check rather than a compatibility check since we do not
               // want incompatibilities in between @any and other types of ownership
               // to be ignored.
               if (F->hasOwnership() && Arg->getOwnershipKind() != OwnershipKind) {
                  auto diagID =
                     diag::silfunc_and_pilarg_have_incompatible_pil_value_ownership;
                  P.diagnose(NameLoc, diagID, Arg->getOwnershipKind().asString(),
                             OwnershipKind.asString());
                  return true;
               }
            } else {
               Arg = BB->createPhiArgument(Ty, OwnershipKind);
            }
            setLocalValue(Arg, Name, NameLoc);
         } while (P.consumeIf(tok::comma));

         if (P.parseToken(tok::r_paren, diag::pil_basicblock_arg_rparen))
            return true;
      }

      if (P.parseToken(tok::colon, diag::expected_pil_block_colon))
         return true;
   }

   // Make sure the block is at the end of the function so that forward
   // references don't affect block layout.
   F->getBlocks().remove(BB);
   F->getBlocks().push_back(BB);

   B.setInsertionPoint(BB);
   do {
      if (parsePILInstruction(B))
         return true;
   } while (isStartOfPILInstruction());

   return false;
}

///   decl-sil:   [[only in PIL mode]]
///     'sil' pil-linkage '@' identifier ':' pil-type decl-pil-body?
///   decl-pil-body:
///     '{' pil-basic-block+ '}'
bool PILParserTUState::parseDeclPIL(Parser &P) {
   // Inform the lexer that we're lexing the body of the PIL declaration.  Do
   // this before we consume the 'sil' token so that all later tokens are
   // properly handled.
   Lexer::PILBodyRAII Tmp(*P.L);

   P.consumeToken(tok::kw_pil);

   PILParser FunctionState(P);

   Optional<PILLinkage> FnLinkage;
   Identifier FnName;
   PILType FnType;
   SourceLoc FnNameLoc;

   Scope S(&P, ScopeKind::TopLevel);
   bool isTransparent = false;
   IsSerialized_t isSerialized = IsNotSerialized;
   bool isCanonical = false;
   IsDynamicallyReplaceable_t isDynamic = IsNotDynamic;
   IsExactSelfClass_t isExactSelfClass = IsNotExactSelfClass;
   bool hasOwnershipSSA = false;
   IsThunk_t isThunk = IsNotThunk;
   bool isGlobalInit = false, isWeakImported = false;
   AvailabilityContext availability = AvailabilityContext::alwaysAvailable();
   bool isWithoutActuallyEscapingThunk = false;
   Inline_t inlineStrategy = InlineDefault;
   OptimizationMode optimizationMode = OptimizationMode::NotSet;
   SmallVector<std::string, 1> Semantics;
   SmallVector<ParsedSpecAttr, 4> SpecAttrs;
   ValueDecl *ClangDecl = nullptr;
   EffectsKind MRK = EffectsKind::Unspecified;
   PILFunction *DynamicallyReplacedFunction = nullptr;
   Identifier objCReplacementFor;
   if (parsePILLinkage(FnLinkage, P) ||
       parseDeclPILOptional(
          &isTransparent, &isSerialized, &isCanonical, &hasOwnershipSSA,
          &isThunk, &isDynamic, &isExactSelfClass, &DynamicallyReplacedFunction,
          &objCReplacementFor, &isGlobalInit, &inlineStrategy, &optimizationMode, nullptr,
          &isWeakImported, &availability, &isWithoutActuallyEscapingThunk, &Semantics,
          &SpecAttrs, &ClangDecl, &MRK, FunctionState, M) ||
       P.parseToken(tok::at_sign, diag::expected_pil_function_name) ||
       P.parseIdentifier(FnName, FnNameLoc, diag::expected_pil_function_name) ||
       P.parseToken(tok::colon, diag::expected_pil_type))
      return true;
   {
      // Construct a Scope for the function body so TypeAliasDecl can be added to
      // the scope.
      Scope Body(&P, ScopeKind::FunctionBody);
      GenericEnvironment *GenericEnv;
      if (FunctionState.parsePILType(FnType, GenericEnv, true /*IsFuncDecl*/))
         return true;
      auto PILFnType = FnType.getAs<PILFunctionType>();
      if (!PILFnType || !FnType.isObject()) {
         P.diagnose(FnNameLoc, diag::expected_pil_function_type);
         return true;
      }

      FunctionState.F =
         FunctionState.getGlobalNameForDefinition(FnName, PILFnType, FnNameLoc);
      FunctionState.F->setBare(IsBare);
      FunctionState.F->setTransparent(IsTransparent_t(isTransparent));
      FunctionState.F->setSerialized(IsSerialized_t(isSerialized));
      FunctionState.F->setWasDeserializedCanonical(isCanonical);
      if (!hasOwnershipSSA)
         FunctionState.F->setOwnershipEliminated();
      FunctionState.F->setThunk(IsThunk_t(isThunk));
      FunctionState.F->setIsDynamic(isDynamic);
      FunctionState.F->setIsExactSelfClass(isExactSelfClass);
      FunctionState.F->setDynamicallyReplacedFunction(
         DynamicallyReplacedFunction);
      if (!objCReplacementFor.empty())
         FunctionState.F->setObjCReplacement(objCReplacementFor);
      FunctionState.F->setGlobalInit(isGlobalInit);
      FunctionState.F->setAlwaysWeakImported(isWeakImported);
      FunctionState.F->setAvailabilityForLinkage(availability);
      FunctionState.F->setWithoutActuallyEscapingThunk(
         isWithoutActuallyEscapingThunk);
      FunctionState.F->setInlineStrategy(inlineStrategy);
      FunctionState.F->setOptimizationMode(optimizationMode);
      FunctionState.F->setEffectsKind(MRK);
      if (ClangDecl)
         FunctionState.F->setClangNodeOwner(ClangDecl);
      for (auto &Attr : Semantics) {
         FunctionState.F->addSemanticsAttr(Attr);
      }
      // Now that we have a PILFunction parse the body, if present.

      bool isDefinition = false;
      SourceLoc LBraceLoc = P.Tok.getLoc();

      if (P.consumeIf(tok::l_brace)) {
         isDefinition = true;

         FunctionState.ContextGenericEnv = GenericEnv;
         FunctionState.F->setGenericEnvironment(GenericEnv);

         if (GenericEnv && !SpecAttrs.empty()) {
            for (auto &Attr : SpecAttrs) {
               SmallVector<Requirement, 2> requirements;
               // Resolve types and convert requirements.
               FunctionState.convertRequirements(FunctionState.F,
                                                 Attr.requirements, requirements);
               auto *fenv = FunctionState.F->getGenericEnvironment();
               auto genericSig = evaluateOrDefault(
                  P.Context.evaluator,
                  AbstractGenericSignatureRequest{
                     fenv->getGenericSignature().getPointer(),
                     /*addedGenericParams=*/{ },
                     std::move(requirements)},
                  GenericSignature());
               FunctionState.F->addSpecializeAttr(PILSpecializeAttr::create(
                  FunctionState.F->getModule(), genericSig, Attr.exported,
                  Attr.kind));
            }
         }

         // Parse the basic block list.
         PILOpenedArchetypesTracker OpenedArchetypesTracker(FunctionState.F);
         PILBuilder B(*FunctionState.F);
         // Track the archetypes just like PILGen. This
         // is required for adding typedef operands to instructions.
         B.setOpenedArchetypesTracker(&OpenedArchetypesTracker);

         // Define a callback to be invoked on the deserialized types.
         auto OldParsedTypeCallback = FunctionState.ParsedTypeCallback;
         POLAR_DEFER {
            FunctionState.ParsedTypeCallback = OldParsedTypeCallback;
         };

         FunctionState.ParsedTypeCallback = [&OpenedArchetypesTracker](Type ty) {
            OpenedArchetypesTracker.registerUsedOpenedArchetypes(
               ty->getCanonicalType());
         };

         do {
            if (FunctionState.parsePILBasicBlock(B))
               return true;
         } while (P.Tok.isNot(tok::r_brace) && P.Tok.isNot(tok::eof));

         SourceLoc RBraceLoc;
         P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                              LBraceLoc);

         // Check that there are no unresolved forward definitions of opened
         // archetypes.
         if (OpenedArchetypesTracker.hasUnresolvedOpenedArchetypeDefinitions())
            llvm_unreachable(
               "All forward definitions of opened archetypes should be resolved");
      }

      FunctionState.F->setLinkage(resolvePILLinkage(FnLinkage, isDefinition));
   }

   if (FunctionState.diagnoseProblems())
      return true;

   // If PIL parsing succeeded, verify the generated PIL.
   if (!P.Diags.hadAnyError())
      FunctionState.F->verify();

   return false;
}

///   decl-pil-stage:   [[only in PIL mode]]
///     'pil_stage' ('raw' | 'canonical')
bool PILParserTUState::parseDeclPILStage(Parser &P) {
   SourceLoc stageLoc = P.consumeToken(tok::kw_pil_stage);
   if (!P.Tok.is(tok::identifier)) {
      P.diagnose(P.Tok, diag::expected_pil_stage_name);
      return true;
   }
   PILStage stage;
   if (P.Tok.isContextualKeyword("raw")) {
      stage = PILStage::Raw;
      P.consumeToken();
   } else if (P.Tok.isContextualKeyword("canonical")) {
      stage = PILStage::Canonical;
      P.consumeToken();
   } else if (P.Tok.isContextualKeyword("lowered")) {
      stage = PILStage::Lowered;
      P.consumeToken();
   } else {
      P.diagnose(P.Tok, diag::expected_pil_stage_name);
      P.consumeToken();
      return true;
   }

   if (DidParsePILStage) {
      P.diagnose(stageLoc, diag::multiple_pil_stage_decls);
      return false;
   }

   M.setStage(stage);
   DidParsePILStage = true;
   return false;
}

/// Lookup a global variable declaration from its demangled name.
///
/// A variable declaration exists for all pil_global variables defined in
/// Swift. A Swift global defined outside this module will be exposed
/// via an addressor rather than as a pil_global. Globals imported
/// from clang will produce a pil_global but will not have any corresponding
/// VarDecl.
///
/// FIXME: lookupGlobalDecl() can handle collisions between private or
/// fileprivate global variables in the same PIL Module, but the typechecker
/// will still incorrectly diagnose this as an "invalid redeclaration" and give
/// all but the first declaration an error type.
static Optional<VarDecl *> lookupGlobalDecl(Identifier GlobalName,
                                            PILLinkage GlobalLinkage,
                                            PILType GlobalType, Parser &P) {
   // Create a set of DemangleOptions to produce the global variable's
   // identifier, which is used as a search key in the declaration context.
   demangling::DemangleOptions demangleOpts;
   demangleOpts.QualifyEntities = false;
   demangleOpts.ShowPrivateDiscriminators = false;
   demangleOpts.DisplayEntityTypes = false;
   std::string GlobalDeclName = demangling::demangleSymbolAsString(
      GlobalName.str(), demangleOpts);

   SmallVector<ValueDecl *, 4> CurModuleResults;
   P.SF.getParentModule()->lookupValue(
      P.Context.getIdentifier(GlobalDeclName), NLKind::UnqualifiedLookup,
      CurModuleResults);
   // Bail-out on clang-imported globals.
   if (CurModuleResults.empty())
      return nullptr;

   // private and fileprivate globals of the same name may be merged into a
   // single PIL module. Find the declaration with the correct type and
   // linkage. (If multiple globals have the same type and linkage then it
   // doesn't matter which declaration we use).
   for (ValueDecl *ValDecl : CurModuleResults) {
      auto *VD = cast<VarDecl>(ValDecl);
      CanType DeclTy = VD->getType()->getCanonicalType();
      if (DeclTy == GlobalType.getAstType()
          && getDeclPILLinkage(VD) == GlobalLinkage) {
         return VD;
      }
   }
   return None;
}

/// decl-pil-global: [[only in PIL mode]]
///   'pil_global' pil-linkage @name : pil-type [external]
bool PILParserTUState::parsePILGlobal(Parser &P) {
   // Inform the lexer that we're lexing the body of the PIL declaration.
   Lexer::PILBodyRAII Tmp(*P.L);

   P.consumeToken(tok::kw_pil_global);
   Optional<PILLinkage> GlobalLinkage;
   Identifier GlobalName;
   PILType GlobalType;
   SourceLoc NameLoc;
   IsSerialized_t isSerialized = IsNotSerialized;
   bool isLet = false;

   Scope S(&P, ScopeKind::TopLevel);
   PILParser State(P);
   if (parsePILLinkage(GlobalLinkage, P) ||
       parseDeclPILOptional(nullptr, &isSerialized, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr,
                            &isLet, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, State, M) ||
       P.parseToken(tok::at_sign, diag::expected_pil_value_name) ||
       P.parseIdentifier(GlobalName, NameLoc, diag::expected_pil_value_name) ||
       P.parseToken(tok::colon, diag::expected_pil_type))
      return true;

   if (State.parsePILType(GlobalType))
      return true;

   // Non-external global variables are definitions by default.
   if (!GlobalLinkage.hasValue())
      GlobalLinkage = PILLinkage::DefaultForDefinition;

   // Lookup the global variable declaration for this pil_global.
   auto VD =
      lookupGlobalDecl(GlobalName, GlobalLinkage.getValue(), GlobalType, P);
   if (!VD) {
      P.diagnose(NameLoc, diag::pil_global_variable_not_found, GlobalName);
      return true;
   }
   auto *GV = PILGlobalVariable::create(
      M, GlobalLinkage.getValue(), isSerialized, GlobalName.str(), GlobalType,
      RegularLocation(NameLoc), VD.getValue());

   GV->setLet(isLet);
   // Parse static initializer if exists.
   if (State.P.consumeIf(tok::equal) && State.P.consumeIf(tok::l_brace)) {
      PILBuilder B(GV);
      do {
         State.parsePILInstruction(B);
      } while (! State.P.consumeIf(tok::r_brace));
   }
   return false;
}

/// decl-pil-property: [[only in PIL mode]]
///   'pil_property' pil-decl-ref '(' pil-key-path-pattern-component ')'

bool PILParserTUState::parsePILProperty(Parser &P) {
   Lexer::PILBodyRAII Tmp(*P.L);

   auto loc = P.consumeToken(tok::kw_pil_property);
   auto InstLoc = RegularLocation(loc);
   PILParser SP(P);

   IsSerialized_t Serialized = IsNotSerialized;
   if (parseDeclPILOptional(nullptr, &Serialized, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, SP, M))
      return true;

   ValueDecl *VD;

   if (SP.parsePILDottedPath(VD))
      return true;

   GenericParamList *generics;
   GenericEnvironment *patternEnv;
   Scope toplevelScope(&P, ScopeKind::TopLevel);
   Scope genericsScope(&P, ScopeKind::Generics);
   generics = P.maybeParseGenericParams().getPtrOrNull();
   patternEnv = handlePILGenericParams(generics, &P.SF);

   if (patternEnv) {
      if (patternEnv->getGenericSignature()->getCanonicalSignature()
          != VD->getInnermostDeclContext()->getGenericSignatureOfContext()
             ->getCanonicalSignature()) {
         P.diagnose(loc, diag::pil_property_generic_signature_mismatch);
         return true;
      }
   } else {
      if (VD->getInnermostDeclContext()->getGenericSignatureOfContext()) {
         P.diagnose(loc, diag::pil_property_generic_signature_mismatch);
         return true;
      }
   }

   Identifier ComponentKind;
   Optional<KeyPathPatternComponent> Component;
   SourceLoc ComponentLoc;
   SmallVector<PILType, 4> OperandTypes;

   if (P.parseToken(tok::l_paren, diag::expected_tok_in_pil_instr, "("))
      return true;

   if (!P.consumeIf(tok::r_paren)) {
      KeyPathPatternComponent parsedComponent;
      if (P.parseIdentifier(ComponentKind, ComponentLoc,
                            diag::expected_tok_in_pil_instr, "component kind")
          || SP.parseKeyPathPatternComponent(parsedComponent, OperandTypes,
                                             ComponentLoc, ComponentKind, InstLoc,
                                             patternEnv)
          || P.parseToken(tok::r_paren, diag::expected_tok_in_pil_instr, ")"))
         return true;

      Component = std::move(parsedComponent);
   }

   PILProperty::create(M, Serialized,
                       cast<AbstractStorageDecl>(VD), Component);
   return false;
}

/// decl-pil-vtable: [[only in PIL mode]]
///   'pil_vtable' ClassName decl-pil-vtable-body
/// decl-pil-vtable-body:
///   '{' pil-vtable-entry* '}'
/// pil-vtable-entry:
///   PILDeclRef ':' PILFunctionName
bool PILParserTUState::parsePILVTable(Parser &P) {
   P.consumeToken(tok::kw_pil_vtable);
   PILParser VTableState(P);

   IsSerialized_t Serialized = IsNotSerialized;
   if (parseDeclPILOptional(nullptr, &Serialized, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr,
                            VTableState, M))
      return true;

   // Parse the class name.
   Identifier Name;
   SourceLoc Loc;
   if (VTableState.parsePILIdentifier(Name, Loc,
                                      diag::expected_pil_value_name))
      return true;

   // Find the class decl.
   llvm::PointerUnion<ValueDecl*, ModuleDecl *> Res =
      lookupTopDecl(P, Name, /*typeLookup=*/true);
   assert(Res.is<ValueDecl*>() && "Class look-up should return a Decl");
   ValueDecl *VD = Res.get<ValueDecl*>();
   if (!VD) {
      P.diagnose(Loc, diag::pil_vtable_class_not_found, Name);
      return true;
   }

   auto *theClass = dyn_cast<ClassDecl>(VD);
   if (!theClass) {
      P.diagnose(Loc, diag::pil_vtable_class_not_found, Name);
      return true;
   }

   SourceLoc LBraceLoc = P.Tok.getLoc();
   P.consumeToken(tok::l_brace);

   // We need to turn on InPILBody to parse PILDeclRef.
   Lexer::PILBodyRAII Tmp(*P.L);
   Scope S(&P, ScopeKind::TopLevel);
   // Parse the entry list.
   std::vector<PILVTable::Entry> vtableEntries;
   if (P.Tok.isNot(tok::r_brace)) {
      do {
         PILDeclRef Ref;
         Identifier FuncName;
         SourceLoc FuncLoc;
         if (VTableState.parsePILDeclRef(Ref, true))
            return true;
         PILFunction *Func = nullptr;
         if (P.Tok.is(tok::kw_nil)) {
            P.consumeToken();
         } else {
            if (P.parseToken(tok::colon, diag::expected_pil_vtable_colon) ||
                P.parseToken(tok::at_sign, diag::expected_pil_function_name) ||
                VTableState.parsePILIdentifier(FuncName, FuncLoc,
                                               diag::expected_pil_value_name))
               return true;
            Func = M.lookUpFunction(FuncName.str());
            if (!Func) {
               P.diagnose(FuncLoc, diag::pil_vtable_func_not_found, FuncName);
               return true;
            }
         }

         auto Kind = PILVTable::Entry::Kind::Normal;
         if (P.Tok.is(tok::l_square)) {
            P.consumeToken(tok::l_square);
            if (P.Tok.isNot(tok::identifier)) {
               P.diagnose(P.Tok.getLoc(), diag::pil_vtable_bad_entry_kind);
               return true;
            }

            if (P.Tok.getText() == "override") {
               P.consumeToken();
               Kind = PILVTable::Entry::Kind::Override;
            } else if (P.Tok.getText() == "inherited") {
               P.consumeToken();
               Kind = PILVTable::Entry::Kind::Inherited;
            } else {
               P.diagnose(P.Tok.getLoc(), diag::pil_vtable_bad_entry_kind);
               return true;
            }

            if (P.parseToken(tok::r_square, diag::pil_vtable_expect_rsquare))
               return true;
         }

         vtableEntries.emplace_back(Ref, Func, Kind);
      } while (P.Tok.isNot(tok::r_brace) && P.Tok.isNot(tok::eof));
   }

   SourceLoc RBraceLoc;
   P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                        LBraceLoc);

   PILVTable::create(M, theClass, Serialized, vtableEntries);
   return false;
}

static InterfaceDecl *parseInterfaceDecl(Parser &P, PILParser &SP) {
   Identifier DeclName;
   SourceLoc DeclLoc;
   if (SP.parsePILIdentifier(DeclName, DeclLoc, diag::expected_pil_value_name))
      return nullptr;

   // Find the protocol decl. The protocol can be imported.
   llvm::PointerUnion<ValueDecl*, ModuleDecl *> Res =
      lookupTopDecl(P, DeclName, /*typeLookup=*/true);
   assert(Res.is<ValueDecl*>() && "Interface look-up should return a Decl");
   ValueDecl *VD = Res.get<ValueDecl*>();
   if (!VD) {
      P.diagnose(DeclLoc, diag::pil_witness_protocol_not_found, DeclName);
      return nullptr;
   }
   auto *proto = dyn_cast<InterfaceDecl>(VD);
   if (!proto)
      P.diagnose(DeclLoc, diag::pil_witness_protocol_not_found, DeclName);
   return proto;
}

static AssociatedTypeDecl *parseAssociatedTypeDecl(Parser &P, PILParser &SP,
                                                   InterfaceDecl *proto) {
   Identifier DeclName;
   SourceLoc DeclLoc;
   if (SP.parsePILIdentifier(DeclName, DeclLoc, diag::expected_pil_value_name))
      return nullptr;
   // We can return multiple decls, for now, we use the first lookup result.
   // One example is two decls when searching for Generator of Sequence:
   // one from Sequence, the other from _Sequence_Type.
   SmallVector<ValueDecl *, 4> values;
   auto VD = lookupMember(P, proto->getInterfaceType(), DeclName, DeclLoc,
                          values, true/*ExpectMultipleResults*/);
   if (!VD) {
      P.diagnose(DeclLoc, diag::pil_witness_assoc_not_found, DeclName);
      return nullptr;
   }
   return dyn_cast<AssociatedTypeDecl>(VD);
}

static bool parseAssociatedTypePath(PILParser &SP,
                                    SmallVectorImpl<Identifier> &path) {
   do {
      Identifier name;
      SourceLoc loc;
      if (SP.parsePILIdentifier(name, loc, diag::expected_pil_value_name))
         return false;
      path.push_back(name);
   } while (SP.P.consumeIf(tok::period));

   return true;
}

static bool matchesAssociatedTypePath(CanType assocType,
                                      ArrayRef<Identifier> path) {
   if (auto memberType = dyn_cast<DependentMemberType>(assocType)) {
      return (!path.empty() &&
              memberType->getName() == path.back() &&
              matchesAssociatedTypePath(memberType.getBase(), path.drop_back()));
   } else {
      assert(isa<GenericTypeParamType>(assocType));
      return path.empty();
   }
}

static CanType parseAssociatedTypePath(Parser &P, PILParser &SP,
                                       InterfaceDecl *proto) {
   SourceLoc loc = SP.P.Tok.getLoc();
   SmallVector<Identifier, 4> path;
   if (!parseAssociatedTypePath(SP, path))
      return CanType();

   // This is only used for parsing associated conformances, so we can
   // go ahead and just search the requirement signature for something that
   // matches the path.
   for (auto &reqt : proto->getRequirementSignature()) {
      if (reqt.getKind() != RequirementKind::Conformance)
         continue;
      CanType assocType = reqt.getFirstType()->getCanonicalType();
      if (matchesAssociatedTypePath(assocType, path))
         return assocType;
   }

   SmallString<128> name;
   name += path[0].str();
   for (auto elt : makeArrayRef(path).slice(1)) {
      name += '.';
      name += elt.str();
   }
   P.diagnose(loc, diag::pil_witness_assoc_conf_not_found, name);
   return CanType();
}

static bool isSelfConformance(Type conformingType, InterfaceDecl *protocol) {
   if (auto protoTy = conformingType->getAs<InterfaceType>())
      return protoTy->getDecl() == protocol;
   return false;
}

static InterfaceConformanceRef
parseRootInterfaceConformance(Parser &P, PILParser &SP, Type ConformingTy,
                              InterfaceDecl *&proto, ConformanceContext context) {
   Identifier ModuleKeyword, ModuleName;
   SourceLoc Loc, KeywordLoc;
   proto = parseInterfaceDecl(P, SP);
   if (!proto)
      return InterfaceConformanceRef();

   if (P.parseIdentifier(ModuleKeyword, KeywordLoc,
                         diag::expected_tok_in_pil_instr, "module") ||
       SP.parsePILIdentifier(ModuleName, Loc,
                             diag::expected_pil_value_name))
      return InterfaceConformanceRef();

   if (ModuleKeyword.str() != "module") {
      P.diagnose(KeywordLoc, diag::expected_tok_in_pil_instr, "module");
      return InterfaceConformanceRef();
   }

   // Calling lookupConformance on a BoundGenericType will return a specialized
   // conformance. We use UnboundGenericType to find the normal conformance.
   Type lookupTy = ConformingTy;
   if (auto bound = lookupTy->getAs<BoundGenericType>())
      lookupTy = bound->getDecl()->getDeclaredType();
   auto lookup = P.SF.getParentModule()->lookupConformance(lookupTy, proto);
   if (lookup.isInvalid()) {
      P.diagnose(KeywordLoc, diag::pil_witness_protocol_conformance_not_found);
      return InterfaceConformanceRef();
   }

   // Use a concrete self-conformance if we're parsing this for a witness table.
   if (context == ConformanceContext::WitnessTable && !lookup.isConcrete() &&
       isSelfConformance(ConformingTy, proto)) {
      lookup = InterfaceConformanceRef(P.Context.getSelfConformance(proto));
   }

   return lookup;
}

///  protocol-conformance ::= normal-protocol-conformance
///  protocol-conformance ::=
///    generic-parameter-list? type: 'inherit' '(' protocol-conformance ')'
///  protocol-conformance ::=
///    generic-parameter-list? type: 'specialize' '<' substitution* '>'
///    '(' protocol-conformance ')'
///  normal-protocol-conformance ::=
///    generic-parameter-list? type: protocolName module ModuleName
/// Note that generic-parameter-list is already parsed before calling this.
InterfaceConformanceRef PILParser::parseInterfaceConformance(
   InterfaceDecl *&proto, GenericEnvironment *&genericEnv,
   ConformanceContext context, InterfaceDecl *defaultForProto) {
   // Parse generic params for the protocol conformance. We need to make sure
   // they have the right scope.
   Optional<Scope> GenericsScope;
   if (context == ConformanceContext::Ordinary)
      GenericsScope.emplace(&P, ScopeKind::Generics);

   // Make sure we don't leave it uninitialized in the caller
   genericEnv = nullptr;

   auto *genericParams = P.maybeParseGenericParams().getPtrOrNull();
   if (genericParams) {
      genericEnv = handlePILGenericParams(genericParams, &P.SF);
   }

   auto retVal = parseInterfaceConformanceHelper(proto, genericEnv, context,
                                                 defaultForProto);

   if (GenericsScope) {
      GenericsScope.reset();
   }
   return retVal;
}

InterfaceConformanceRef PILParser::parseInterfaceConformanceHelper(
   InterfaceDecl *&proto, GenericEnvironment *witnessEnv,
   ConformanceContext context, InterfaceDecl *defaultForProto) {
   // Parse AST type.
   ParserResult<TypeRepr> TyR = P.parseType();
   if (TyR.isNull())
      return InterfaceConformanceRef();
   TypeLoc Ty = TyR.get();
   if (defaultForProto) {
      bindInterfaceSelfInTypeRepr(Ty, defaultForProto);
   }

   if (performTypeLocChecking(Ty, /*IsPILType=*/ false, witnessEnv,
                              defaultForProto))
      return InterfaceConformanceRef();
   auto ConformingTy = Ty.getType();

   if (P.parseToken(tok::colon, diag::expected_pil_witness_colon))
      return InterfaceConformanceRef();

   if (P.Tok.is(tok::identifier) && P.Tok.getText() == "specialize") {
      P.consumeToken();

      // Parse substitutions for specialized conformance.
      SmallVector<ParsedSubstitution, 4> parsedSubs;
      if (parseSubstitutions(parsedSubs, witnessEnv, defaultForProto))
         return InterfaceConformanceRef();

      if (P.parseToken(tok::l_paren, diag::expected_pil_witness_lparen))
         return InterfaceConformanceRef();
      InterfaceDecl *dummy;
      GenericEnvironment *specializedEnv;
      auto genericConform =
         parseInterfaceConformance(dummy, specializedEnv,
                                   ConformanceContext::Ordinary,
                                   defaultForProto);
      if (genericConform.isInvalid() || !genericConform.isConcrete())
         return InterfaceConformanceRef();
      if (P.parseToken(tok::r_paren, diag::expected_pil_witness_rparen))
         return InterfaceConformanceRef();

      SubstitutionMap subMap =
         getApplySubstitutionsFromParsed(*this, specializedEnv, parsedSubs);
      if (!subMap)
         return InterfaceConformanceRef();

      auto result = P.Context.getSpecializedConformance(
         ConformingTy, genericConform.getConcrete(), subMap);
      return InterfaceConformanceRef(result);
   }

   if (P.Tok.is(tok::identifier) && P.Tok.getText() == "inherit") {
      P.consumeToken();

      if (P.parseToken(tok::l_paren, diag::expected_pil_witness_lparen))
         return InterfaceConformanceRef();
      auto baseConform = parseInterfaceConformance(defaultForProto,
                                                   ConformanceContext::Ordinary);
      if (baseConform.isInvalid() || !baseConform.isConcrete())
         return InterfaceConformanceRef();
      if (P.parseToken(tok::r_paren, diag::expected_pil_witness_rparen))
         return InterfaceConformanceRef();

      auto result = P.Context.getInheritedConformance(ConformingTy,
                                                      baseConform.getConcrete());
      return InterfaceConformanceRef(result);
   }

   auto retVal =
      parseRootInterfaceConformance(P, *this, ConformingTy, proto, context);
   return retVal;
}

/// Parser a single PIL vtable entry and add it to either \p witnessEntries
/// or \c conditionalConformances.
static bool parsePILVTableEntry(
   Parser &P,
   PILModule &M,
   InterfaceDecl *proto,
   GenericEnvironment *witnessEnv,
   PILParser &witnessState,
   bool isDefaultWitnessTable,
   std::vector<PILWitnessTable::Entry> &witnessEntries,
   std::vector<PILWitnessTable::ConditionalConformance>
   &conditionalConformances) {
   InterfaceDecl *defaultForProto = isDefaultWitnessTable ? proto : nullptr;
   Identifier EntryKeyword;
   SourceLoc KeywordLoc;
   if (P.parseIdentifier(EntryKeyword, KeywordLoc,
                         diag::expected_tok_in_pil_instr,
                         "method, associated_type, associated_type_protocol, base_protocol"
                         ", no_default"))
      return true;

   if (EntryKeyword.str() == "no_default") {
      witnessEntries.push_back(PILDefaultWitnessTable::Entry());
      return false;
   }

   if (EntryKeyword.str() == "base_protocol") {
      InterfaceDecl *proto = parseInterfaceDecl(P, witnessState);
      if (!proto)
         return true;
      if (P.parseToken(tok::colon, diag::expected_pil_witness_colon))
         return true;
      auto conform =
         witnessState.parseInterfaceConformance(defaultForProto,
                                                ConformanceContext::Ordinary);
      // Ignore invalid and abstract witness entries.
      if (conform.isInvalid() || !conform.isConcrete())
         return false;

      witnessEntries.push_back(
         PILWitnessTable::BaseInterfaceWitness{proto, conform.getConcrete()});
      return false;
   }

   if (EntryKeyword.str() == "associated_type_protocol" ||
       EntryKeyword.str() == "conditional_conformance") {
      if (P.parseToken(tok::l_paren, diag::expected_pil_witness_lparen))
         return true;
      CanType assocOrSubject;
      if (EntryKeyword.str() == "associated_type_protocol") {
         assocOrSubject = parseAssociatedTypePath(P, witnessState, proto);
      } else {
         // Parse AST type.
         ParserResult<TypeRepr> TyR = P.parseType();
         if (TyR.isNull())
            return true;
         TypeLoc Ty = TyR.get();
         if (isDefaultWitnessTable)
            bindInterfaceSelfInTypeRepr(Ty, proto);
         if (polar::performTypeLocChecking(P.Context, Ty,
            /*isPILMode=*/false,
            /*isPILType=*/false,
                                           witnessEnv,
                                           &P.SF))
            return true;

         assocOrSubject = Ty.getType()->getCanonicalType();
      }
      if (!assocOrSubject)
         return true;
      if (P.parseToken(tok::colon, diag::expected_pil_witness_colon))
         return true;
      InterfaceDecl *proto = parseInterfaceDecl(P, witnessState);
      if (!proto)
         return true;
      if (P.parseToken(tok::r_paren, diag::expected_pil_witness_rparen) ||
          P.parseToken(tok::colon, diag::expected_pil_witness_colon))
         return true;

      InterfaceConformanceRef conformance(proto);
      if (P.Tok.getText() != "dependent") {
         auto concrete =
            witnessState.parseInterfaceConformance(defaultForProto,
                                                   ConformanceContext::Ordinary);
         // Ignore invalid and abstract witness entries.
         if (concrete.isInvalid() || !concrete.isConcrete())
            return false;
         conformance = concrete;
      } else {
         P.consumeToken();
      }

      if (EntryKeyword.str() == "associated_type_protocol")
         witnessEntries.push_back(
            PILWitnessTable::AssociatedTypeInterfaceWitness{assocOrSubject,
                                                            proto,
                                                            conformance});
      else
         conditionalConformances.push_back(
            PILWitnessTable::ConditionalConformance{assocOrSubject,
                                                    conformance});

      return false;
   }

   if (EntryKeyword.str() == "associated_type") {
      AssociatedTypeDecl *assoc = parseAssociatedTypeDecl(P, witnessState,
                                                          proto);
      if (!assoc)
         return true;
      if (P.parseToken(tok::colon, diag::expected_pil_witness_colon))
         return true;

      // Parse AST type.
      ParserResult<TypeRepr> TyR = P.parseType();
      if (TyR.isNull())
         return true;
      TypeLoc Ty = TyR.get();
      if (isDefaultWitnessTable)
         bindInterfaceSelfInTypeRepr(Ty, proto);
      if (polar::performTypeLocChecking(P.Context, Ty,
         /*isPILMode=*/false,
         /*isPILType=*/false,
                                        witnessEnv,
                                        &P.SF))
         return true;

      witnessEntries.push_back(PILWitnessTable::AssociatedTypeWitness{
         assoc, Ty.getType()->getCanonicalType()
      });
      return false;
   }

   if (EntryKeyword.str() != "method") {
      P.diagnose(KeywordLoc, diag::expected_tok_in_pil_instr, "method");
      return true;
   }

   PILDeclRef Ref;
   Identifier FuncName;
   SourceLoc FuncLoc;
   if (witnessState.parsePILDeclRef(Ref, true) ||
       P.parseToken(tok::colon, diag::expected_pil_witness_colon))
      return true;

   PILFunction *Func = nullptr;
   if (P.Tok.is(tok::kw_nil)) {
      P.consumeToken();
   } else {
      if (P.parseToken(tok::at_sign, diag::expected_pil_function_name) ||
          witnessState.parsePILIdentifier(FuncName, FuncLoc,
                                          diag::expected_pil_value_name))
         return true;

      Func = M.lookUpFunction(FuncName.str());
      if (!Func) {
         P.diagnose(FuncLoc, diag::pil_witness_func_not_found, FuncName);
         return true;
      }
   }
   witnessEntries.push_back(PILWitnessTable::MethodWitness{
      Ref, Func
   });

   return false;
}

/// decl-pil-witness ::= 'pil_witness_table' pil-linkage?
///                      normal-protocol-conformance decl-pil-witness-body
/// normal-protocol-conformance ::=
///   generic-parameter-list? type: protocolName module ModuleName
/// decl-pil-witness-body:
///   '{' pil-witness-entry* '}'
/// pil-witness-entry:
///   method PILDeclRef ':' @PILFunctionName
///   associated_type AssociatedTypeDeclName: Type
///   associated_type_protocol (AssocName: InterfaceName):
///                              protocol-conformance|dependent
///   base_protocol InterfaceName: protocol-conformance
bool PILParserTUState::parsePILWitnessTable(Parser &P) {
   P.consumeToken(tok::kw_pil_witness_table);
   PILParser WitnessState(P);

   // Parse the linkage.
   Optional<PILLinkage> Linkage;
   parsePILLinkage(Linkage, P);

   IsSerialized_t isSerialized = IsNotSerialized;
   if (parseDeclPILOptional(nullptr, &isSerialized, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr,
                            WitnessState, M))
      return true;

   Scope S(&P, ScopeKind::TopLevel);
   // We should use WitnessTableBody. This ensures that the generic params
   // are visible.
   Optional<Scope> BodyScope;
   BodyScope.emplace(&P, ScopeKind::FunctionBody);

   // Parse the protocol conformance.
   InterfaceDecl *proto;
   GenericEnvironment *witnessEnv;
   auto conf = WitnessState.parseInterfaceConformance(proto,
                                                      witnessEnv,
                                                      ConformanceContext::WitnessTable,
                                                      nullptr);
   WitnessState.ContextGenericEnv = witnessEnv;

   // FIXME: should we really allow a specialized or inherited conformance here?
   RootInterfaceConformance *theConformance = nullptr;
   if (conf.isConcrete())
      theConformance = conf.getConcrete()->getRootConformance();

   PILWitnessTable *wt = nullptr;
   if (theConformance) {
      wt = M.lookUpWitnessTable(theConformance, false);
      assert((!wt || wt->isDeclaration()) &&
             "Attempting to create duplicate witness table.");
   }

   // If we don't have an lbrace, then this witness table is a declaration.
   if (P.Tok.getKind() != tok::l_brace) {
      // Default to public external linkage.
      if (!Linkage)
         Linkage = PILLinkage::PublicExternal;
      // We ignore empty witness table without normal protocol conformance.
      if (!wt && theConformance)
         wt = PILWitnessTable::create(M, *Linkage, theConformance);
      BodyScope.reset();
      return false;
   }

   if (!theConformance) {
      P.diagnose(P.Tok, diag::pil_witness_protocol_conformance_not_found);
      return true;
   }

   SourceLoc LBraceLoc = P.Tok.getLoc();
   P.consumeToken(tok::l_brace);

   // We need to turn on InPILBody to parse PILDeclRef.
   Lexer::PILBodyRAII Tmp(*P.L);
   // Parse the entry list.
   std::vector<PILWitnessTable::Entry> witnessEntries;
   std::vector<PILWitnessTable::ConditionalConformance> conditionalConformances;

   if (P.Tok.isNot(tok::r_brace)) {
      do {
         if (parsePILVTableEntry(P, M, proto, witnessEnv, WitnessState, false,
                                 witnessEntries, conditionalConformances))
            return true;
      } while (P.Tok.isNot(tok::r_brace) && P.Tok.isNot(tok::eof));
   }

   SourceLoc RBraceLoc;
   P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                        LBraceLoc);

   // Default to public linkage.
   if (!Linkage)
      Linkage = PILLinkage::Public;

   if (!wt)
      wt = PILWitnessTable::create(M, *Linkage, theConformance);
   else
      wt->setLinkage(*Linkage);
   wt->convertToDefinition(witnessEntries, conditionalConformances,
                           isSerialized);
   BodyScope.reset();
   return false;
}

/// decl-pil-default-witness ::= 'pil_default_witness_table'
///                              pil-linkage identifier
///                              decl-pil-default-witness-body
/// decl-pil-default-witness-body:
///   '{' pil-default-witness-entry* '}'
/// pil-default-witness-entry:
///   pil-witness-entry
///   'no_default'
bool PILParserTUState::parsePILDefaultWitnessTable(Parser &P) {
   P.consumeToken(tok::kw_pil_default_witness_table);
   PILParser WitnessState(P);

   // Parse the linkage.
   Optional<PILLinkage> Linkage;
   parsePILLinkage(Linkage, P);

   Scope S(&P, ScopeKind::TopLevel);
   // We should use WitnessTableBody. This ensures that the generic params
   // are visible.
   Optional<Scope> BodyScope;
   BodyScope.emplace(&P, ScopeKind::FunctionBody);

   // Parse the protocol.
   InterfaceDecl *protocol = parseInterfaceDecl(P, WitnessState);
   if (!protocol)
      return true;

   // Parse the body.
   SourceLoc LBraceLoc = P.Tok.getLoc();
   P.consumeToken(tok::l_brace);

   // We need to turn on InPILBody to parse PILDeclRef.
   Lexer::PILBodyRAII Tmp(*P.L);

   // Parse the entry list.
   std::vector<PILWitnessTable::Entry> witnessEntries;
   std::vector<PILWitnessTable::ConditionalConformance> conditionalConformances;

   if (P.Tok.isNot(tok::r_brace)) {
      do {
         if (parsePILVTableEntry(P, M, protocol, protocol->getGenericEnvironment(),
                                 WitnessState, true, witnessEntries,
                                 conditionalConformances))
            return true;
      } while (P.Tok.isNot(tok::r_brace) && P.Tok.isNot(tok::eof));
   }

   SourceLoc RBraceLoc;
   P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                        LBraceLoc);

   // Default to public linkage.
   if (!Linkage)
      Linkage = PILLinkage::Public;

   PILDefaultWitnessTable::create(M, *Linkage, protocol, witnessEntries);
   BodyScope.reset();
   return false;
}

llvm::Optional<llvm::coverage::Counter> PILParser::parsePILCoverageExpr(
   llvm::coverage::CounterExpressionBuilder &Builder) {
   if (P.Tok.is(tok::integer_literal)) {
      unsigned CounterId;
      if (parseInteger(CounterId, diag::pil_coverage_invalid_counter))
         return None;
      return llvm::coverage::Counter::getCounter(CounterId);
   }

   if (P.Tok.is(tok::identifier)) {
      Identifier Zero;
      SourceLoc Loc;
      if (parsePILIdentifier(Zero, Loc, diag::pil_coverage_invalid_counter))
         return None;
      if (Zero.str() != "zero") {
         P.diagnose(Loc, diag::pil_coverage_invalid_counter);
         return None;
      }
      return llvm::coverage::Counter::getZero();
   }

   if (P.Tok.is(tok::l_paren)) {
      P.consumeToken(tok::l_paren);
      auto LHS = parsePILCoverageExpr(Builder);
      if (!LHS)
         return None;
      Identifier Operator;
      SourceLoc Loc;
      if (P.parseAnyIdentifier(Operator, Loc,
                               diag::pil_coverage_invalid_operator))
         return None;
      if (Operator.str() != "+" && Operator.str() != "-") {
         P.diagnose(Loc, diag::pil_coverage_invalid_operator);
         return None;
      }
      auto RHS = parsePILCoverageExpr(Builder);
      if (!RHS)
         return None;
      if (P.parseToken(tok::r_paren, diag::pil_coverage_expected_rparen))
         return None;

      if (Operator.str() == "+")
         return Builder.add(*LHS, *RHS);
      return Builder.subtract(*LHS, *RHS);
   }

   P.diagnose(P.Tok, diag::pil_coverage_invalid_counter);
   return None;
}

/// decl-pil-coverage-map ::= 'pil_coverage_map' CoveredName PGOFuncName CoverageHash
///                           decl-pil-coverage-body
/// decl-pil-coverage-body:
///   '{' pil-coverage-entry* '}'
/// pil-coverage-entry:
///   pil-coverage-loc ':' pil-coverage-expr
/// pil-coverage-loc:
///   StartLine ':' StartCol '->' EndLine ':' EndCol
/// pil-coverage-expr:
///   ...
bool PILParserTUState::parsePILCoverageMap(Parser &P) {
   P.consumeToken(tok::kw_pil_coverage_map);
   PILParser State(P);

   // Parse the filename.
   Identifier Filename;
   SourceLoc FileLoc;
   if (State.parsePILIdentifier(Filename, FileLoc,
                                diag::expected_pil_value_name))
      return true;

   // Parse the covered name.
   if (!P.Tok.is(tok::string_literal)) {
      P.diagnose(P.Tok, diag::pil_coverage_expected_quote);
      return true;
   }
   StringRef FuncName = P.Tok.getText().drop_front().drop_back();
   P.consumeToken();

   // Parse the PGO func name.
   if (!P.Tok.is(tok::string_literal)) {
      P.diagnose(P.Tok, diag::pil_coverage_expected_quote);
      return true;
   }
   StringRef PGOFuncName = P.Tok.getText().drop_front().drop_back();
   P.consumeToken();

   uint64_t Hash;
   if (State.parseInteger(Hash, diag::pil_coverage_invalid_hash))
      return true;

   if (!P.Tok.is(tok::l_brace)) {
      P.diagnose(P.Tok, diag::pil_coverage_expected_lbrace);
      return true;
   }
   SourceLoc LBraceLoc = P.Tok.getLoc();
   P.consumeToken(tok::l_brace);

   llvm::coverage::CounterExpressionBuilder Builder;
   std::vector<PILCoverageMap::MappedRegion> Regions;
   bool BodyHasError = false;
   if (P.Tok.isNot(tok::r_brace)) {
      do {
         unsigned StartLine, StartCol, EndLine, EndCol;
         if (State.parseInteger(StartLine, diag::pil_coverage_expected_loc) ||
             P.parseToken(tok::colon, diag::pil_coverage_expected_loc) ||
             State.parseInteger(StartCol, diag::pil_coverage_expected_loc) ||
             P.parseToken(tok::arrow, diag::pil_coverage_expected_arrow) ||
             State.parseInteger(EndLine, diag::pil_coverage_expected_loc) ||
             P.parseToken(tok::colon, diag::pil_coverage_expected_loc) ||
             State.parseInteger(EndCol, diag::pil_coverage_expected_loc)) {
            BodyHasError = true;
            break;
         }

         if (P.parseToken(tok::colon, diag::pil_coverage_expected_colon)) {
            BodyHasError = true;
            break;
         }

         auto Counter = State.parsePILCoverageExpr(Builder);
         if (!Counter) {
            BodyHasError = true;
            break;
         }

         Regions.emplace_back(StartLine, StartCol, EndLine, EndCol, *Counter);
      } while (P.Tok.isNot(tok::r_brace) && P.Tok.isNot(tok::eof));
   }
   if (BodyHasError)
      P.skipUntilDeclRBrace();

   SourceLoc RBraceLoc;
   P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                        LBraceLoc);

   if (!BodyHasError)
      PILCoverageMap::create(M, Filename.str(), FuncName.str(), PGOFuncName.str(),
                             Hash, Regions, Builder.getExpressions());
   return false;
}

/// pil-scope-ref ::= 'scope' [0-9]+
/// pil-scope ::= 'pil_scope' [0-9]+ '{'
///                 debug-loc
///                 'parent' scope-parent
///                 ('inlined_at' pil-scope-ref)?
///               '}'
/// scope-parent ::= pil-function-name ':' pil-type
/// scope-parent ::= pil-scope-ref
/// debug-loc ::= 'loc' string-literal ':' [0-9]+ ':' [0-9]+
bool PILParserTUState::parsePILScope(Parser &P) {
   P.consumeToken(tok::kw_pil_scope);
   PILParser ScopeState(P);

   SourceLoc SlotLoc = P.Tok.getLoc();
   unsigned Slot;
   if (ScopeState.parseInteger(Slot, diag::pil_invalid_scope_slot))
      return true;

   SourceLoc LBraceLoc = P.Tok.getLoc();
   P.consumeToken(tok::l_brace);

   StringRef Key = P.Tok.getText();
   RegularLocation Loc{PILLocation::DebugLoc()};
   if (Key == "loc")
      if (ScopeState.parsePILLocation(Loc))
         return true;
   ScopeState.parseVerbatim("parent");
   Identifier FnName;
   PILDebugScope *Parent = nullptr;
   PILFunction *ParentFn = nullptr;
   if (P.Tok.is(tok::integer_literal)) {
      /// scope-parent ::= pil-scope-ref
      if (ScopeState.parseScopeRef(Parent))
         return true;
   } else {
      /// scope-parent ::= pil-function-name
      PILType Ty;
      SourceLoc FnLoc = P.Tok.getLoc();
      // We need to turn on InPILBody to parse the function reference.
      Lexer::PILBodyRAII Tmp(*P.L);
      GenericEnvironment *IgnoredEnv;
      Scope S(&P, ScopeKind::TopLevel);
      Scope Body(&P, ScopeKind::FunctionBody);
      if ((ScopeState.parseGlobalName(FnName)) ||
          P.parseToken(tok::colon, diag::expected_pil_colon_value_ref) ||
          ScopeState.parsePILType(Ty, IgnoredEnv, true))
         return true;

      // The function doesn't exist yet. Create a zombie forward declaration.
      auto FnTy = Ty.getAs<PILFunctionType>();
      if (!FnTy || !Ty.isObject()) {
         P.diagnose(FnLoc, diag::expected_pil_function_type);
         return true;
      }
      ParentFn = ScopeState.getGlobalNameForReference(FnName, FnTy, FnLoc, true);
      ScopeState.TUState.PotentialZombieFns.insert(ParentFn);
   }

   PILDebugScope *InlinedAt = nullptr;
   if (P.Tok.getText() == "inlined_at") {
      P.consumeToken();
      if (ScopeState.parseScopeRef(InlinedAt))
         return true;
   }

   SourceLoc RBraceLoc;
   P.parseMatchingToken(tok::r_brace, RBraceLoc, diag::expected_pil_rbrace,
                        LBraceLoc);

   auto &Scope = ScopeSlots[Slot];
   if (Scope) {
      P.diagnose(SlotLoc, diag::pil_scope_redefined, Slot);
      return true;
   }

   Scope = new (M) PILDebugScope(Loc, ParentFn, Parent, InlinedAt);
   return false;
}
