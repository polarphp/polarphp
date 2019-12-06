//===--- ExperimentalDependenciesSourceFileDepGraphConstructor.cpp --------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/03.

#include <stdio.h>

// may not all be needed
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/ast/ExperimentalDependencies.h"
#include "polarphp/ast/Filesystem.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ModuleLoader.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/Filesystem.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/frontend/FrontendOptions.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/YAMLParser.h"

// This file holds the code to build a SourceFileDepGraph in the frontend.
// This graph captures relationships between definitions and uses, and
// it is written to a file which is read by the driver in order to decide which
// source files require recompilation.

namespace polar::ast {
namespace experimentaldependencies {

//==============================================================================
// MARK: Emitting and reading SourceFileDepGraph
//==============================================================================

Optional<SourceFileDepGraph> SourceFileDepGraph::loadFromPath(StringRef path)
{
   auto bufferOrError = llvm::MemoryBuffer::getFile(path);
   if (!bufferOrError) {
      return None;
   }
   return loadFromBuffer(*bufferOrError.get());
}

Optional<SourceFileDepGraph>
SourceFileDepGraph::loadFromBuffer(llvm::MemoryBuffer &buffer)
{
   SourceFileDepGraph fg;
   llvm::yaml::Input yamlReader(llvm::MemoryBufferRef(buffer), nullptr);
   yamlReader >> fg;
   if (yamlReader.error()) {
      return None;
   }
   // return fg; compiles for Mac but not Linux, because it cannot be copied.
   return Optional<SourceFileDepGraph>(std::move(fg));
}

//==============================================================================
// MARK: Start of SourceFileDepGraph building, specific to status quo
//==============================================================================

//==============================================================================
// MARK: Helpers for key construction that must be in frontend
//==============================================================================

//template <typename DeclT> static std::string getBaseName(const DeclT *decl)
//{
//   return decl->getBaseName().userFacingName();
//}

//template <typename DeclT> static std::string getName(const DeclT *decl) {
//   return DeclBaseName(decl->getName()).userFacingName();
//}

//static std::string mangleTypeAsContext(const NominalTypeDecl *NTD) {
//   Mangle::ASTMangler Mangler;
//   return !NTD ? "" : Mangler.mangleTypeAsContextUSR(NTD);
//}

//==============================================================================
// MARK: Privacy queries
//==============================================================================

//static bool declIsPrivate(const ValueDecl *VD) {
//   return VD->getFormalAccess() <= AccessLevel::FilePrivate;
//}

///// Return true if \param D cannot affect other files.
//static bool declIsPrivate(const Decl *D) {
//   if (auto *VD = dyn_cast<ValueDecl>(D))
//      return declIsPrivate(VD);
//   switch (D->getKind()) {
//   case DeclKind::Import:
//   case DeclKind::PatternBinding:
//   case DeclKind::EnumCase:
//   case DeclKind::TopLevelCode:
//   case DeclKind::IfConfig:
//   case DeclKind::PoundDiagnostic:
//      return true;

//   case DeclKind::Extension:
//   case DeclKind::InfixOperator:
//   case DeclKind::PrefixOperator:
//   case DeclKind::PostfixOperator:
//      return false;

//   default:
//      llvm_unreachable("everything else is a ValueDecl");
//   }
//}

///// Return true if \ref ED does not contain a member that can affect other
///// files.
//static bool allMembersArePrivate(const ExtensionDecl *ED) {
//   return std::all_of(ED->getMembers().begin(), ED->getMembers().end(),
//                      [](const Decl *d) { return declIsPrivate(d); });
//   //                     declIsPrivate);
//}

/// \ref inheritedType, an inherited protocol, return true if this inheritance
/// cannot affect other files.
//static bool extendedTypeIsPrivate(TypeLoc inheritedType) {
//   auto type = inheritedType.getType();
//   if (!type)
//      return true;

//   if (!type->isExistentialType()) {
//      // Be conservative. We don't know how to deal with other extended types.
//      return false;
//   }

//   auto layout = type->getExistentialLayout();
//   assert(!layout.explicitSuperclass &&
//          "Should not have a subclass existential "
//          "in the inheritance clause of an extension");
//   for (auto protoTy : layout.getProtocols()) {
//      if (!declIsPrivate(protoTy->getDecl()))
//         return false;
//   }

//   return true;
//}

/// Return true if \ref ED does not inherit a protocol that can affect other
/// files. Was called "justMembers" in ReferenceDependencies.cpp
/// \ref ED might be null.
//static bool allInheritedProtocolsArePrivate(const ExtensionDecl *ED) {
//   return std::all_of(ED->getInherited().begin(), ED->getInherited().end(),
//                      extendedTypeIsPrivate);
//}

//==============================================================================
// MARK: SourceFileDeclFinder
//==============================================================================

namespace {
/// Takes all the Decls in a SourceFile, and collects them into buckets by
/// groups of DeclKinds. Also casts them to more specific types
/// TODO: Factor with SourceFileDeclFinder
struct SourceFileDeclFinder
{

public:
   /// Existing system excludes private decls in some cases.
   /// In the future, we might not want to do this, so use bool to decide.
   const bool includePrivateDecls;

