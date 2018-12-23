// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

#ifndef POLARPHP_UTILS_FILE_UTILS_H
#define POLARPHP_UTILS_FILE_UTILS_H

#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"

namespace polar {
namespace fs {

/// DiffFilesWithTolerance - Compare the two files specified, returning 0 if
/// the files match, 1 if they are different, and 2 if there is a file error.
/// This function allows you to specify an absolute and relative FP error that
/// is allowed to exist.  If you specify a string to fill in for the error
/// option, it will set the string to an error message if an error occurs, or
/// if the files are different.
///
int diff_files_with_tolerance(StringRef fileA,
                              StringRef fileB,
                              double absTol, double relTol,
                              std::string *error = nullptr);


/// FileRemover - This class is a simple object meant to be stack allocated.
/// If an exception is thrown from a region, the object removes the filename
/// specified (if deleteIt is true).
///
class FileRemover
{
   SmallString<128> m_filename;
   bool m_deleteIter;
public:
   FileRemover() : m_deleteIter(false)
   {}

   explicit FileRemover(const Twine& filename, bool deleteIt = true)
      : m_deleteIter(deleteIt)
   {
      filename.toVector(m_filename);
   }

   ~FileRemover()
   {
      if (m_deleteIter) {
         // Ignore problems deleting the file.
         fs::remove(m_filename);
      }
   }

   /// setFile - Give ownership of the file to the FileRemover so it will
   /// be removed when the object is destroyed.  If the FileRemover already
   /// had ownership of a file, remove it first.
   void setFile(const Twine& filename, bool deleteIt = true)
   {
      if (deleteIt) {
         // Ignore problems deleting the file.
         fs::remove(filename);
      }

      m_filename.clear();
      filename.toVector(m_filename);
      m_deleteIter = deleteIt;
   }

   /// releaseFile - Take ownership of the file away from the FileRemover so it
   /// will not be removed when the object is destroyed.
   void releaseFile()
   {
      m_deleteIter = false;
   }
};

} // fs
} // polar

#endif // POLARPHP_UTILS_FILE_UTILS_H
