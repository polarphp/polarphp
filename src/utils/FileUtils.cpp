// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/17.

//===- Support/FileUtilities.cpp - File System Utilities ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a family of utility functions which are useful for doing
// various things with files.
//
//===----------------------------------------------------------------------===//

// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/11.

//===----------------------------------------------------------------------===//
//
// This file implements a family of utility functions which are useful for doing
// various things with files.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/FileUtils.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/RawOutStream.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <system_error>

namespace polar {
namespace fs {

using polar::utils::RawStringOutStream;
using polar::utils::MemoryBuffer;
using polar::utils::OptionalError;

namespace {

bool is_signed_char(char c)
{
   return (c == '+' || c == '-');
}

bool is_exponent_char(char c)
{
   switch (c) {
   case 'D':  // Strange exponential notation.
   case 'd':  // Strange exponential notation.
   case 'e':
   case 'E': return true;
   default: return false;
   }
}

bool is_number_char(char c)
{
   switch (c) {
   case '0': case '1': case '2': case '3': case '4':
   case '5': case '6': case '7': case '8': case '9':
   case '.': return true;
   default: return is_signed_char(c) || is_exponent_char(c);
   }
}

const char *backup_number(const char *pos, const char *firstChar)
{
   // If we didn't stop in the middle of a number, don't backup.
   if (!is_number_char(*pos)) {
      return pos;
   }
   // Otherwise, return to the start of the number.
   bool hasPeriod = false;
   while (pos > firstChar && is_number_char(pos[-1])) {
      // Backup over at most one period.
      if (pos[-1] == '.') {
         if (hasPeriod) {
            break;
         }
         hasPeriod = true;
      }
      --pos;
      if (pos > firstChar && is_signed_char(pos[0]) && !is_exponent_char(pos[-1])) {
         break;
      }
   }
   return pos;
}

/// end_of_number - Return the first character that is not part of the specified
/// number.  This assumes that the buffer is null terminated, so it won't fall
/// off the end.
const char *end_of_number(const char *pos)
{
   while (is_number_char(*pos)) {
      ++pos;
   }
   return pos;
}

/// compare_numbers - compare two numbers, returning true if they are different.
bool compare_numbers(const char *&lhsPtr, const char *&rhsPtr,
                     const char *lhsEndPtr, const char *rhsEndPtr,
                     double absTolerance, double relTolerance,
                     std::string *errorMsg)
{
   const char *lhsNumEnd, *rhsNumEnd;
   double v1 = 0.0;
   double v2 = 0.0;

   // If one of the positions is at a space and the other isn't, chomp up 'til
   // the end of the space.
   while (isspace(static_cast<unsigned char>(*lhsPtr)) && lhsPtr != lhsEndPtr) {
      ++lhsPtr;
   }

   while (isspace(static_cast<unsigned char>(*rhsPtr)) && rhsPtr != rhsEndPtr) {
      ++rhsPtr;
   }
   // If we stop on numbers, compare their difference.
   if (!is_number_char(*lhsPtr) || !is_number_char(*rhsPtr)) {
      // The diff failed.
      lhsNumEnd = lhsPtr;
      rhsNumEnd = rhsPtr;
   } else {
      // Note that some ugliness is built into this to permit support for numbers
      // that use "D" or "d" as their exponential marker, e.g. "1.234D45".  This
      // occurs in 200.sixtrack in spec2k.
      v1 = strtod(lhsPtr, const_cast<char**>(&lhsNumEnd));
      v2 = strtod(rhsPtr, const_cast<char**>(&rhsNumEnd));

      if (*lhsNumEnd == 'D' || *lhsNumEnd == 'd') {
         // Copy string into tmp buffer to replace the 'D' with an 'e'.
         SmallString<200> strTmp(lhsPtr, end_of_number(lhsNumEnd)+1);
         // Strange exponential notation!
         strTmp[static_cast<unsigned>(lhsNumEnd-lhsPtr)] = 'e';

         v1 = strtod(&strTmp[0], const_cast<char**>(&lhsNumEnd));
         lhsNumEnd = lhsPtr + (lhsNumEnd-&strTmp[0]);
      }

      if (*rhsNumEnd == 'D' || *rhsNumEnd == 'd') {
         // Copy string into tmp buffer to replace the 'D' with an 'e'.
         SmallString<200> strTmp(rhsPtr, end_of_number(rhsNumEnd)+1);
         // Strange exponential notation!
         strTmp[static_cast<unsigned>(rhsNumEnd-rhsPtr)] = 'e';
         v2 = strtod(&strTmp[0], const_cast<char**>(&rhsNumEnd));
         rhsNumEnd = rhsPtr + (rhsNumEnd-&strTmp[0]);
      }
   }

   if (lhsNumEnd == lhsPtr || rhsNumEnd == rhsPtr) {
      if (errorMsg) {
         *errorMsg = "FP Comparison failed, not a numeric difference between '";
         *errorMsg += lhsPtr[0];
         *errorMsg += "' and '";
         *errorMsg += rhsPtr[0];
         *errorMsg += "'";
      }
      return true;
   }

   // Check to see if these are inside the absolute tolerance
   if (absTolerance < std::abs(v1-v2)) {
      // Nope, check the relative tolerance...
      double diff;
      if (v2) {
         diff = std::abs(v1/v2 - 1.0);
      } else if (v1) {
         diff = std::abs(v2/v1 - 1.0);
      } else {
         diff = 0;  // Both zero.
      }
      if (diff > relTolerance) {
         if (errorMsg) {
            RawStringOutStream(*errorMsg)
                  << "Compared: " << v1 << " and " << v2 << '\n'
                  << "abs. diff = " << std::abs(v1-v2) << " rel.diff = " << diff << '\n'
                  << "Out of tolerance: rel/abs: " << relTolerance << '/'
                  << absTolerance;
         }
         return true;
      }
   }

   // Otherwise, advance our read pointers to the end of the numbers.
   lhsPtr = lhsNumEnd;
   rhsPtr = rhsNumEnd;
   return false;
}

} // anonymous namespace

/// diff_files_with_tolerance - Compare the two files specified, returning 0 if the
/// files match, 1 if they are different, and 2 if there is a file error.  This
/// function differs from DiffFiles in that you can specify an absolete and
/// relative FP error that is allowed to exist.  If you specify a string to fill
/// in for the error option, it will set the string to an error message if an
/// error occurs, allowing the caller to distinguish between a failed diff and a
/// file system error.
///
int diff_files_with_tolerance(StringRef lhs,
                              StringRef rhs,
                              double absTol, double relTol,
                              std::string *error)
{
   // Now its safe to mmap the files into memory because both files
   // have a non-zero size.
   OptionalError<std::unique_ptr<MemoryBuffer>> lhsOrErr = MemoryBuffer::getFile(lhs);
   if (std::error_code errorCode = lhsOrErr.getError()) {
      if (error) {
         *error = errorCode.message();
      }
      return 2;
   }
   MemoryBuffer &lhsFile = *lhsOrErr.get();

   OptionalError<std::unique_ptr<MemoryBuffer>> rhsOrErr = MemoryBuffer::getFile(rhs);
   if (std::error_code errorCode = rhsOrErr.getError()) {
      if (error) {
         *error = errorCode.message();
      }
      return 2;
   }
   MemoryBuffer &rhsFile = *rhsOrErr.get();

   // Okay, now that we opened the files, scan them for the first difference.
   const char *lhsFileStart = lhsFile.getBufferStart();
   const char *rhsFileStart = rhsFile.getBufferStart();
   const char *lhsFileEnd = lhsFile.getBufferEnd();
   const char *rhsFileEnd = rhsFile.getBufferEnd();
   const char *lhsPtr = lhsFileStart;
   const char *rhsPtr = rhsFileStart;
   uint64_t lhsSize = lhsFile.getBufferSize();
   uint64_t rhsSize = rhsFile.getBufferSize();

   // Are the buffers identical?  Common case: Handle this efficiently.
   if (lhsSize == rhsSize &&
       std::memcmp(lhsFileStart, rhsFileStart, lhsSize) == 0) {
      return 0;
   }

   // Otherwise, we are done a tolerances are set.
   if (absTol == 0 && relTol == 0) {
      if (error) {
         *error = "Files differ without tolerance allowance";
      }
      return 1;   // Files different!
   }

   bool compareFailed = false;
   while (true) {
      // Scan for the end of file or next difference.
      while (lhsPtr < lhsFileEnd && rhsPtr < rhsFileEnd && *lhsPtr == *rhsPtr) {
         ++lhsPtr;
         ++rhsPtr;
      }

      if (lhsPtr >= lhsFileEnd || rhsPtr >= rhsFileEnd) break;

      // Okay, we must have found a difference.  Backup to the start of the
      // current number each stream is at so that we can compare from the
      // beginning.
      lhsPtr = backup_number(lhsPtr, lhsFileStart);
      rhsPtr = backup_number(rhsPtr, rhsFileStart);

      // Now that we are at the start of the numbers, compare them, exiting if
      // they don't match.
      if (compare_numbers(lhsPtr, rhsPtr, lhsFileEnd, rhsFileEnd, absTol, relTol, error)) {
         compareFailed = true;
         break;
      }
   }

   // Okay, we reached the end of file.  If both files are at the end, we
   // succeeded.
   bool lhsFileAtEnd = lhsPtr >= lhsFileEnd;
   bool rhsFileAtEnd = rhsPtr >= rhsFileEnd;
   if (!compareFailed && (!lhsFileAtEnd || !rhsFileAtEnd)) {
      // Else, we might have run off the end due to a number: backup and retry.
      if (lhsFileAtEnd && is_number_char(lhsPtr[-1])) --lhsPtr;
      if (rhsFileAtEnd && is_number_char(rhsPtr[-1])) --rhsPtr;
      lhsPtr = backup_number(lhsPtr, lhsFileStart);
      rhsPtr = backup_number(rhsPtr, rhsFileStart);

      // Now that we are at the start of the numbers, compare them, exiting if
      // they don't match.
      if (compare_numbers(lhsPtr, rhsPtr, lhsFileEnd, rhsFileEnd, absTol, relTol, error)) {
         compareFailed = true;
      }
      // If we found the end, we succeeded.
      if (lhsPtr < lhsFileEnd || rhsPtr < rhsFileEnd) {
         compareFailed = true;
      }
   }

   return compareFailed;
}

} // fs
} // polar