   // The extracted Decls:
   ConstPtrVec<ExtensionDecl> extensions;
   ConstPtrVec<OperatorDecl> operators;
   ConstPtrVec<PrecedenceGroupDecl> precedenceGroups;
   ConstPtrVec<NominalTypeDecl> topNominals;
   ConstPtrVec<ValueDecl> topValues;
   ConstPtrVec<NominalTypeDecl> allNominals;
   ConstPtrVec<NominalTypeDecl> potentialMemberHolders;
   ConstPtrVec<FuncDecl> memberOperatorDecls;
   ConstPtrPairVec<NominalTypeDecl, ValueDecl> valuesInExtensions;
   ConstPtrVec<ValueDecl> classMembers;

   /// Construct me and separates the Decls.
   // clang-format off
   SourceFileDeclFinder(const SourceFile *const m_sourceFile, const bool includePrivateDecls)
      : includePrivateDecls(includePrivateDecls)
   {
      //      for (const Decl *const D : m_sourceFile->Decls) {
      //         select<ExtensionDecl, DeclKind::Extension>(D, extensions, false) ||
      //               select<OperatorDecl, DeclKind::InfixOperator, DeclKind::PrefixOperator,
      //               DeclKind::PostfixOperator>(D, operators, false) ||
      //               select<PrecedenceGroupDecl, DeclKind::PrecedenceGroup>(
      //                  D, precedenceGroups, false) ||
      //               select<NominalTypeDecl, DeclKind::Enum, DeclKind::Struct,
      //               DeclKind::Class, DeclKind::Protocol>(D, topNominals, true) ||
      //               select<ValueDecl, DeclKind::TypeAlias, DeclKind::Var, DeclKind::Func,
      //               DeclKind::Accessor>(D, topValues, true);
      //      }
      // clang-format on
      // The order is important because some of these use instance variables
      // computed by others.
      findNominalsFromExtensions();
      findNominalsInTopNominals();
      findValuesInExtensions();
      findClassMembers(m_sourceFile);
   }

private:
   /// Extensions may contain nominals and operators.
   void findNominalsFromExtensions()
   {
      //      for (auto *ED : extensions)
      //         findNominalsAndOperatorsIn(ED->getExtendedNominal(), ED);
   }
   /// Top-level nominals may contain nominals and operators.
   void findNominalsInTopNominals()
   {
      //      for (const auto *const NTD : topNominals)
      //         findNominalsAndOperatorsIn(NTD);
   }
   /// Any nominal may contain nominals and operators.
   /// (indirectly recursive)
   void findNominalsAndOperatorsIn(const NominalTypeDecl *const NTD,
                                   const ExtensionDecl *ED = nullptr)
   {
      //      if (excludeIfPrivate(NTD))
      //         return;
      //      const bool exposedProtocolIsExtended =
      //            ED && !allInheritedProtocolsArePrivate(ED);
      //      if (ED && !includePrivateDecls && !exposedProtocolIsExtended &&
      //          std::all_of(ED->getMembers().begin(), ED->getMembers().end(),
      //                      [&](const Decl *D) { return declIsPrivate(D); })) {
      //         return;
      //      }
      //      if (includePrivateDecls || !ED || exposedProtocolIsExtended)
      //         allNominals.push_back(NTD);
      //      potentialMemberHolders.push_back(NTD);
      //      findNominalsAndOperatorsInMembers(ED ? ED->getMembers()
      //                                           : NTD->getMembers());
   }

