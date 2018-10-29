// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/29.

#ifndef POLAR_DEVLTOOLS_LIT_FORWARD_DEFS_H
#define POLAR_DEVLTOOLS_LIT_FORWARD_DEFS_H

#include <memory>
#include <list>

namespace polar {
namespace basic {
class StringRef;
} // basic
} // polar

namespace polar {
namespace lit {

using polar::basic::StringRef;

class TestingConfig;
class LitConfig;

class Test;
class Result;
class ResultCode;
class MetricValue;
class TestSuite;
class LitTestCase;
class TestFormat;
class Run;
class TestingProgressDisplay;
class Command;

using RunPointer = std::shared_ptr<Run>;
using TestPointer = std::shared_ptr<Test>;
using TestSuitePointer = std::shared_ptr<TestSuite>;
using TestList = std::list<TestPointer>;
using TestSuiteList = std::list<TestSuitePointer>;
using TestSuitSearchResult = std::tuple<TestSuitePointer, std::list<std::string>>;
using LitConfigPointer = std::shared_ptr<LitConfig>;
using TestingConfigPointer = std::shared_ptr<TestingConfig>;
using ResultPointer = std::shared_ptr<Result>;
using MetricValuePointer = std::shared_ptr<MetricValue>;
using ParallelismGroupSetter = std::string (*)(TestPointer);

using OpenFileEntryType = std::tuple<std::string, std::string, int, std::string>;
using StdFdsTuple = std::tuple<int, int, int>;
using SubstitutionPair = std::pair<StringRef, std::string>;
using SubstitutionList = std::list<SubstitutionPair>;
using TestingProgressDisplayPointer = std::shared_ptr<TestingProgressDisplay>;
using ExecResultTuple = std::tuple<const ResultCode *, std::string>;

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_FORWARD_DEFS_H
