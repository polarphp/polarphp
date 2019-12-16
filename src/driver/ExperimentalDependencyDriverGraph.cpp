////===-- ExperimentalDependencyGraph.cpp ------------------------------------==//
////
//// This source file is part of the Swift.org open source project
////
//// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
//// Licensed under Apache License v2.0 with Runtime Library Exception
////
//// See https://swift.org/LICENSE.txt for license information
//// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
////
////===----------------------------------------------------------------------===//
//// This source file is part of the polarphp.org open source project
////
//// Copyright (c) 2017 - 2019 polarphp software foundation
//// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
//// Licensed under Apache License v2.0 with Runtime Library Exception
////
//// See https://polarphp.org/LICENSE.txt for license information
//// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
////
//// Created by polarboy on 2019/12/03.

//#include "polarphp/driver/ExperimentalDependencyDriverGraph.h"
//// Next two includes needed for reporting errors opening dot file for writing.
//#include "polarphp/ast/DiagnosticsFrontend.h"
//#include "polarphp/ast/Filesystem.h"
//#include "polarphp/basic/ReferenceDependencyKeys.h"
//#include "polarphp/basic/Statistic.h"
//#include "polarphp/demangling/Demangle.h"
//#include "polarphp/driver/Job.h"
//#include "llvm/ADT/STLExtras.h"
//#include "llvm/ADT/SmallString.h"
//#include "llvm/ADT/SmallVector.h"
//#include "llvm/ADT/StringSet.h"
//#include "llvm/ADT/StringSwitch.h"
//#include "llvm/Support/MemoryBuffer.h"
//#include "llvm/Support/Path.h"
//#include "llvm/Support/SourceMgr.h"
//#include "llvm/Support/YAMLParser.h"
//#include "llvm/Support/raw_ostream.h"

//// Definitions for the portion experimental dependency system used by the
//// driver.

//namespace polar::driver {
//namespace experimentaldependencies {

//using polar::FrontendStatsTracer;

////==============================================================================
//// MARK: Interfacing to Compilation
////==============================================================================

//using LoadResult = DependencyGraphImpl::LoadResult;

//LoadResult ModuleDepGraph::loadFromPath(const Job *Cmd, StringRef path,
//                                        DiagnosticEngine &diags)
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-loadFromPath");

//   if (driverDotFileBasePath.empty()) {
//      driverDotFileBasePath = path;
//      llvm::sys::path::remove_filename(driverDotFileBasePath);
//      llvm::sys::path::append(driverDotFileBasePath, "driver");
//   }

//   auto buffer = llvm::MemoryBuffer::getFile(path);
//   if (!buffer) {
//      return LoadResult::HadError;
//   }
//   auto r = loadFromBuffer(Cmd, *buffer.get());
//   if (m_emitExperimentalDependencyDotFileAfterEveryImport) {
//      emitDotFileForJob(diags, Cmd);
//   }
//   if (m_verifyExperimentalDependencyGraphAfterEveryImport) {
//      verify();
//   }
//   return r;
//}

//LoadResult ModuleDepGraph::loadFromBuffer(const Job *job,
//                                          llvm::MemoryBuffer &buffer)
//{

//   Optional<SourceFileDepGraph> sourceFileDepGraph =
//         SourceFileDepGraph::loadFromBuffer(buffer);
//   if (!sourceFileDepGraph) {
//      return DependencyGraphImpl::LoadResult::HadError;
//   }
//   addIndependentNode(job);
//   return integrate(sourceFileDepGraph.getValue());
//}

//bool ModuleDepGraph::isMarked(const Job *cmd) const
//{
//   return m_cascadingJobs.count(getPolarphpDeps(cmd));
//}

