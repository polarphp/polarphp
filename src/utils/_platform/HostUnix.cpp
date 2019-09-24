//===- llvm/Support/Unix/Host.inc -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
// Created by polarboy on 2018/06/20.

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only generic UNIX code that
//===          is guaranteed to work on *all* UNIX variants.
//===----------------------------------------------------------------------===//

#include "polarphp/utils/unix/Unix.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/global/Config.h"
#include "polarphp/basic/adt/Triple.h"
#include <cctype>
#include <string>
#include <sys/utsname.h>

namespace polar::sys {

using polar::basic::Triple;

std::string get_os_version()
{
   struct utsname info;
   if (uname(&info)) {
      return "";
   }
   return info.release;
}

std::string update_triple_os_version(std::string targetTripleString)
{
   // On darwin, we want to update the version to match that of the target.
   std::string::size_type darwinDashIdx = targetTripleString.find("-darwin");
   if (darwinDashIdx != std::string::npos) {
      targetTripleString.resize(darwinDashIdx + strlen("-darwin"));
      targetTripleString += get_os_version();
      return targetTripleString;
   }
   std::string::size_type macOSDashIdx = targetTripleString.find("-macos");
   if (macOSDashIdx != std::string::npos) {
      targetTripleString.resize(macOSDashIdx);
      // Reset the OS to darwin as the OS version from `uname` doesn't use the
      // macOS version scheme.
      targetTripleString += "-darwin";
      targetTripleString += get_os_version();
   }
   // On AIX, the AIX version and release should be that of the current host
   // unless if the version has already been specified.
   if (Triple(POLAR_HOST_TRIPLE).getOS() == Triple::OSType::AIX) {
      Triple tempTriple(targetTripleString);
      if (tempTriple.getOS() == Triple::OSType::AIX && !tempTriple.getOSMajorVersion()) {
         struct utsname name;
         if (uname(&name) != -1) {
            std::string newOsName = Triple::getOSTypeName(Triple::OSType::AIX);
            newOsName += name.version;
            newOsName += '.';
            newOsName += name.release;
            newOsName += ".0.0";
            tempTriple.setOSName(newOsName);
            return tempTriple.getStr();
         }
      }
   }
   return targetTripleString;
}

std::string get_default_target_triple()
{
   std::string targetTripleString =
         update_triple_os_version(POLAR_DEFAULT_TARGET_TRIPLE);

   // Override the default target with an environment variable named by
   // LOLAR_TARGET_TRIPLE_ENV.
#if defined(POLAR_TARGET_TRIPLE_ENV)
   if (const char *envTriple = std::getenv(POLAR_TARGET_TRIPLE_ENV)) {
      targetTripleString = envTriple;
   }
#endif

   return Triple::normalize(targetTripleString);
}

} // polar::sys
