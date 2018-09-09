// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/09.

#ifndef POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H
#define POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H

#include "Global.h"
#include <stdexcept>
#include <string>
#include <regex>

namespace polar {
namespace lit {

class InternalShellError : public std::runtime_error
{
public:
   InternalShellError(const std::string &command, const std::string &message)
      : std::runtime_error(command + ": " + message)
   {}
   const std::string &getCommand() const
   {
      return m_command;
   }
   const std::string &getMessage() const
   {
      return m_message;
   }
protected:
   const std::string &m_command;
   const std::string &m_message;
};

#ifndef POLAR_OS_WIN32
#define POLAR_KUSE_CLOSE_FDS
#else
#define POLAR_AVOID_DEV_NULL
#endif

const static std::string sgc_kdevNull("/dev/null");
const static std::regex sgc_kpdbgRegex("%dbg\(([^)'\"]*)\)");

class ShellEnvironment
{
public:
   ShellEnvironment(const std::string &cwd, std::map<std::string, std::string> &env)
      : m_cwd(cwd),
        m_env(env)
   {}
protected:
   std::string m_cwd;
   std::map<std::string, std::string> m_env;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TEST_RUNNER_H