//void ModuleDepGraph::markTransitive(
//      SmallVectorImpl<const Job *> &consequentJobsToRecompile,
//      const Job *jobToBeRecompiled, const void *ignored)
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-markTransitive");
//   std::unordered_set<const ModuleDepGraphNode *> dependentNodes;
//   const StringRef swiftDepsToBeRecompiled = getPolarphpDeps(jobToBeRecompiled);
//   // Do the traversal.
//   for (auto &fileAndNode : m_nodeMap[swiftDepsToBeRecompiled]) {
//      assert(isCurrentPathForTracingEmpty());
//      findDependentNodesAndRecordCascadingOnes(dependentNodes,
//                                               fileAndNode.second);
//   }
//   computeUniqueJobsFromNodes(consequentJobsToRecompile, dependentNodes);
//}

//void ModuleDepGraph::computeUniqueJobsFromNodes(
//      SmallVectorImpl<const Job *> &jobs,
//      const std::unordered_set<const ModuleDepGraphNode *> &nodes)
//{
//   std::unordered_set<std::string> polarphpDepsOfNodes;
//   for (const ModuleDepGraphNode *n : nodes) {
//      if (!n->getPolarphpDeps().hasValue()) {
//         continue;
//      }
//      const std::string &polarphpDeps = n->getPolarphpDeps().getValue();
//      if (polarphpDepsOfNodes.insert(polarphpDeps).second) {
//         assert(n->assertImplementationMustBeInAFile());
//         ensureJobIsTracked(polarphpDeps);
//         jobs.push_back(getJob(polarphpDeps));
//      }
//   }
//}

//bool ModuleDepGraph::markIntransitive(const Job *node)
//{
//   return rememberThatJobCascades(getPolarphpDeps(node));
//}

//void ModuleDepGraph::addIndependentNode(const Job *job)
//{
//   // No need to create any nodes; that will happen when the swiftdeps file is
//   // read. Just record the correspondence.
//   m_jobsByPolarphpDeps.insert(std::make_pair(getPolarphpDeps(job), job));
//}

//std::vector<std::string> ModuleDepGraph::getExternalDependencies() const
//{
//   return std::vector<std::string>(m_externalDependencies.begin(),
//                                   m_externalDependencies.end());
//}

//// Add every (swiftdeps) use of the external dependency to uses.
//void ModuleDepGraph::markExternal(SmallVectorImpl<const Job *> &uses,
//                                  StringRef externalDependency)
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-markExternal");
//   // TODO move nameForDep into key
//   // These nodes will depend on the *interface* of the external Decl.
//   DependencyKey key =
//         DependencyKey::createDependedUponKey<NodeKind::externalDepend>(
//            externalDependency.str());
//   // collect answers into useSet
//   std::unordered_set<std::string> visitedSet;
//   for (const ModuleDepGraphNode *useNode : m_usesByDef[key]) {
//      const Job *job = getJob(useNode->getPolarphpDeps());
//      if (!isMarked(job)) {
//         continue;
//      }
//      uses.push_back(job);
//      markTransitive(uses, job);
//   }
//}

////==============================================================================
//// MARK: Integrating SourceFileDepGraph into ModuleDepGraph
////==============================================================================

//LoadResult ModuleDepGraph::integrate(const SourceFileDepGraph &g)
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-integrate");
//   StringRef polarphpDeps = g.getPolarphpDepsFromSourceFileProvide();
//   // When done, disappearedNodes contains the nodes which no longer exist.
//   auto disappearedNodes = m_nodeMap[polarphpDeps];
//   // When done, changeDependencyKeys contains a list of keys that changed
//   // as a result of this integration.
//   auto changedNodes = std::unordered_set<DependencyKey>();
//   g.forEachNode([&](const SourceFileDepGraphNode *integrand) {
//      const auto &key = integrand->getKey();
//      auto preexistingMatch = findPreexistingMatch(polarphpDeps, integrand);
//      if (preexistingMatch.hasValue() &&
//          preexistingMatch.getValue().first == LocationOfPreexistingNode::here)
//         disappearedNodes.erase(key); // Node was and still is. Do not erase it.
//      const bool changed =
//            integrateSourceFileDepGraphNode(g, integrand, preexistingMatch);
//      if (changed) {
//         changedNodes.insert(key);
//      }
//   });

