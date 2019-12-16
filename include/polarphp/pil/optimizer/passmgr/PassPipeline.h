//===--- PassPipeline.h -----------------------------------------*- C++ -*-===//
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
/// This file defines the PILPassPipelinePlan and PILPassPipeline
/// classes. These are higher level representations of sequences of PILPasses
/// and the run behavior of these sequences (i.e. run one, until fixed point,
/// etc). This makes it easy to serialize and deserialize pipelines without work
/// on the part of the user. Eventually this will be paired with a gyb based
/// representation of Passes.def that will able to be used to generate a python
/// based script builder.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSPIPELINE_H
#define POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSPIPELINE_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/optimizer/passmgr/PassPipeline.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include <vector>

namespace polar {

class PILPassPipelinePlan;
struct PILPassPipeline;

enum class PassPipelineKind {
#define PASSPIPELINE(NAME, DESCRIPTION) NAME,
#include "polarphp/pil/optimizer/passmgr/PassPipelineDef.h"
};

class PILPassPipelinePlan final {
  const PILOptions &Options;
  std::vector<PassKind> Kinds;
  std::vector<PILPassPipeline> PipelineStages;

public:
  PILPassPipelinePlan(const PILOptions &Options)
      : Options(Options), Kinds(), PipelineStages() {}

  ~PILPassPipelinePlan() = default;
  PILPassPipelinePlan(const PILPassPipelinePlan &) = default;

  const PILOptions &getOptions() const { return Options; }

// Each pass gets its own add-function.
#define PASS(ID, TAG, NAME)                                                    \
  void add##ID() {                                                             \
    assert(!PipelineStages.empty() && "startPipeline before adding passes.");  \
    Kinds.push_back(PassKind::ID);                                             \
  }
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"

  void addPasses(ArrayRef<PassKind> PassKinds);

#define PASSPIPELINE(NAME, DESCRIPTION)                                        \
  static PILPassPipelinePlan get##NAME##PassPipeline(const PILOptions &Options);
#include "polarphp/pil/optimizer/passmgr/PassPipelineDef.h"

  static PILPassPipelinePlan getPassPipelineForKinds(const PILOptions &Options,
                                                     ArrayRef<PassKind> Kinds);
  static PILPassPipelinePlan getPassPipelineFromFile(const PILOptions &Options,
                                                     StringRef Filename);

  /// Our general format is as follows:
  ///
  ///   [
  ///     [
  ///       "PASS_MANAGER_ID",
  ///       "one_iteration"|"until_fix_point",
  ///       "PASS1", "PASS2", ...
  ///     ],
  ///   ...
  ///   ]
  void dump();

  void print(llvm::raw_ostream &os);

  void startPipeline(StringRef Name = "");
  using PipelineKindIterator = decltype(Kinds)::const_iterator;
  using PipelineKindRange = iterator_range<PipelineKindIterator>;
  iterator_range<PipelineKindIterator>
  getPipelinePasses(const PILPassPipeline &P) const;

  using PipelineIterator = decltype(PipelineStages)::const_iterator;
  using PipelineRange = iterator_range<PipelineIterator>;
  PipelineRange getPipelines() const {
    return {PipelineStages.begin(), PipelineStages.end()};
  }
};

struct PILPassPipeline final {
  unsigned ID;
  StringRef Name;
  unsigned KindOffset;
};

inline void PILPassPipelinePlan::startPipeline(StringRef Name) {
  PipelineStages.push_back(PILPassPipeline{
      unsigned(PipelineStages.size()), Name, unsigned(Kinds.size())});
}

inline PILPassPipelinePlan::PipelineKindRange
PILPassPipelinePlan::getPipelinePasses(const PILPassPipeline &P) const {
  unsigned ID = P.ID;
  assert(PipelineStages.size() > ID && "Pipeline with ID greater than the "
                                       "size of its container?!");

  // In this case, we are the last pipeline. Return end and the kind offset.
  if ((PipelineStages.size() - 1) == ID) {
    return {std::next(Kinds.begin(), P.KindOffset), Kinds.end()};
  }

  // Otherwise, end is the beginning of the next PipelineStage.
  return {std::next(Kinds.begin(), P.KindOffset),
          std::next(Kinds.begin(), PipelineStages[ID + 1].KindOffset)};
}

} // end namespace polar

#endif
