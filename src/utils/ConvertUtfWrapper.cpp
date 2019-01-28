// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/05.

#include "polarphp/utils/ConvertUtf.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/SwapByteOrder.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StringRef.h"

#include <string>
#include <vector>

namespace polar {
namespace utils {

using polar::basic::StringRef;

bool convert_utf8_to_wide(unsigned wideCharWidth, StringRef source,
                          char *&resultPtr, const Utf8 *&errorPtr)
{
   assert(wideCharWidth == 1 || wideCharWidth == 2 || wideCharWidth == 4);
   ConversionResult result = ConversionResult::ConversionOK;
   // Copy the character span over.
   if (wideCharWidth == 1) {
      const Utf8 *pos = reinterpret_cast<const Utf8*>(source.begin());
      if (!is_legal_utf8_string(&pos, reinterpret_cast<const Utf8*>(source.end()))) {
         result = ConversionResult::SourceIllegal;
         errorPtr = pos;
      } else {
         memcpy(resultPtr, source.getData(), source.getSize());
         resultPtr += source.getSize();
      }
   } else if (wideCharWidth == 2) {
      const Utf8 *sourceStart = (const Utf8*)source.getData();
      // FIXME: Make the type of the result buffer correct instead of
      // using reinterpret_cast.
      Utf16 *targetStart = reinterpret_cast<Utf16*>(resultPtr);
      ConversionFlags flags = ConversionFlags::StrictConversion;
      result = convert_utf8_to_utf16(
               &sourceStart, sourceStart + source.getSize(),
               &targetStart, targetStart + source.getSize(), flags);
      if (result == ConversionResult::ConversionOK) {
         resultPtr = reinterpret_cast<char*>(targetStart);
      } else {
         errorPtr = sourceStart;
      }
   } else if (wideCharWidth == 4) {
      const Utf8 *sourceStart = (const Utf8*)source.getData();
      // FIXME: Make the type of the result buffer correct instead of
      // using reinterpret_cast.
      Utf32 *targetStart = reinterpret_cast<Utf32*>(resultPtr);
      ConversionFlags flags = ConversionFlags::StrictConversion;
      result = convert_utf8_to_utf32(
               &sourceStart, sourceStart + source.getSize(),
               &targetStart, targetStart + source.getSize(), flags);
      if (result == ConversionResult::ConversionOK) {
         resultPtr = reinterpret_cast<char*>(targetStart);
      } else {
         errorPtr = sourceStart;
      }
   }
   assert((result != ConversionResult::TargetExhausted)
          && "ConvertUTF8toutFXX exhausted target buffer");
   return result == ConversionResult::ConversionOK;
}

bool convert_code_point_to_utf8(char32_t source, char *&resultPtr)
{
   const Utf32 *sourceStart = &source;
   const Utf32 *sourceEnd = sourceStart + 1;
   Utf8 *targetStart = reinterpret_cast<Utf8 *>(resultPtr);
   Utf8 *targetEnd = targetStart + 4;
   ConversionResult result = convert_utf32_to_utf8(&sourceStart, sourceEnd,
                                                   &targetStart, targetEnd,
                                                   ConversionFlags::StrictConversion);
   if (result != ConversionResult::ConversionOK) {
      return false;
   }
   resultPtr = reinterpret_cast<char*>(targetStart);
   return true;
}

bool has_utf16_byte_order_mark(ArrayRef<char> str)
{
   return (str.getSize() >= 2 &&
           ((str[0] == '\xff' && str[1] == '\xfe') ||
         (str[0] == '\xfe' && str[1] == '\xff')));
}

bool convert_utf16_to_utf8_string(ArrayRef<char> srcBytes, std::string &out)
{
   assert(out.empty());

   // Error out on an uneven byte count.
   if (srcBytes.getSize() % 2) {
      return false;
   }
   // Avoid OOB by returning early on empty input.
   if (srcBytes.empty()) {
      return true;
   }
   const Utf16 *src = reinterpret_cast<const Utf16 *>(srcBytes.begin());
   const Utf16 *srcEnd = reinterpret_cast<const Utf16 *>(srcBytes.end());

   // Byteswap if necessary.
   std::vector<Utf16> byteSwapped;
   if (src[0] == UNI_UTF16_BYTE_ORDER_MARK_SWAPPED) {
      byteSwapped.insert(byteSwapped.end(), src, srcEnd);
      for (unsigned index = 0, endMark = byteSwapped.size(); index != endMark; ++index) {
         byteSwapped[index] = polar::utils::swap_byte_order16(byteSwapped[index]);
      }
      src = &byteSwapped[0];
      srcEnd = &byteSwapped[byteSwapped.size() - 1] + 1;
   }

   // Skip the BOM for conversion.
   if (src[0] == UNI_UTF16_BYTE_ORDER_MARK_NATIVE) {
      src++;
   }
   // Just allocate enough space up front.  We'll shrink it later.  Allocate
   // enough that we can fit a null terminator without reallocating.
   out.resize(srcBytes.getSize() * UNI_MAX_UTF8_BYTES_PER_CODE_POINT + 1);
   Utf8 *dest = reinterpret_cast<Utf8 *>(&out[0]);
   Utf8 *destEnd = dest + out.size();

   ConversionResult result =
         convert_utf16_to_utf8(&src, srcEnd, &dest, destEnd, ConversionFlags::StrictConversion);
   assert(result != ConversionResult::TargetExhausted);

   if (result != ConversionResult::ConversionOK) {
      out.clear();
      return false;
   }

   out.resize(reinterpret_cast<char *>(dest) - &out[0]);
   out.push_back(0);
   out.pop_back();
   return true;
}

bool convert_utf16_to_utf8_string(ArrayRef<Utf16> src, std::string &out)
{
   return convert_utf16_to_utf8_string(
            ArrayRef<char>(reinterpret_cast<const char *>(src.getData()),
                           src.getSize() * sizeof(Utf16)), out);
}

bool convert_utf8_to_utf16_string(StringRef src,
                                  SmallVectorImpl<Utf16> &dest)
{
   assert(dest.empty());

   // Avoid OOB by returning early on empty input.
   if (src.empty()) {
      dest.push_back(0);
      dest.pop_back();
      return true;
   }

   const Utf8 *csrc = reinterpret_cast<const Utf8*>(src.begin());
   const Utf8 *csrcEnd = reinterpret_cast<const Utf8*>(src.end());

   // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
   // as UTF-16 should always require the same amount or less code units than the
   // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
   // so that someone calling destUTF16.data() gets a null terminated string.
   // We resize down later so we don't have to worry that this over allocates.
   dest.resize(src.getSize() + 1);
   Utf16 *destUtf16 = &dest[0];
   Utf16 *destUtf16End = destUtf16 + dest.getSize();

   ConversionResult cresult =
         convert_utf8_to_utf16(&csrc, csrcEnd, &destUtf16, destUtf16End, ConversionFlags::StrictConversion);
   assert(cresult != ConversionResult::TargetExhausted);

   if (cresult != ConversionResult::ConversionOK) {
      dest.clear();
      return false;
   }

   dest.resize(destUtf16 - &dest[0]);
   dest.push_back(0);
   dest.pop_back();
   return true;
}

static_assert(sizeof(wchar_t) == 1 || sizeof(wchar_t) == 2 ||
              sizeof(wchar_t) == 4,
              "Expected wchar_t to be 1, 2, or 4 bytes");

template <typename TResult>
static inline bool convert_utf8_to_wide_internal(StringRef source,
                                                 TResult &result)
{
   // Even in the case of UTF-16, the number of bytes in a UTF-8 string is
   // at least as large as the number of elements in the resulting wide
   // string, because surrogate pairs take at least 4 bytes in UTF-8.
   result.resize(source.getSize() + 1);
   char *resultPtr = reinterpret_cast<char *>(&result[0]);
   const Utf8 *errorPtr;
   if (!convert_utf8_to_wide(sizeof(wchar_t), source, resultPtr, errorPtr)) {
      result.clear();
      return false;
   }
   result.resize(reinterpret_cast<wchar_t *>(resultPtr) - &result[0]);
   return true;
}

bool convert_utf8_to_wide(StringRef source, std::wstring &result)
{
   return convert_utf8_to_wide_internal(source, result);
}

bool convert_utf8_to_wide(const char *source, std::wstring &result)
{
   if (!source) {
      result.clear();
      return true;
   }
   return convert_utf8_to_wide(StringRef(source), result);
}

bool convert_wide_to_utf8(const std::wstring &source, std::string &result)
{
   if (sizeof(wchar_t) == 1) {
      const Utf8 *start = reinterpret_cast<const Utf8 *>(source.data());
      const Utf8 *end =
            reinterpret_cast<const Utf8 *>(source.data() + source.size());
      if (!is_legal_utf8_string(&start, end)) {
         return false;
      }
      result.resize(source.size());
      memcpy(&result[0], source.data(), source.size());
      return true;
   } else if (sizeof(wchar_t) == 2) {
      return convert_utf16_to_utf8_string(
               ArrayRef<Utf16>(reinterpret_cast<const Utf16 *>(source.data()),
                               source.size()),
               result);
   } else if (sizeof(wchar_t) == 4) {
      const Utf32 *start = reinterpret_cast<const Utf32 *>(source.data());
      const Utf32 *end =
            reinterpret_cast<const Utf32 *>(source.data() + source.size());
      result.resize(UNI_MAX_UTF8_BYTES_PER_CODE_POINT * source.size());
      Utf8 *resultPtr = reinterpret_cast<Utf8 *>(&result[0]);
      Utf8 *resultEnd = reinterpret_cast<Utf8 *>(&result[0] + result.size());
      if (convert_utf32_to_utf8(&start, end, &resultPtr, resultEnd,
                                ConversionFlags::StrictConversion) == ConversionResult::ConversionOK) {
         result.resize(reinterpret_cast<char *>(resultPtr) - &result[0]);
         return true;
      } else {
         result.clear();
         return false;
      }
   } else {
      polar_unreachable(
               "Control should never reach this point; see static_assert further up");
   }
}

} // utils
} // polar
