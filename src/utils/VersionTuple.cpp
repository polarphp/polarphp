// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/17.

#include "polarphp/utils/VersionTuple.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

using polar::utils::RawStringOutStream;

std::string VersionTuple::getAsString() const {
   std::string result;
   {
      RawStringOutStream out(result);
      out << *this;
   }
   return result;
}

RawOutStream &operator<<(RawOutStream &out, const VersionTuple &version)
{
   out << version.getMajor();
   if (std::optional<unsigned> minor = version.getMinor()) {
      out << '.' << *minor;
   }
   if (std::optional<unsigned> subminor = version.getSubminor()) {
      out << '.' << *subminor;
   }
   if (std::optional<unsigned> build = version.getBuild()) {
      out << '.' << *build;
   }
   return out;
}

static bool parseInt(StringRef &input, unsigned &value)
{
   assert(value == 0);
   if (input.empty()) {
      return true;
   }
   char next = input[0];
   input = input.substr(1);
   if (next < '0' || next > '9') {
      return true;
   }
   value = (unsigned)(next - '0');
   while (!input.empty()) {
      next = input[0];
      if (next < '0' || next > '9') {
         return false;
      }
      input = input.substr(1);
      value = value * 10 + (unsigned)(next - '0');
   }
   return false;
}

bool VersionTuple::tryParse(StringRef input)
{
   unsigned major = 0, minor = 0, micro = 0, build = 0;

   // Parse the major version, [0-9]+
   if (parseInt(input, major)) {
      return true;
   }
   if (input.empty()) {
      *this = VersionTuple(major);
      return false;
   }

   // If we're not done, parse the minor version, \.[0-9]+
   if (input[0] != '.') {
      return true;
   }

   input = input.substr(1);
   if (parseInt(input, minor)) {
      return true;
   }

   if (input.empty()) {
      *this = VersionTuple(major, minor);
      return false;
   }

   // If we're not done, parse the micro version, \.[0-9]+
   if (input[0] != '.') {
      return true;
   }

   input = input.substr(1);
   if (parseInt(input, micro)) {
      return true;
   }

   if (input.empty()) {
      *this = VersionTuple(major, minor, micro);
      return false;
   }

   // If we're not done, parse the micro version, \.[0-9]+
   if (input[0] != '.') {
      return true;
   }
   input = input.substr(1);
   if (parseInt(input, build)) {
      return true;
   }
   // If we have characters left over, it's an error.
   if (!input.empty()) {
      return true;
   }

   *this = VersionTuple(major, minor, micro, build);
   return false;
}

} // utils
} // polar

