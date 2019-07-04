// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/11.

#include "polarphp/utils/MathExtras.h"
#ifdef _MSC_VER
#include <limits>
#else
#include <math.h>
#endif

namespace polar {
namespace utils {

#if defined(_MSC_VER)
// Visual Studio defines the HUGE_VAL class of macros using purposeful
// constant arithmetic overflow, which it then warns on when encountered.
const float huge_valf = std::numeric_limits<float>::infinity();
#else
const float huge_valf = HUGE_VALF;
#endif

double bstr_to_double(const char *str, const char **endptr)
{
   const char *s = str;
   char c;
   double value = 0;
   bool any = false;

   if ('0' == *s && ('b' == s[1] || 'B' == s[1])) {
      s += 2;
   }

   while ((c = *s++)) {
      /*
          * Verify the validity of the current character as a base-2 digit.  In
          * the event that an invalid digit is found, halt the conversion and
          * return the portion which has been converted thus far.
          */
      if ('0' == c || '1' == c) {
         value = value * 2 + c - '0';
      } else {
         break;
      }
      any = true;
   }
   /*
       * As with many strtoX implementations, should the subject sequence be
       * empty or not well-formed, no conversion is performed and the original
       * value of str is stored in *endptr, provided that endptr is not a null
       * pointer.
       */
   if (nullptr != endptr) {
      *endptr = const_cast<char *>(any ? s - 1 : str);
   }
   return value;
}

double hexstr_to_double(const char *str, const char **endptr)
{
   const char *s = str;
   char c;
   bool any = false;
   double value = 0;

   if (*s == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;
   }

   while ((c = *s++)) {
      if (c >= '0' && c <= '9') {
         c -= '0';
      } else if (c >= 'A' && c <= 'F') {
         c -= 'A' - 10;
      } else if (c >= 'a' && c <= 'f') {
         c -= 'a' - 10;
      } else {
         break;
      }

      any = true;
      value = value * 16 + c;
   }

   if (endptr != nullptr) {
      *endptr = any ? s - 1 : str;
   }

   return value;
}

double octstr_to_double(const char *str, const char **endptr)
{
   const char *s = str;
   char c;
   double value = 0;
   bool any = false;

   if (str[0] == '\0') {
      if (endptr != nullptr) {
         *endptr = str;
      }
      return 0.0;
   }

   /* skip leading zero */
   s++;

   while ((c = *s++)) {
      if (c < '0' || c > '7') {
         /* break and return the current value if the number is not well-formed
             * that's what Linux strtol() does
             */
         break;
      }
      value = value * 8 + c - '0';
      any = true;
   }

   if (endptr != nullptr) {
      *endptr = any ? s - 1 : str;
   }

   return value;
}

} // utils
} // polar