//   for (auto &p : disappearedNodes) {
//      changedNodes.insert(p.second->getKey());
//      removeNode(p.second);
//   }

//   // TODO: use changedKeys sometime, for instance by returning them
//   // as part of return value so that the driver can only mark from them.
//   return changedNodes.empty() ? LoadResult::UpToDate
//                               : LoadResult::AffectsDownstream;
//}

//ModuleDepGraph::PreexistingNodeIfAny ModuleDepGraph::findPreexistingMatch(
//      StringRef swiftDepsOfCompilationToBeIntegrated,
//      const SourceFileDepGraphNode *integrand)
//{
//   const auto &matches = m_nodeMap[integrand->getKey()];
//   const auto &expatsIter = matches.find("");
//   if (expatsIter != matches.end()) {
//      assert(matches.size() == 1 &&
//             "If an expat exists, then must not be any matches in other files");
//      return std::make_pair(LocationOfPreexistingNode::nowhere,
//                            expatsIter->second);
//   }
//   if (integrand->getIsProvides()) {
//      const auto &preexistingNodeInPlaceIter =
//            matches.find(swiftDepsOfCompilationToBeIntegrated);
//      if (preexistingNodeInPlaceIter != matches.end()) {
//         return std::make_pair(LocationOfPreexistingNode::here,
//                               preexistingNodeInPlaceIter->second);
//      }
//   }
//   if (!matches.empty()) {
//      return std::make_pair(LocationOfPreexistingNode::elsewhere,
//                            matches.begin()->second);
//   }
//   return None;
//}

//bool ModuleDepGraph::integrateSourceFileDepGraphNode(
//      const SourceFileDepGraph &g, const SourceFileDepGraphNode *integrand,
//      const PreexistingNodeIfAny preexistingMatch)
//{

//   // Track externalDependencies so Compilation can check them.
//   if (integrand->getKey().getKind() == NodeKind::externalDepend) {
//      return m_externalDependencies.insert(integrand->getKey().getName()).second;
//   }
//   if (integrand->isDepends())
//      return false; // dependency will be handled by the use node

//   StringRef polarphpDepsOfSourceFileGraph = g.getPolarphpDepsFromSourceFileProvide();
//   auto changedAndUseNode = integrateSourceFileDeclNode(
//            integrand, polarphpDepsOfSourceFileGraph, preexistingMatch);
//   recordWhatUseDependsUpon(g, integrand, changedAndUseNode.second);
//   return changedAndUseNode.first;
//}

//std::pair<bool, ModuleDepGraphNode *>
//ModuleDepGraph::integrateSourceFileDeclNode(
//      const SourceFileDepGraphNode *integrand,
//      StringRef polarphpDepsOfSourceFileGraph,
//      const PreexistingNodeIfAny preexistingMatch)
//{

//   if (!preexistingMatch.hasValue()) {
//      auto *newNode = integrateByCreatingANewNode(
//               integrand, polarphpDepsOfSourceFileGraph.str());
//      return std::make_pair(true, newNode); // New node
//   }
//   const auto where = preexistingMatch.getValue().first;
//   auto *match = preexistingMatch.getValue().second;
//   switch (where) {
//   case LocationOfPreexistingNode::here:
//      return std::make_pair(match->integrateFingerprintFrom(integrand), match);

//   case LocationOfPreexistingNode::nowhere:
//      // Some other file depended on this, but didn't know where it was.
//      moveNodeToDifferentFile(match, polarphpDepsOfSourceFileGraph.str());
//      match->integrateFingerprintFrom(integrand);
//      return std::make_pair(true, match); // New Decl, assume changed

//   case LocationOfPreexistingNode::elsewhere:
//      auto *newNode = integrateByCreatingANewNode(
//               integrand, polarphpDepsOfSourceFileGraph.str());
//      return std::make_pair(true, newNode); // New node;
//   }
//   llvm_unreachable("impossible");
//}

