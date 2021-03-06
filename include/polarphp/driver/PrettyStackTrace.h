//===--- PrettyStackTrace.h - PrettyStackTrace for the Driver ---*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_DRIVER_PRETTYSTACKTRACE_H
#define POLARPHP_DRIVER_PRETTYSTACKTRACE_H

#include "polarphp/basic/FileTypes.h"
#include "llvm/Support/PrettyStackTrace.h"

namespace polar {
namespace driver {

class Action;
class Job;
class CommandOutput;

class PrettyStackTraceDriverAction : public llvm::PrettyStackTraceEntry {
   const Action *TheAction;
   const char *Description;

public:
   PrettyStackTraceDriverAction(const char *desc, const Action *A)
      : TheAction(A), Description(desc) {}
   void print(llvm::raw_ostream &OS) const override;
};

class PrettyStackTraceDriverJob : public llvm::PrettyStackTraceEntry {
   const Job *TheJob;
   const char *Description;

public:
   PrettyStackTraceDriverJob(const char *desc, const Job *A)
      : TheJob(A), Description(desc) {}
   void print(llvm::raw_ostream &OS) const override;
};

class PrettyStackTraceDriverCommandOutput : public llvm::PrettyStackTraceEntry {
   const CommandOutput *TheCommandOutput;
   const char *Description;

public:
   PrettyStackTraceDriverCommandOutput(const char *desc, const CommandOutput *A)
      : TheCommandOutput(A), Description(desc) {}
   void print(llvm::raw_ostream &OS) const override;
};

class PrettyStackTraceDriverCommandOutputAddition
   : public llvm::PrettyStackTraceEntry {
   const CommandOutput *TheCommandOutput;
   StringRef PrimaryInput;
   filetypes::FileTypeId NewOutputType;
   StringRef NewOutputName;
   const char *Description;

public:
   PrettyStackTraceDriverCommandOutputAddition(const char *desc,
                                               const CommandOutput *A,
                                               StringRef Primary,
                                               filetypes::FileTypeId type,
                                               StringRef New)
      : TheCommandOutput(A), PrimaryInput(Primary), NewOutputType(type),
        NewOutputName(New), Description(desc) {}
   void print(llvm::raw_ostream &OS) const override;
};
} // driver
} // polar

#endif // POLARPHP_DRIVER_PRETTYSTACKTRACE_H
