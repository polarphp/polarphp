// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/DebugCounter.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/Options.h"

namespace polar {
namespace utils {

namespace {
// This class overrides the default list implementation of printing so we
// can pretty print the list of debug counter options.  This type of
// dynamic option is pretty rare (basically this and pass lists).
class DebugCounterList : public cmd::List<std::string, DebugCounter>
{
private:
   using Base = cmd::List<std::string, DebugCounter>;

public:
   template <class... Mods>
   explicit DebugCounterList(Mods &&... mods) : Base(std::forward<Mods>(mods)...)
   {}

private:
   void printOptionInfo(size_t globalWidth) const override
   {
      // This is a variant of from generic_parser_base::printOptionInfo.  Sadly,
      // it's not easy to make it more usable.  We could get it to print these as
      // options if we were a cl::opt and registered them, but lists don't have
      // options, nor does the parser for std::string.  The other mechanisms for
      // options are global and would pollute the global namespace with our
      // counters.  Rather than go that route, we have just overridden the
      // printing, which only a few things call anyway.
      out_stream() << "  -" << m_argStr;
      // All of the other options in CommandLine.cpp use m_argStr.size() + 6 for
      // width, so we do the same.
      Option::printHelpStr(m_helpStr, globalWidth, m_argStr.getSize() + 6);
      const auto &counterInstance = DebugCounter::getInstance();
      for (auto name : counterInstance) {
         const auto info =
               counterInstance.getCounterInfo(counterInstance.getCounterId(name));
         size_t NumSpaces = globalWidth - info.first.size() - 8;
         out_stream() << "    =" << info.first;
         out_stream().indent(NumSpaces) << " -   " << info.second << '\n';
      }
   }
};
} // namespace

// Create our command line option.
static DebugCounterList sg_debugCounterOption(
      "debug-counter",
      cmd::Desc("Comma separated list of debug counter skip and count"),
      cmd::CommaSeparated, cmd::ZeroOrMore, cmd::location(DebugCounter::getInstance()));


static polar::cmd::Opt<bool> sg_printDebugCounter(
      "print-debug-counter", polar::cmd::Hidden, polar::cmd::init(false), polar::cmd::Optional,
      polar::cmd::Desc("Print out debug counter info after all counters accumulated"));

static ManagedStatic<DebugCounter> sg_debugCounter;

// Print information when destroyed, iff command line option is specified.
DebugCounter::~DebugCounter()
{
   if (isCountingEnabled() && sg_printDebugCounter) {
      print(debug_stream());
   }
}

DebugCounter &DebugCounter::getInstance()
{
   return *sg_debugCounter;
}

// This is called by the command line parser when it sees a value for the
// debug-counter option defined above.
void DebugCounter::pushBack(const std::string &value)
{
   if (value.empty()) {
      return;
   }
   // The strings should come in as counter=value
   auto counterPair = StringRef(value).split('=');
   if (counterPair.second.empty()) {
      error_stream() << "DebugCounter Error: " << value << " does not have an = in it\n";
      return;
   }
   // Now we have counter=value.
   // First, process value.
   int64_t counterVal;
   if (counterPair.second.getAsInteger(0, counterVal)) {
      error_stream() << "DebugCounter Error: " << counterPair.second
                     << " is not a number\n";
      return;
   }
   // Now we need to see if this is the skip or the count, remove the suffix, and
   // add it to the counter values.
   if (counterPair.first.endsWith("-skip")) {
      auto counterName = counterPair.first.dropBack(5);
      unsigned counterID = getCounterId(counterName);
      if (!counterID) {
         error_stream() << "DebugCounter Error: " << counterName
                        << " is not a registered counter\n";
         return;
      }

      CounterInfo &counter = m_counters[counterID];
      counter.Skip = counterVal;
      counter.IsSet = true;

   } else if (counterPair.first.endsWith("-count")) {
      auto counterName = counterPair.first.dropBack(6);
      unsigned counterID = getCounterId(counterName);
      if (!counterID) {
         error_stream() << "DebugCounter Error: " << counterName
                        << " is not a registered counter\n";
         return;
      }

      enableAllCounters();

      CounterInfo &counter = m_counters[counterID];
      counter.StopAfter = counterVal;
      counter.IsSet = true;
   } else {
      error_stream() << "DebugCounter Error: " << counterPair.first
                     << " does not end with -skip or -count\n";
   }
}

void DebugCounter::print(RawOutStream &outstream) const
{
   SmallVector<StringRef, 16> counterNames(m_registeredCounters.begin(),
                                           m_registeredCounters.end());
   sort(counterNames.begin(), counterNames.end());

   auto &us = getInstance();
   outstream << "Counters and values:\n";
   for (auto &CounterName : counterNames) {
      unsigned counterID = getCounterId(CounterName);
      outstream << left_justify(m_registeredCounters[counterID], 32) << ": {"
                << us.m_counters[counterID].Count << "," << us.m_counters[counterID].Skip
                << "," << us.m_counters[counterID].StopAfter << "}\n";
   }
}

POLAR_DUMP_METHOD void DebugCounter::dump() const
{
   print(debug_stream());
}

} // utils
} // polar