   /// Search through the members to find nominals and operators.
   /// (indirectly recursive)
   /// TODO: clean this up, maybe recurse separately for each purpose.
   //   void findNominalsAndOperatorsInMembers(const DeclRange members)
   //   {
   //      for (const Decl *const D : members) {
   //         auto *VD = dyn_cast<ValueDecl>(D);
   //         if (!VD || excludeIfPrivate(VD))
   //            continue;
   //         if (VD->getFullName().isOperator())
   //            memberOperatorDecls.push_back(cast<FuncDecl>(D));
   //         else if (const auto *const NTD = dyn_cast<NominalTypeDecl>(D))
   //            findNominalsAndOperatorsIn(NTD);
   //      }
   //   }

   /// Extensions may contain ValueDecls.
   void findValuesInExtensions() {
      //      for (const auto *ED : extensions) {
      //         if (excludeIfPrivate(ED->getExtendedNominal()))
      //            continue;
      //         if (!includePrivateDecls &&
      //             (!allInheritedProtocolsArePrivate(ED) || allMembersArePrivate(ED)))
      //            continue;
      //         for (const auto *member : ED->getMembers())
      //            if (const auto *VD = dyn_cast<ValueDecl>(member))
      //               if (VD->hasName() && (includePrivateDecls || !declIsPrivate(VD)))
      //                  valuesInExtensions.push_back(
      //                           std::make_pair(ED->getExtendedNominal(), VD));
      //      }
   }

   /// Class members are needed for dynamic lookup dependency nodes.
   void findClassMembers(const SourceFile *const m_sourceFile)
   {
      //      struct Collector : public VisibleDeclConsumer {
      //         ConstPtrVec<ValueDecl> &classMembers;
      //         Collector(ConstPtrVec<ValueDecl> &classMembers)
      //            : classMembers(classMembers) {}
      //         void foundDecl(ValueDecl *VD, DeclVisibilityKind,
      //                        DynamicLookupInfo) override {
      //            classMembers.push_back(VD);
      //         }
      //      } collector{classMembers};
      //      m_sourceFile->lookupClassMembers({}, collector);
   }

   /// Check \p D to see if it is one of the DeclKinds in the template
   /// arguments. If so, cast it to DesiredDeclType and add it to foundDecls.
   /// \returns true if successful.
   template <typename DesiredDeclType, DeclKind firstKind,
             DeclKind... restOfKinds>
   bool select(const Decl *const D, ConstPtrVec<DesiredDeclType> &foundDecls,
               const bool canExcludePrivateDecls)
   {
      //      if (D->getKind() == firstKind) {
      //         auto *dd = cast<DesiredDeclType>(D);
      //         const bool exclude = canExcludePrivateDecls && excludeIfPrivate(dd);
      //         if (!exclude)
      //            foundDecls.push_back(cast<DesiredDeclType>(D));
      //         return true;
      //      }
      //      return select<DesiredDeclType, restOfKinds...>(D, foundDecls,
      //                                                     canExcludePrivateDecls);
   }

   /// Terminate the template recursion.
   template <typename DesiredDeclType>
   bool select(const Decl *const D, ConstPtrVec<DesiredDeclType> &foundDecls,
               bool)
   {
      return false;
   }

