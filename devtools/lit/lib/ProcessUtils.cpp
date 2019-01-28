// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#include "ProcessUtils.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/utils/OptionalError.h"

#include "Utils.h"
#include <stack>

namespace polar {
namespace lit {

using polar::basic::SmallString;
using polar::basic::SmallVector;
using polar::utils::MemoryBuffer;
using polar::fs::create_temporary_file;
using polar::fs::FileRemover;

std::tuple<std::list<pid_t>, bool> retrieve_children_pids(pid_t pid, bool recursive) noexcept
{
   if (!recursive) {
      return call_pgrep_command(pid);
   }
   std::list<pid_t> resultList;
   std::stack<pid_t> workList;
   std::tuple<std::list<pid_t>, bool> selfPids = call_pgrep_command(pid);
   if (std::get<1>(selfPids)) {
      // init pids
      for (pid_t curpid : std::get<0>(selfPids)) {
         workList.push(curpid);
      }
      while (!workList.empty()) {
         pid_t topPid = workList.top();
         resultList.push_back(topPid);
         workList.pop();
         std::tuple<std::list<pid_t>, bool> selfPids = call_pgrep_command(topPid);
         if (std::get<1>(selfPids)) {
            for (pid_t curpid : std::get<0>(selfPids)) {
               workList.push(curpid);
            }
         }
      }
   } else {
      return selfPids;
   }
   return std::make_tuple(resultList, true);
}

std::tuple<std::list<pid_t>, bool> call_pgrep_command(pid_t pid) noexcept
{
   polar::utils::OptionalError<std::string> findResult = polar::sys::find_program_by_name("pgrep");
   assert(findResult && "pgrep command not found.");
   std::string pidStr = std::to_string(pid);
   ArrayRef<StringRef> args{
      "pgrep",
      "-P",
      pidStr
   };
   RunCmdResponse result = execute_and_wait(findResult.get(), args);
   if (0 != std::get<0>(result)) {
      return std::make_tuple(std::list<pid_t>{}, false);
   }
   std::list<int32_t> pids;
   for (const std::string &item : split_string(std::get<1>(result), '\n')) {
      pids.push_back(std::stoi(item));
   }
   return std::make_tuple(pids, true);
}

int execute_and_wait(
      StringRef program,
      ArrayRef<StringRef> args,
      std::optional<StringRef> cwd,
      std::optional<ArrayRef<StringRef>> env,
      ArrayRef<std::optional<StringRef>> redirects,
      unsigned secondsToWait,
      unsigned memoryLimit,
      std::string *errMsg,
      bool *executionFailed)
{
   ArrayRef<std::optional<int>> openModes{
      std::nullopt,
            std::nullopt,
            std::nullopt
   };
   return execute_and_wait(program, args, cwd, env, redirects, openModes, secondsToWait,
                           memoryLimit, errMsg, executionFailed);
}

RunCmdResponse execute_and_wait(
      StringRef program,
      ArrayRef<StringRef> args,
      std::optional<StringRef> cwd,
      std::optional<ArrayRef<StringRef>> env,
      unsigned secondsToWait,
      unsigned memoryLimit,
      std::string *errMsg,
      bool *executionFailed)
{
   SmallString<32> outTempFilename;
   fs::create_temporary_file(TESTRUNNER_TEMP_PREFIX, "", outTempFilename);
   SmallString<32> errorTempFilename;
   fs::create_temporary_file(TESTRUNNER_TEMP_PREFIX, "", errorTempFilename);
   FileRemover outTempRemover(outTempFilename);
   FileRemover errTempRemover(errorTempFilename);
   ArrayRef<std::optional<StringRef>> redirects{
      std::nullopt,
            outTempFilename,
            errorTempFilename
   };
   int exitCode = execute_and_wait(program, args, cwd, env,
                                   redirects, secondsToWait, memoryLimit, errMsg, executionFailed);
   std::string output;
   std::string errorMsg;
   if (0 != exitCode) {
      auto errorMsgBuffer = MemoryBuffer::getFile(errorTempFilename);
      if (errorMsgBuffer) {
         errorMsg = errorMsgBuffer.get()->getBuffer().getStr();
      } else {
         // here get the buffer info error
         errorMsg = format_string("get error output buffer error: %s",
                                  errorMsgBuffer.getError().message());
         exitCode = -3;
      }
   }
   auto outputBuf = MemoryBuffer::getFile(outTempFilename);
   if (outputBuf) {
      output = outputBuf.get()->getBuffer().getStr();
   } else {
      // here get the buffer info error
      errorMsg = format_string("get output buffer error: %s",
                               outputBuf.getError().message());
      exitCode = -3;
   }
   return std::make_tuple(exitCode, output, errorMsg);
}

} // lit
} // polar
