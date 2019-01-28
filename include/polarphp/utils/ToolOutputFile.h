// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_TOOL_OUTPUT_FILE_H
#define POLARPHP_UTILS_TOOL_OUTPUT_FILE_H

#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

/// This class contains a raw_fd_ostream and adds a few extra features commonly
/// needed for compiler-like tool output files:
///   - The file is automatically deleted if the process is killed.
///   - The file is automatically deleted when the ToolOutputFile
///     object is destroyed unless the client calls keep().
class ToolOutputFile
{
   /// This class is declared before the raw_fd_ostream so that it is constructed
   /// before the raw_fd_ostream is constructed and destructed after the
   /// raw_fd_ostream is destructed. It installs cleanups in its constructor and
   /// uninstalls them in its destructor.
   class CleanupInstaller
   {
      /// The name of the file.
      std::string m_filename;
   public:
      /// The flag which indicates whether we should not delete the file.
      bool m_keep;

      explicit CleanupInstaller(StringRef filename);
      ~CleanupInstaller();
   } m_installer;

   /// The contained stream. This is intentionally declared after m_installer.
   RawFdOutStream m_outstream;

public:
   /// This constructor's arguments are passed to to raw_fd_ostream's
   /// constructor.
   ToolOutputFile(StringRef filename, std::error_code &errorCode,
                  polar::fs::OpenFlags flags);

   ToolOutputFile(StringRef filename, int fd);

   /// Return the contained raw_fd_ostream.
   RawFdOutStream &getOutStream()
   {
      return m_outstream;
   }

   /// Indicate that the tool's job wrt this output file has been successful and
   /// the file should not be deleted.
   void keep()
   {
      m_installer.m_keep = true;
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_TOOL_OUTPUT_FILE_H