   /// Return true if \param D should be excluded on privacy grounds.
   bool excludeIfPrivate(const Decl *const D)
   {
      //      return !includePrivateDecls && declIsPrivate(D);
   }
};
} // namespace

//==============================================================================
// MARK: computeContextForProvidedEntity
//==============================================================================

template <NodeKind kind, typename Entity>
std::string DependencyKey::computeContextForProvidedEntity(Entity)
{
   // Context field is not used for most kinds
   return "";
}

// \ref nominal dependencies are created from a Decl and use the context field.
template <>
std::string DependencyKey::computeContextForProvidedEntity<
NodeKind::nominal, NominalTypeDecl const *>(NominalTypeDecl const *D)
{
   //   return mangleTypeAsContext(D);
}

/// \ref potentialMember dependencies are created from a Decl and use the
/// context field.
template <>
std::string
DependencyKey::computeContextForProvidedEntity<NodeKind::potentialMember,
NominalTypeDecl const *>(
      const NominalTypeDecl *D)
{
   //   return mangleTypeAsContext(D);
}

/// \ref member dependencies are created from a pair and use the context field.
template <>
std::string DependencyKey::computeContextForProvidedEntity<
NodeKind::member, std::pair<const NominalTypeDecl *, const ValueDecl *>>(
      std::pair<const NominalTypeDecl *, const ValueDecl *> holderAndMember)
{
   //   return mangleTypeAsContext(holderAndMember.first);
}

//==============================================================================
// MARK: computeNameForProvidedEntity
//==============================================================================

template <>
std::string
DependencyKey::computeNameForProvidedEntity<NodeKind::sourceFileProvide,
StringRef>(StringRef m_polarphpDeps)
{
   assert(!m_polarphpDeps.empty());
   return m_polarphpDeps;
}

template <>
std::string
DependencyKey::computeNameForProvidedEntity<NodeKind::topLevel,
PrecedenceGroupDecl const *>(
      const PrecedenceGroupDecl *D)
{
   //   return ::get_name(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::topLevel, FuncDecl const *>(const FuncDecl *D)
{
   //   return ::getName(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::topLevel, OperatorDecl const *>(const OperatorDecl *D)
{
   //   return ::getName(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::topLevel, NominalTypeDecl const *>(const NominalTypeDecl *D)
{
   //   return ::getName(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::topLevel, ValueDecl const *>(const ValueDecl *D)
{
   //   return getBaseName(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::dynamicLookup, ValueDecl const *>(const ValueDecl *D)
{
   //   return getBaseName(D);
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::nominal, NominalTypeDecl const *>(const NominalTypeDecl *D)
{
   return "";
}

template <>
std::string
DependencyKey::computeNameForProvidedEntity<NodeKind::potentialMember,
NominalTypeDecl const *>(
      const NominalTypeDecl *D)
{
   return "";
}

template <>
std::string DependencyKey::computeNameForProvidedEntity<
NodeKind::member, std::pair<const NominalTypeDecl *, const ValueDecl *>>(
      std::pair<const NominalTypeDecl *, const ValueDecl *> holderAndMember)
{
   //   return getBaseName(holderAndMember.second);
}

//==============================================================================
// MARK: createDependedUponKey
//==============================================================================

template <>
DependencyKey
DependencyKey::createDependedUponKey<NodeKind::topLevel, DeclBaseName>(
      const DeclBaseName &dbn)
{
   return DependencyKey(NodeKind::topLevel, DeclAspect::interface, "",
                        dbn.userFacingName());
}

template <>
DependencyKey
DependencyKey::createDependedUponKey<NodeKind::dynamicLookup, DeclBaseName>(
      const DeclBaseName &dbn)
{
   return DependencyKey(NodeKind::dynamicLookup, DeclAspect::interface, "",
                        dbn.userFacingName());
}

template <>
DependencyKey DependencyKey::createDependedUponKey<
NodeKind::nominal, std::pair<const NominalTypeDecl *, DeclBaseName>>(
      const std::pair<const NominalTypeDecl *, DeclBaseName> &p)
{
   //   return DependencyKey(NodeKind::nominal, DeclAspect::interface,
   //                        mangleTypeAsContext(p.first), "");
}

template <>
DependencyKey DependencyKey::createDependedUponKey<
NodeKind::member, std::pair<const NominalTypeDecl *, DeclBaseName>>(
      const std::pair<const NominalTypeDecl *, DeclBaseName> &p)
{
   //   const bool isMemberBlank = p.second.empty();
   //   const auto kind =
   //         isMemberBlank ? NodeKind::potentialMember : NodeKind::member;
   //   return DependencyKey(kind, DeclAspect::interface,
   //                        mangleTypeAsContext(p.first),
   //                        isMemberBlank ? "" : p.second.userFacingName());
}

template <>
DependencyKey
DependencyKey::createDependedUponKey<NodeKind::externalDepend, std::string>(
      const std::string &file)
{
   return DependencyKey(NodeKind::externalDepend, DeclAspect::interface, "",
                        file);
}

//==============================================================================
// MARK: SourceFileDepGraphConstructor
//==============================================================================

namespace {

/// Reads the information provided by the frontend and builds the
/// SourceFileDepGraph
class SourceFileDepGraphConstructor
{
   /// The SourceFile containing the Decls.
   SourceFile *m_sourceFile;

   /// Furnishes depended-upon names resulting from lookups.
   const DependencyTracker &m_depTracker;

   /// Name of the m_polarphpDeps file, for inclusion in the constructed graph.
   StringRef m_polarphpDeps; // TODO rm?

   /// To match the existing system, set this to false.
   /// To include even private entities and get intra-file info, set to true.
   const bool m_includePrivateDeps;

   /// If there was an error, cannot get accurate info.
   const bool m_hadCompilationError;

   /// Graph under construction
   SourceFileDepGraph m_graph;

public:
   SourceFileDepGraphConstructor(SourceFile *sourceFile,
                                 const DependencyTracker &depTracker,
                                 StringRef polarphpDeps,
                                 const bool includePrivateDeps,
                                 const bool hadCompilationError)
      : m_sourceFile(sourceFile), m_depTracker(depTracker), m_polarphpDeps(polarphpDeps),
        m_includePrivateDeps(includePrivateDeps),
        m_hadCompilationError(hadCompilationError)
   {}

   /// Construct the graph and return it.
   SourceFileDepGraph construct()
   {
      // Order matters here, each function adds state used by the next one.
      addSourceFileNodesToGraph();
      if (!m_hadCompilationError) {
         addProviderNodesToGraph();
         addDependencyArcsToGraph();
      }
      assert(m_graph.verify());
      return std::move(m_graph);
   }

private:
   std::string getSourceFileFingerprint() const
   {
      return getInterfaceHash(m_sourceFile);
   }

   static std::string getInterfaceHash(SourceFile *sourceFile)
   {
      llvm::SmallString<32> interfaceHash;
      sourceFile->getInterfaceHash(interfaceHash);
      return interfaceHash.str().str();
   }

   /// Also sets sourceFileNodes
   void addSourceFileNodesToGraph();
   /// Uses sourceFileNodes
   void addProviderNodesToGraph();
   /// Uses provides nodes for intra-graph dependences
   void addDependencyArcsToGraph();

   /// Given an array of Decls or pairs of them in \p declsOrPairs
   /// create nodes if needed and add the new nodes to the graph.
   template <NodeKind kind, typename ContentsT>
   void addAllProviderNodesOfAGivenType(std::vector<ContentsT> &contentsVec)
   {
      for (const auto declOrPair : contentsVec) {
         // No fingerprints for providers (Decls) yet.
         // Someday ...
         const Optional<std::string> fingerprint = None;
         auto p = m_graph.findExistingNodePairOrCreateAndAddIfNew(
                  kind,
                  DependencyKey::computeContextForProvidedEntity<kind>(declOrPair),
                  DependencyKey::computeNameForProvidedEntity<kind>(declOrPair),
                  fingerprint);
         // Since we don't have fingerprints yet, must rebuild every provider when
         // interfaceHash changes. So when interface (i.e. interface hash) of
         // m_sourceFile changes, every provides is dirty. And since we don't know
         // what happened, dirtyness might affect the interface.
         if (!p.getInterface()->getFingerprint().hasValue()) {
            m_graph.addArc(m_graph.getSourceFileNodePair().getInterface(), p.getInterface());
         }
      }
   }

   /// Given a map of names and isCascades, add the resulting dependencies to the
   /// graph.
   template <NodeKind kind>
   void addAllDependenciesFrom(const llvm::DenseMap<DeclBaseName, bool> &map)
   {
      for (const auto &p : map) {
         recordThatThisWholeFileDependsOn(
                  DependencyKey::createDependedUponKey<kind>(p.first), p.second);
      }
   }

   /// Given a map of holder-and-member-names and isCascades, add the resulting
   /// dependencies to the graph.
   void addAllDependenciesFrom(
         const llvm::DenseMap<std::pair<const NominalTypeDecl *, DeclBaseName>,
         bool> &);

   /// Given an array of external m_polarphpDeps files, add the resulting external
   /// dependencies to the graph.
   void addAllDependenciesFrom(ArrayRef<std::string> externals)
   {
      for (const auto &s : externals) {
         recordThatThisWholeFileDependsOn(
                  DependencyKey::createDependedUponKey<NodeKind::externalDepend>(s),
                  true);
      }
   }

   /// In the status quo, we don't get to know which provided entities are
   /// affected by a particular dependency; we only get to know that the whole
   /// file must be recompiled if said def changes. However if \p cascades is
   /// true, then every other file that depends upon something provided here must
   /// be recompiled, too.
   void recordThatThisWholeFileDependsOn(const DependencyKey &, bool cascades);
};
} // namespace

using UsedMembersMap =
llvm::DenseMap<std::pair<const NominalTypeDecl *, DeclBaseName>, bool>;
void SourceFileDepGraphConstructor::addAllDependenciesFrom(
      const UsedMembersMap &map)
{

   //   UsedMembersMap filteredMap;
   //   for (const auto &entry : map)
   //      if (m_includePrivateDeps || !declIsPrivate(entry.first.first))
   //         filteredMap[entry.getFirst()] = entry.getSecond();

   //   std::unordered_set<const NominalTypeDecl *> holdersOfCascadingMembers;
   //   for (auto &entry : filteredMap)
   //      if (entry.second)
   //         holdersOfCascadingMembers.insert(entry.first.first);

   //   for (auto &entry : filteredMap) {
   //      // mangles twice in the name of symmetry
   //      recordThatThisWholeFileDependsOn(
   //               DependencyKey::createDependedUponKey<NodeKind::nominal>(entry.first),
   //               holdersOfCascadingMembers.count(entry.first.first) != 0);
   //      recordThatThisWholeFileDependsOn(
   //               DependencyKey::createDependedUponKey<NodeKind::member>(entry.first),
   //               entry.second);
   //   }
}

//==============================================================================
// MARK: SourceFileDepGraphConstructor: Adding nodes to the graph
//==============================================================================

void SourceFileDepGraphConstructor::addSourceFileNodesToGraph()
{
   m_graph.findExistingNodePairOrCreateAndAddIfNew(
            NodeKind::sourceFileProvide,
            DependencyKey::computeContextForProvidedEntity<
            NodeKind::sourceFileProvide>(m_polarphpDeps),
            DependencyKey::computeNameForProvidedEntity<NodeKind::sourceFileProvide>(
               m_polarphpDeps),
            getSourceFileFingerprint());
}

void SourceFileDepGraphConstructor::addProviderNodesToGraph()
{
   SourceFileDeclFinder declFinder(m_sourceFile, m_includePrivateDeps);
   // TODO: express the multiple provides and depends streams with variadic
   // templates

   // Many kinds of Decls become top-level depends.
   addAllProviderNodesOfAGivenType<NodeKind::topLevel>(
            declFinder.precedenceGroups);
   addAllProviderNodesOfAGivenType<NodeKind::topLevel>(
            declFinder.memberOperatorDecls);
   addAllProviderNodesOfAGivenType<NodeKind::topLevel>(declFinder.operators);
   addAllProviderNodesOfAGivenType<NodeKind::topLevel>(declFinder.topNominals);
   addAllProviderNodesOfAGivenType<NodeKind::topLevel>(declFinder.topValues);

   addAllProviderNodesOfAGivenType<NodeKind::nominal>(declFinder.allNominals);

   addAllProviderNodesOfAGivenType<NodeKind::potentialMember>(
            declFinder.potentialMemberHolders);
   addAllProviderNodesOfAGivenType<NodeKind::member>(
            declFinder.valuesInExtensions);

   addAllProviderNodesOfAGivenType<NodeKind::dynamicLookup>(
            declFinder.classMembers);
}

void SourceFileDepGraphConstructor::addDependencyArcsToGraph()
{
   // TODO: express the multiple provides and depends streams with variadic
   // templates
   //   addAllDependenciesFrom<NodeKind::topLevel>(
   //            m_sourceFile->getReferencedNameTracker()->getTopLevelNames());
   //   addAllDependenciesFrom(m_sourceFile->getReferencedNameTracker()->getUsedMembers());
   //   addAllDependenciesFrom<NodeKind::dynamicLookup>(
   //            m_sourceFile->getReferencedNameTracker()->getDynamicLookupNames());
   //   addAllDependenciesFrom(m_depTracker.getDependencies());
}

void SourceFileDepGraphConstructor::recordThatThisWholeFileDependsOn(
      const DependencyKey &key, bool cascades)
{
   SourceFileDepGraphNode *def =
         m_graph.findExistingNodeOrCreateIfNew(key, None, false /* = !isProvides */);
   m_graph.addArc(def, m_graph.getSourceFileNodePair().useDependingOnCascading(cascades));
}

//==============================================================================
// Entry point from the Frontend to this whole system
//==============================================================================

bool emit_reference_dependencies (
      DiagnosticEngine &diags, SourceFile *const sourceFile,
      const DependencyTracker &depTracker, StringRef outputPath)
{
   return false;
   //   // Before writing to the dependencies file path, preserve any previous file
   //   // that may have been there. No error handling -- this is just a nicety, it
   //   // doesn't matter if it fails.
   //   llvm::sys::fs::rename(outputPath, outputPath + "~");
   //   const bool includeIntrafileDeps =
   //         sourceFile->getAstContext().LangOpts.ExperimentalDependenciesIncludeIntrafileOnes;
   //   const bool m_hadCompilationError = sourceFile->getAstContext().hadError();
   //   SourceFileDepGraphConstructor gc(sourceFile, depTracker, outputPath,
   //                                    includeIntrafileDeps, m_hadCompilationError);
   //   SourceFileDepGraph m_graph = gc.construct();

   //   const bool hadError =
   //         withOutputFile(diags, outputPath, [&](llvm::raw_pwrite_stream &out) {
   //         out << m_graph.yamlProlog(m_hadCompilationError);
   //         llvm::yaml::Output yamlWriter(out);
   //         yamlWriter << m_graph;
   //         return false;
   //});

//   assert(m_graph.verifyReadsWhatIsWritten(outputPath));

//   std::string dotFileName = outputPath.str() + ".dot";
//   withOutputFile(diags, dotFileName, [&](llvm::raw_pwrite_stream &out) {
//      DotFileEmitter<SourceFileDepGraph>(out, m_graph, false, false).emit();
//      return false;
//   });
//   return hadError;
}

} // experimentaldependencies
} // polar::ast
