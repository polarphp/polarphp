//===--- Subsystems.h - Swift Compiler Subsystem Entrypoints ----*- C++ -*-===//
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
//  This file declares the main entrypoints to the various subsystems.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_GLOBAL_SUBSYSTEMS_H
#define POLARPHP_GLOBAL_SUBSYSTEMS_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OptionSet.h"
#include "polarphp/basic/PrimarySpecificPaths.h"
#include "polarphp/basic/Version.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Mutex.h"

#include <memory>

namespace llvm {
class GlobalVariable;
class MemoryBuffer;
class Module;
class TargetOptions;
class TargetMachine;
}

namespace polar::llparser {
class Parser;
class Token;
class PersistentParserState;
class CodeCompletionCallbacksFactory;
}

namespace polar {
class GenericSignatureBuilder;
class AstContext;
class Decl;
class DeclContext;
class DiagnosticConsumer;
class DiagnosticEngine;
class Evaluator;
class FileUnit;
class GenericEnvironment;
class GenericParamList;
class IRGenOptions;
class ModuleDecl;
typedef void *OpaqueSyntaxNode;
class SerializationOptions;
class SourceFile;
class PILOptions;
class PILParserTUState;
class SyntaxParseActions;
class SyntaxParsingCache;
class TypeChecker;
struct TypeLoc;
enum class SourceFileKind;
class TypeCheckerOptions;
class LangOptions;
class UnifiedStatsReporter;
class SourceManager;
class PILModule;
using polar::PrimarySpecificPaths;
using polar::llparser::Parser;
using polar::llparser::Token;
using polar::llparser::PersistentParserState;
using polar::llparser::CodeCompletionCallbacksFactory;

namespace lowering {
class TypeConverter;
}

/// Used to optionally maintain PIL parsing context for the parser.
///
/// When not parsing PIL, this has no overhead.
class PILParserState {
public:
   std::unique_ptr<PILParserTUState> Impl;

   explicit PILParserState(PILModule *M);

   ~PILParserState();
};

/// @{

/// \returns true if the declaration should be verified.  This can return
/// false to decrease the number of declarations we verify in a single
/// compilation.
bool shouldVerify(const Decl *D, const AstContext &Context);

/// Check that the source file is well-formed, aborting and spewing
/// errors if not.
///
/// "Well-formed" here means following the invariants of the AST, not that the
/// code written by the user makes sense.
void verify(SourceFile &SF);

void verify(Decl *D);

/// @}

/// Parse a single buffer into the given source file.
///
/// If the source file is the main file, stop parsing after the next
/// stmt-brace-item with side-effects.
///
/// \param SF the file within the module being parsed.
///
/// \param BufferID the buffer to parse from.
///
/// \param[out] Done set to \c true if end of the buffer was reached.
///
/// \param PIL if non-null, we're parsing a PIL file.
///
/// \param PersistentState if non-null the same PersistentState object can
/// be used to resume parsing or parse delayed function bodies.
///
/// \return true if the parser found code with side effects.
bool parseIntoSourceFile(SourceFile &SF, unsigned BufferID, bool *Done,
                         PILParserState *PIL = nullptr,
                         PersistentParserState *PersistentState = nullptr,
                         bool DelayBodyParsing = true);

/// Parse a single buffer into the given source file, until the full source
/// contents are parsed.
///
/// \return true if the parser found code with side effects.
bool parseIntoSourceFileFull(SourceFile &SF, unsigned BufferID,
                             PersistentParserState *PersistentState = nullptr,
                             bool DelayBodyParsing = true);

/// Finish the code completion.
void performCodeCompletionSecondPass(PersistentParserState &PersistentState,
                                     CodeCompletionCallbacksFactory &Factory);

}

namespace polar::llparser {

using polar::LangOptions;
using polar::SourceManager;
using polar::DiagnosticEngine;
using llvm::ArrayRef;

/// Lex and return a vector of tokens for the given buffer.
std::vector<Token> tokenize(const LangOptions &LangOpts,
                            const SourceManager &SM, unsigned BufferID,
                            unsigned Offset = 0, unsigned EndOffset = 0,
                            DiagnosticEngine *Diags = nullptr,
                            bool KeepComments = true,
                            bool TokenizeInterpolatedString = true,
                            ArrayRef<Token> SplitTokens = ArrayRef<Token>());
}

