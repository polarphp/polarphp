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

namespace polar::driver {

class Action;
class Job;
class CommandOutput;

class PrettyStackTraceDriverAction : public llvm::PrettyStackTraceEntry
{
public:
   PrettyStackTraceDriverAction(const char *desc, const Action *action)
      : m_theAction(action),
        m_description(desc)
   {}

   void print(llvm::raw_ostream &OS) const override;
private:
   const Action *m_theAction;
   const char *m_description;
};

class PrettyStackTraceDriverJob : public llvm::PrettyStackTraceEntry
{
public:
   PrettyStackTraceDriverJob(const char *desc, const Job *job)
      : m_theJob(job),
        m_description(desc) {}
   void print(llvm::raw_ostream &stream) const override;
private:
   const Job *m_theJob;
   const char *m_description;
};

class PrettyStackTraceDriverCommandOutput : public llvm::PrettyStackTraceEntry
{
public:
   PrettyStackTraceDriverCommandOutput(const char *desc, const CommandOutput *output)
      : m_theCommandOutput(output),
        m_description(desc)
   {}

   void print(llvm::raw_ostream &OS) const override;
private:
   const CommandOutput *m_theCommandOutput;
   const char *m_description;
};

class PrettyStackTraceDriverCommandOutputAddition
      : public llvm::PrettyStackTraceEntry
{
public:
   PrettyStackTraceDriverCommandOutputAddition(const char *desc,
                                               const CommandOutput *output,
                                               StringRef Primary,
                                               filetypes::FileTypeId type,
                                               StringRef newOutputType)
      : m_theCommandOutput(output),
        m_primaryInput(Primary),
        m_newOutputType(type),
        m_newOutputName(newOutputType),
        m_description(desc)
   {}

   void print(llvm::raw_ostream &outStream) const override;
private:
   const CommandOutput *m_theCommandOutput;
   StringRef m_primaryInput;
   filetypes::FileTypeId m_newOutputType;
   StringRef m_newOutputName;
   const char *m_description;
};

} // polar::driver

#endif // POLARPHP_DRIVER_PRETTYSTACKTRACE_H