//ModuleDepGraphNode *ModuleDepGraph::integrateByCreatingANewNode(
//      const SourceFileDepGraphNode *integrand,
//      const Optional<std::string> swiftDepsForNewNode)
//{
//   const auto &key = integrand->getKey();
//   ModuleDepGraphNode *newNode = new ModuleDepGraphNode(
//            key, integrand->getFingerprint(), swiftDepsForNewNode);
//   addToMap(newNode);
//   return newNode;
//}

//void ModuleDepGraph::recordWhatUseDependsUpon(
//      const SourceFileDepGraph &g,
//      const SourceFileDepGraphNode *sourceFileUseNode,
//      ModuleDepGraphNode *moduleUseNode)
//{
//   g.forEachDefDependedUponBy(sourceFileUseNode,
//                              [&](const SourceFileDepGraphNode *def) {
//      m_usesByDef[def->getKey()].insert(moduleUseNode);
//   });
//}

//void ModuleDepGraph::removeNode(ModuleDepGraphNode *n)
//{
//   eraseNodeFromMap(n);
//   delete n;
//}

////==============================================================================
//// MARK: ModuleDepGraph access
////==============================================================================

//void ModuleDepGraph::forEachUseOf(
//      const ModuleDepGraphNode *def,
//      function_ref<void(const ModuleDepGraphNode *)> fn)
//{
//   auto iter = m_usesByDef.find(def->getKey());
//   if (iter == m_usesByDef.end()) {
//      return;
//   }
//   for (const ModuleDepGraphNode *useNode : iter->second) {
//      fn(useNode);
//   }
//}

//void ModuleDepGraph::forEachNode(
//      function_ref<void(const ModuleDepGraphNode *)> fn) const
//{
//   m_nodeMap.forEachEntry([&](const std::string &, const DependencyKey &,
//                          ModuleDepGraphNode *n) { fn(n); });
//}

//void ModuleDepGraph::forEachMatchingNode(
//      const DependencyKey &key,
//      function_ref<void(const ModuleDepGraphNode *)> fn) const
//{
//   m_nodeMap.forEachValueMatching(
//            key, [&](const std::string &, ModuleDepGraphNode *n) { fn(n); });
//}

//void ModuleDepGraph::forEachArc(
//      function_ref<void(const ModuleDepGraphNode *, const ModuleDepGraphNode *)>
//      fn) const
//{
//   /// Use find instead of [] because this is const
//   for (const auto &defUse : m_usesByDef) {
//      forEachMatchingNode(defUse.first, [&](const ModuleDepGraphNode *defNode) {
//         for (const auto &useNode : defUse.second) {
//            fn(defNode, useNode);
//         }
//      });
//   }
//}

////==============================================================================
//// MARK: ModuleDepGraph traversal
////==============================================================================

//// Could be faster by passing in a file, not a node, but we are trying for
//// generality.

//void ModuleDepGraph::findDependentNodesAndRecordCascadingOnes(
//      std::unordered_set<const ModuleDepGraphNode *> &foundDependents,
//      const ModuleDepGraphNode *definition)
//{

//   size_t pathLengthAfterArrival = traceArrival(definition);

//   // Moved this out of the following loop for effieciency.
//   assert(definition->getPolarphpDeps().hasValue() &&
//          "Should only call me for Decl nodes.");

//   forEachUseOf(definition, [&](const ModuleDepGraphNode *u) {
//      // Cycle recording and check.
//      if (!foundDependents.insert(u).second) {
//         return;
//      }
//      if (u->getKey().isInterface() && u->getPolarphpDeps().hasValue()) {
//         // An interface depends on something. Thus, if that something changes
//         // the interface must be recompiled. But if an interface changes, then
//         // anything using that interface must also be recompiled.
//         // So, the job containing the interface "cascades", in other words
//         // whenever that job gets recompiled, anything depending on it
//         // (since we don't have interface-specific dependency info as of Dec.
//         // 2018) must be recompiled.
//         rememberThatJobCascades(u->getPolarphpDeps().getValue());
//         findDependentNodesAndRecordCascadingOnes(foundDependents, u);
//      }
//   });
//   traceDeparture(pathLengthAfterArrival);
//}