namespace polar {

/// Once parsing is complete, this walks the AST to resolve imports, record
/// operators, and do other top-level validation.
///
/// \param StartElem Where to start for incremental name binding in the main
///                  source file.
void performNameBinding(SourceFile &SF, unsigned StartElem = 0);

/// Once type-checking is complete, this instruments code with calls to an
/// intrinsic that record the expected values of local variables so they can
/// be compared against the results from the debugger.
void performDebuggerTestingTransform(SourceFile &SF);

/// Once parsing and name-binding are complete, this optionally transforms the
/// ASTs to add calls to external logging functions.
///
/// \param HighPerformance True if the playground transform should omit
/// instrumentation that has a high runtime performance impact.
void performPlaygroundTransform(SourceFile &SF, bool HighPerformance);

/// Once parsing and name-binding are complete this optionally walks the ASTs
/// to add calls to externally provided functions that simulate
/// "program counter"-like debugging events.
void performPCMacro(SourceFile &SF);

/// Creates a type checker instance on the given AST context, if it
/// doesn't already have one.
///
/// \returns a reference to the type checker instance.
TypeChecker &createTypeChecker(AstContext &Ctx);

/// Bind all 'extension' visible from \p SF to the extended nominal.
void bindExtensions(SourceFile &SF);

/// Once parsing and name-binding are complete, this walks the AST to resolve
/// types and diagnose problems therein.
///
/// \param StartElem Where to start for incremental type-checking in the main
/// source file.
void performTypeChecking(SourceFile &SF, unsigned StartElem = 0);

/// Now that we have type-checked an entire module, perform any type
/// checking that requires the full module, e.g., Objective-C method
/// override checking.
///
/// Note that clients still perform this checking file-by-file to
/// provide a somewhat defined order in which diagnostics should be
/// emitted.
void performWholeModuleTypeChecking(SourceFile &SF);

/// Checks to see if any of the imports in \p M use `@_implementationOnly` in
/// one file and not in another.
///
/// Like redeclaration checking, but for imports. This isn't part of
/// swift::performWholeModuleTypeChecking because it's linear in the number
/// of declarations in the module.
void checkInconsistentImplementationOnlyImports(ModuleDecl *M);

/// Recursively validate the specified type.
///
/// This is used when dealing with partial source files (e.g. PIL parsing,
/// code completion).
///
/// \returns false on success, true on error.
bool performTypeLocChecking(AstContext &Ctx, TypeLoc &T,
                            DeclContext *DC,
                            bool ProduceDiagnostics = true);

/// Recursively validate the specified type.
///
/// This is used when dealing with partial source files (e.g. PIL parsing,
/// code completion).
///
/// \returns false on success, true on error.
bool performTypeLocChecking(AstContext &Ctx, TypeLoc &T,
                            bool isPILMode,
                            bool isPILType,
                            GenericEnvironment *GenericEnv,
                            DeclContext *DC,
                            bool ProduceDiagnostics = true);

/// Expose TypeChecker's handling of GenericParamList to PIL parsing.
GenericEnvironment *handlePILGenericParams(GenericParamList *genericParams,
                                           DeclContext *DC);

/// Turn the given module into PIL IR.
///
/// The module must contain source files. The optimizer will assume that the
/// PIL of all files in the module is present in the PILModule.
std::unique_ptr<PILModule>
performPILGeneration(ModuleDecl *M, lowering::TypeConverter &TC,
                     PILOptions &options);

/// Turn a source file into PIL IR.
std::unique_ptr<PILModule>
performPILGeneration(FileUnit &SF, lowering::TypeConverter &TC,
                     PILOptions &options);

using ModuleOrSourceFile = PointerUnion<ModuleDecl *, SourceFile *>;

/// Serializes a module or single source file to the given output file.
void serialize(ModuleOrSourceFile DC, const SerializationOptions &options,
               const PILModule *M = nullptr);

/// Serializes a module or single source file to the given output file and
/// returns back the file's contents as a memory buffer.
///
/// Use this if you intend to immediately load the serialized module, as that
/// will both avoid extra filesystem traffic and will ensure you read back
/// exactly what was written.
void serializeToBuffers(ModuleOrSourceFile DC,
                        const SerializationOptions &opts,
                        std::unique_ptr<llvm::MemoryBuffer> *moduleBuffer,
                        std::unique_ptr<llvm::MemoryBuffer> *moduleDocBuffer,
                        std::unique_ptr<llvm::MemoryBuffer> *moduleSourceInfoBuffer,
                        const PILModule *M = nullptr);

/// Get the CPU, subtarget feature options, and triple to use when emitting code.
std::tuple<llvm::TargetOptions, std::string, std::vector<std::string>,
std::string>
getIRTargetOptions(IRGenOptions &Opts, AstContext &Ctx);

/// Turn the given Swift module into either LLVM IR or native code
/// and return the generated LLVM IR module.
/// If you set an outModuleHash, then you need to call performLLVM.
std::unique_ptr<llvm::Module>
performIRGeneration(IRGenOptions &Opts, ModuleDecl *M,
                    std::unique_ptr<PILModule> PILMod,
                    StringRef ModuleName, const PrimarySpecificPaths &PSPs,
                    llvm::LLVMContext &LLVMContext,
                    ArrayRef<std::string> parallelOutputFilenames,
                    llvm::GlobalVariable **outModuleHash = nullptr);

/// Turn the given Swift module into either LLVM IR or native code
/// and return the generated LLVM IR module.
/// If you set an outModuleHash, then you need to call performLLVM.
std::unique_ptr<llvm::Module>
performIRGeneration(IRGenOptions &Opts, SourceFile &SF,
                    std::unique_ptr<PILModule> PILMod,
                    StringRef ModuleName, const PrimarySpecificPaths &PSPs,
                    llvm::LLVMContext &LLVMContext,
                    llvm::GlobalVariable **outModuleHash = nullptr);

/// Given an already created LLVM module, construct a pass pipeline and run
/// the Swift LLVM Pipeline upon it. This does not cause the module to be
/// printed, only to be optimized.
void performLLVMOptimizations(IRGenOptions &Opts, llvm::Module *Module,
                              llvm::TargetMachine *TargetMachine);

/// Wrap a serialized module inside a swift AST section in an object file.
void createTypePHPModuleObjectFile(PILModule &PILMod, StringRef Buffer,
                                    StringRef OutputPath);

/// Turn the given LLVM module into native code and return true on error.
bool performLLVM(IRGenOptions &Opts, AstContext &Ctx, llvm::Module *Module,
                 StringRef OutputFilename,
                 UnifiedStatsReporter *Stats=nullptr);

/// Run the LLVM passes. In multi-threaded compilation this will be done for
/// multiple LLVM modules in parallel.
/// \param Diags may be null if LLVM code gen diagnostics are not required.
/// \param DiagMutex may also be null if a mutex around \p Diags is not
///                  required.
/// \param HashGlobal used with incremental LLVMCodeGen to know if a module
///                   was already compiled, may be null if not desired.
/// \param Module LLVM module to code gen, required.
/// \param TargetMachine target of code gen, required.
/// \param effectiveLanguageVersion version of the language, effectively.
/// \param OutputFilename Filename for output.
bool performLLVM(IRGenOptions &Opts, DiagnosticEngine *Diags,
                 llvm::sys::Mutex *DiagMutex,
                 llvm::GlobalVariable *HashGlobal,
                 llvm::Module *Module,
                 llvm::TargetMachine *TargetMachine,
                 const version::Version &effectiveLanguageVersion,
                 StringRef OutputFilename,
                 UnifiedStatsReporter *Stats=nullptr);

/// Dump YAML describing all fixed-size types imported from the given module.
bool performDumpTypeInfo(IRGenOptions &Opts,
                         PILModule &PILMod,
                         llvm::LLVMContext &LLVMContext);

/// Creates a TargetMachine from the IRGen opts and AST Context.
std::unique_ptr<llvm::TargetMachine>
createTargetMachine(IRGenOptions &Opts, AstContext &Ctx);

/// A convenience wrapper for Parser functionality.
class ParserUnit {
public:
   ParserUnit(SourceManager &SM, SourceFileKind SFKind, unsigned BufferID,
              const LangOptions &LangOpts, const TypeCheckerOptions &TyOpts,
              StringRef ModuleName,
              std::shared_ptr<SyntaxParseActions> spActions = nullptr
              /* SyntaxParsingCache *SyntaxCache = nullptr*/);
   ParserUnit(SourceManager &SM, SourceFileKind SFKind, unsigned BufferID);
   ParserUnit(SourceManager &SM, SourceFileKind SFKind, unsigned BufferID,
              unsigned Offset, unsigned EndOffset);

