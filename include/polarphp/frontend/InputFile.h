//===--- InputFile.h --------------------------------------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_FRONTEND_INPUT_FILE_H
#define POLARPHP_FRONTEND_INPUT_FILE_H

#include "polarphp/basic/PrimarySpecificPaths.h"
#include "polarphp/basic/SupplementaryOutputPaths.h"
#include "llvm/Support/MemoryBuffer.h"
#include <string>
#include <vector>

namespace polar::frontend {

using polar::basic::PrimarySpecificPaths;
using polar::basic::SupplementaryOutputPaths;

enum class InputFileKind
{
   None,
   Polarphp,
   PolarphpLibrary,
   PolarphpREPL,
   PolarphpModuleInterface,
   PIL,
   LLVM
};

// Inputs may include buffers that override contents, and eventually should
// always include a buffer.
class InputFile
{
public:
   /// Does not take ownership of \p buffer. Does take ownership of (copy) a
   /// string.
   InputFile(StringRef name, bool isPrimary,
             llvm::MemoryBuffer *buffer = nullptr,
             StringRef outputFilename = StringRef())
      : m_filename(
           convertBufferNameFromLLVMGetFileOrSTDINToPolarphpConventions(name)),
        m_isPrimary(isPrimary),
        m_buffer(buffer),
        m_psps(PrimarySpecificPaths())
   {
      assert(!name.empty());
   }

   bool isPrimary() const
   {
      return m_isPrimary;
   }

   llvm::MemoryBuffer *buffer() const
   {
      return m_buffer;
   }

   const std::string &file() const
   {
      assert(!m_filename.empty());
      return m_filename;
   }

   /// Return Swift-standard file name from a buffer name set by
   /// llvm::MemoryBuffer::getFileOrSTDIN, which uses "<stdin>" instead of "-".
   static StringRef convertBufferNameFromLLVMGetFileOrSTDINToPolarphpConventions(
         StringRef filename)
   {
      return filename.equals("<stdin>") ? "-" : filename;
   }

   std::string outputFilename() const
   {
      return m_psps.outputFilename;
   }

   const PrimarySpecificPaths &getPrimarySpecificPaths() const
   {
      return m_psps;
   }

   void setPrimarySpecificPaths(const PrimarySpecificPaths &psps)
   {
      this->m_psps = psps;
   }

   // The next set of functions provides access to those primary-specific paths
   // accessed directly from an InputFile, as opposed to via
   // FrontendInputsAndOutputs. They merely make the call sites
   // a bit shorter. Add more forwarding methods as needed.

   std::string dependenciesFilePath() const
   {
      return getPrimarySpecificPaths().supplementaryOutputs.dependenciesFilePath;
   }

   std::string loadedModuleTracePath() const
   {
      return getPrimarySpecificPaths().supplementaryOutputs.loadedModuleTracePath;
   }

   std::string serializedDiagnosticsPath() const
   {
      return getPrimarySpecificPaths().supplementaryOutputs
            .serializedDiagnosticsPath;
   }

   std::string fixItsOutputPath() const
   {
      return getPrimarySpecificPaths().supplementaryOutputs.fixItsOutputPath;
   }

private:
   std::string m_filename;
   bool m_isPrimary;
   /// Points to a buffer overriding the file's contents, or nullptr if there is
   /// none.
   llvm::MemoryBuffer *m_buffer;

   /// If there are explicit primary inputs (i.e. designated with -primary-input
   /// or -primary-filelist), the paths specific to those inputs (other than the
   /// input file path itself) are kept here. If there are no explicit primary
   /// inputs (for instance for whole module optimization), the corresponding
   /// paths are kept in the first input file.
   PrimarySpecificPaths m_psps;
};

} // polar::frontend

#endif // POLARPHP_FRONTEND_INPUT_FILE_H