//size_t ModuleDepGraph::traceArrival(const ModuleDepGraphNode *visitedNode)
//{
//   if (!m_currentPathIfTracing.hasValue()) {
//      return 0;
//   }
//   auto &currentPath = m_currentPathIfTracing.getValue();
//   recordDependencyPathToJob(currentPath, getJob(visitedNode->getPolarphpDeps()));
//   currentPath.push_back(visitedNode);
//   return currentPath.size();
//}

//void ModuleDepGraph::recordDependencyPathToJob(
//      const std::vector<const ModuleDepGraphNode *> &pathToJob,
//      const driver::Job *dependentJob)
//{
//   m_dependencyPathsToJobs.insert(std::make_pair(dependentJob, pathToJob));
//}

//void ModuleDepGraph::traceDeparture(size_t pathLengthAfterArrival)
//{
//   if (!m_currentPathIfTracing) {
//      return;
//   }
//   auto &currentPath = m_currentPathIfTracing.getValue();
//   assert(pathLengthAfterArrival == currentPath.size() &&
//          "Path must be maintained throughout recursive visits.");
//   currentPath.pop_back();
//}

//// Emitting Dot file for ModuleDepGraph
//// ===========================================

//void ModuleDepGraph::emitDotFileForJob(DiagnosticEngine &diags,
//                                       const Job *job)
//{
//   emitDotFile(diags, getPolarphpDeps(job));
//}

//void ModuleDepGraph::emitDotFile(DiagnosticEngine &diags, StringRef baseName)
//{
//   unsigned seqNo = m_dotFileSequenceNumber[baseName]++;
//   std::string fullName = baseName.str() + "." + std::to_string(seqNo) + ".dot";
//   withOutputFile(diags, fullName, [&](llvm::raw_ostream &out) {
//      emitDotFile(out);
//      return false;
//   });
//}

//void ModuleDepGraph::emitDotFile(llvm::raw_ostream &out)
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-emitDotFile");
//   DotFileEmitter<ModuleDepGraph>(out, *this, true, false).emit();
//}

////==============================================================================
//// MARK: ModuleDepGraph debugging
////==============================================================================

//void ModuleDepGraphNode::dump() const
//{
//   DepGraphNode::dump();
//   if (getPolarphpDeps().hasValue()) {
//      llvm::errs() << " polarphpDeps: <" << getPolarphpDeps().getValue() << ">\n";
//   } else {
//      llvm::errs() << " no polarphpDeps\n";
//   }
//}

//bool ModuleDepGraph::verify() const
//{
//   FrontendStatsTracer tracer(m_stats, "experimental-dependencies-verify");
//   verifyNodeMapEntries();
//   verifyCanFindEachJob();
//   verifyEachJobInGraphIsTracked();
//   return true;
//}

//void ModuleDepGraph::verifyNodeMapEntries() const
//{
//   FrontendStatsTracer tracer(m_stats,
//                              "experimental-dependencies-verifyNodeMapEntries");
//   // TODO: disable when not debugging
//   std::array<
//         std::unordered_map<DependencyKey,
//         std::unordered_map<std::string, ModuleDepGraphNode *>>,
//         2>
//         nodesSeenInNodeMap;
//   m_nodeMap.verify([&](const std::string &polarphpDepsString,
//                    const DependencyKey &key, ModuleDepGraphNode *n,
//                    unsigned submapIndex) {
//      verifyNodeMapEntry(nodesSeenInNodeMap, polarphpDepsString, key, n,
//                         submapIndex);
//   });
//}