   ~ParserUnit();
   /// TODO

//   OpaqueSyntaxNode parse();
   void parse();

   Parser &getParser();
   SourceFile &getSourceFile();
   DiagnosticEngine &getDiagnosticEngine();
   const LangOptions &getLangOptions() const;

private:
   struct Implementation;
   Implementation &Impl;
};

/// Register AST-level request functions with the evaluator.
///
/// The AstContext will automatically call these upon construction.
void registerAccessRequestFunctions(Evaluator &evaluator);

/// Register AST-level request functions with the evaluator.
///
/// The AstContext will automatically call these upon construction.
void registerNameLookupRequestFunctions(Evaluator &evaluator);

/// Register Parse-level request functions with the evaluator.
///
/// Clients that form an AstContext and will perform any parsing queries
/// using Parse-level logic should call these functions after forming the
/// AstContext.
void registerParseRequestFunctions(Evaluator &evaluator);

/// Register Sema-level request functions with the evaluator.
///
/// Clients that form an AstContext and will perform any semantic queries
/// using Sema-level logic should call these functions after forming the
/// AstContext.
void registerTypeCheckerRequestFunctions(Evaluator &evaluator);

/// Register IDE-level request functions with the evaluator.
///
/// The AstContext will automatically call these upon construction.
void registerIDERequestFunctions(Evaluator &evaluator);

/// Register type check request functions for IDE's usage with the evaluator.
///
/// The AstContext will automatically call these upon construction.
/// Calling registerIDERequestFunctions will invoke this function as well.
void registerIDETypeCheckRequestFunctions(Evaluator &evaluator);

} // end namespace polar

#endif // POLARPHP_GLOBAL_SUBSYSTEMS_H