//void ModuleDepGraph::verifyNodeMapEntry(
//      std::array<std::unordered_map<
//      DependencyKey,
//      std::unordered_map<std::string, ModuleDepGraphNode *>>,
//      2> &nodesSeenInNodeMap,
//      const std::string &polarphpDepsString, const DependencyKey &key,
//      ModuleDepGraphNode *n, const unsigned submapIndex) const
//{
//   verifyNodeIsUniqueWithinSubgraph(nodesSeenInNodeMap, polarphpDepsString, key, n,
//                                    submapIndex);
//   verifyNodeIsInRightEntryInNodeMap(polarphpDepsString, key, n);
//   key.verify();
//   verifyExternalDependencyUniqueness(key);
//}

//void ModuleDepGraph::verifyNodeIsUniqueWithinSubgraph(
//      std::array<std::unordered_map<
//      DependencyKey,
//      std::unordered_map<std::string, ModuleDepGraphNode *>>,
//      2> &nodesSeenInNodeMap,
//      const std::string &polarphpDepsString, const DependencyKey &key,
//      ModuleDepGraphNode *const n, const unsigned submapIndex) const
//{
//   assert(submapIndex < nodesSeenInNodeMap.size() &&
//          "submapIndex is out of bounds.");
//   auto iterInserted = nodesSeenInNodeMap[submapIndex][n->getKey()].insert(
//            std::make_pair(n->getPolarphpDeps().hasValue() ? n->getPolarphpDeps().getValue()
//                                                           : std::string(),
//                           n));
//   if (!iterInserted.second) {
//      llvm_unreachable("duplicate driver keys");
//   }
//}

//void ModuleDepGraph::verifyNodeIsInRightEntryInNodeMap(
//      const std::string &polarphpDepsString, const DependencyKey &key,
//      const ModuleDepGraphNode *const n) const
//{
//   const DependencyKey &nodeKey = n->getKey();
//   const Optional<std::string> polarphpDeps =
//         polarphpDepsString.empty() ? None : Optional<std::string>(polarphpDepsString);
//   assert(n->getPolarphpDeps() == polarphpDeps ||
//          mapCorruption("Node misplaced for polarphpDeps"));
//   assert(nodeKey == key || mapCorruption("Node misplaced for key"));
//}

//void ModuleDepGraph::verifyExternalDependencyUniqueness(
//      const DependencyKey &key) const
//{
//   assert((key.getKind() != NodeKind::externalDepend ||
//         m_externalDependencies.count(key.getName()) == 1) &&
//          "Ensure each external dependency is tracked exactly once");
//}

//void ModuleDepGraph::verifyCanFindEachJob() const
//{
//   FrontendStatsTracer tracer(m_stats,
//                              "experimental-dependencies-verifyCanFindEachJob");
//   for (const auto &p : m_jobsByPolarphpDeps) {
//      getJob(p.first);
//   }
//}

//void ModuleDepGraph::verifyEachJobInGraphIsTracked() const
//{
//   FrontendStatsTracer tracer(
//            m_stats, "experimental-dependencies-verifyEachJobIsTracked");
//   m_nodeMap.forEachKey1(
//            [&](const std::string &polarphpDeps, const typename NodeMap::Key2Map &) {
//      ensureJobIsTracked(polarphpDeps);
//   });
//}

//bool ModuleDepGraph::emitDotFileAndVerify(DiagnosticEngine &diags)
//{
//   if (!driverDotFileBasePath.empty()) {
//      emitDotFile(diags, driverDotFileBasePath);
//   }
//   return verify();
//}

///// Dump the path that led to \p node.
///// TODO: make output more like existing system's
//void ModuleDepGraph::printPath(raw_ostream &out,
//                               const driver::Job *jobToBeBuilt) const
//{
//   assert(m_currentPathIfTracing.hasValue() &&
//          "Cannot print paths of paths weren't tracked.");
//   auto const allPaths = m_dependencyPathsToJobs.find(jobToBeBuilt);
//   if (allPaths == m_dependencyPathsToJobs.cend()) {
//      return;
//   }
//   for (const auto *n : allPaths->second) {
//      out << n->humanReadableName() << "\n";
//   }
//   out << "\n";
//}

//} // experimentaldependencies
//} // polar::driver
