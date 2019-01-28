// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/12.

//===----------------------------------------------------------------------===//
//
// This class implements a command line argument processor that is useful when
// creating a tool.  It provides a simple, minimalistic interface that is easily
// extensible and supports nonlocal (library) command line options.
//
// Note that rather than trying to figure out what this code does, you could try
// reading the library documentation located in docs/CommandLine.html
//
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/utils/ConvertUtf.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/StringSaver.h"
#include "polarphp/utils/RawOutStream.h"
#include <cstdlib>
#include <map>

namespace polar {
namespace cmd {

using polar::basic::Triple;

#define DEBUG_TYPE "commandline"

template class BasicParser<bool>;
template class BasicParser<BoolOrDefault>;
template class BasicParser<int>;
template class BasicParser<unsigned>;
template class BasicParser<unsigned long long>;
template class BasicParser<double>;
template class BasicParser<float>;
template class BasicParser<std::string>;
template class BasicParser<char>;

template class Opt<unsigned>;
template class Opt<int>;
template class Opt<std::string>;
template class Opt<char>;
template class Opt<bool>;

using polar::basic::IteratorRange;
using polar::basic::SmallString;
using polar::basic::SmallPtrSetImpl;
using polar::basic::array_pod_sort;

using polar::utils::error_stream;
using polar::debug_stream;
using polar::utils::out_stream;
using polar::utils::report_fatal_error;
using polar::utils::has_utf16_byte_order_mark;
using polar::utils::convert_utf16_to_utf8_string;
using polar::utils::MemoryBuffer;
using polar::utils::OptionalError;
using polar::utils::BumpPtrAllocator;
using polar::utils::RawOutStream;
using polar::utils::RawStringOutStream;


// Pin the vtables to this file.
void GenericOptionValue::anchor()
{}

void OptionValue<BoolOrDefault>::anchor()
{}

void OptionValue<std::string>::anchor()
{}

void Option::anchor()
{}

void BasicParserImpl::anchor()
{}

void Parser<bool>::anchor()
{}

void Parser<BoolOrDefault>::anchor()
{}

void Parser<int>::anchor()
{}

void Parser<unsigned>::anchor()
{}

void Parser<unsigned long long>::anchor()
{}

void Parser<double>::anchor()
{}

void Parser<float>::anchor()
{}

void Parser<std::string>::anchor()
{}

void Parser<char>::anchor()
{}

//===----------------------------------------------------------------------===//

namespace {

class CommandLineParser
{
public:
   // Globals for name and overview of program.  Program name is not a string to
   // avoid static ctor/dtor issues.
   std::string m_programName;
   StringRef m_programOverview;

   // This collects additional help to be printed.
   std::vector<StringRef> m_moreHelp;

   // This collects the different option categories that have been registered.
   SmallPtrSet<OptionCategory *, 16> m_registeredOptionCategories;

   // This collects the different subcommands that have been registered.
   SmallPtrSet<SubCommand *, 4> m_registeredSubCommands;

   CommandLineParser() : m_activeSubCommand(nullptr)
   {
      registerSubCommand(&*sg_topLevelSubCommand);
      registerSubCommand(&*sg_allSubCommands);
   }

   void resetAllOptionOccurrences();

   bool parseCommandLineOptions(int argc, const char *const *argv,
                                StringRef overview, RawOutStream *errorStream = nullptr);

   void addLiteralOption(Option &option, SubCommand *subcommand, StringRef name)
   {
      if (option.hasArgStr()) {
         return;
      }
      if (!subcommand->m_optionsMap.insert(std::make_pair(name, &option)).second) {
         error_stream() << m_programName << ": CommandLine Error: Option '" << name
                        << "' registered more than once!\n";
         report_fatal_error("inconsistency in registered CommandLine options");
      }

      // If we're adding this to all sub-commands, add it to the ones that have
      // already been registered.
      if (subcommand == &*sg_allSubCommands) {
         for (const auto &sub : m_registeredSubCommands) {
            if (subcommand == sub) {
               continue;
            }
            addLiteralOption(option, sub, name);
         }
      }
   }

   void addLiteralOption(Option &option, StringRef name) {
      if (option.m_subs.empty()) {
         addLiteralOption(option, &*sg_topLevelSubCommand, name);
      } else {
         for (auto subcommand : option.m_subs) {
            addLiteralOption(option, subcommand, name);
         }
      }
   }

   void addOption(Option *option, SubCommand *subcommand)
   {
      bool hadErrors = false;
      if (option->hasArgStr()) {
         // Add argument to the argument map!
         if (!subcommand->m_optionsMap.insert(std::make_pair(option->m_argStr, option)).second) {
            error_stream() << m_programName << ": CommandLine Error: Option '" << option->m_argStr
                           << "' registered more than once!\n";
            hadErrors = true;
         }
      }

      // Remember information about positional options.
      if (option->getFormattingFlag() == cmd::Positional) {
         subcommand->m_positionalOpts.push_back(option);
      } else if (option->getMiscFlags() & cmd::Sink) {
         // Remember sink options
         subcommand->m_sinkOpts.push_back(option);
      } else if (option->getNumOccurrencesFlag() == cmd::ConsumeAfter) {
         if (subcommand->m_consumeAfterOpt) {
            option->error("Cannot specify more than one option with cl::ConsumeAfter!");
            hadErrors = true;
         }
         subcommand->m_consumeAfterOpt = option;
      }

      // Fail hard if there were errors. These are strictly unrecoverable and
      // indicate serious issues such as conflicting option names or an
      // incorrectly
      // linked polarphp distribution.
      if (hadErrors) {
         report_fatal_error("inconsistency in registered CommandLine options");
      }

      // If we're adding this to all sub-commands, add it to the ones that have
      // already been registered.
      if (subcommand == &*sg_allSubCommands) {
         for (const auto &sub : m_registeredSubCommands) {
            if (subcommand == sub) {
               continue;
            }
            addOption(option, sub);
         }
      }
   }

   void addOption(Option *option)
   {
      if (option->m_subs.empty()) {
         addOption(option, &*sg_topLevelSubCommand);
      } else {
         for (auto SubCommand : option->m_subs) {
            addOption(option, SubCommand);
         }
      }
   }

   void removeOption(Option *option, SubCommand *subcommand)
   {
      SmallVector<StringRef, 16> optionNames;
      option->getExtraOptionNames(optionNames);
      if (option->hasArgStr()) {
         optionNames.push_back(option->m_argStr);
      }
      SubCommand &sub = *subcommand;
      for (auto name : optionNames) {
         sub.m_optionsMap.erase(name);
      }
      if (option->getFormattingFlag() == cmd::Positional)
         for (auto opt = sub.m_positionalOpts.begin();
              opt != sub.m_positionalOpts.end(); ++opt) {
            if (*opt == option) {
               sub.m_positionalOpts.erase(opt);
               break;
            }
         }
      else if (option->getMiscFlags() & cmd::Sink)
         for (auto opt = sub.m_sinkOpts.begin(); opt != sub.m_sinkOpts.end(); ++opt) {
            if (*opt == option) {
               sub.m_sinkOpts.erase(opt);
               break;
            }
         }
      else if (option == sub.m_consumeAfterOpt) {
         sub.m_consumeAfterOpt = nullptr;
      }
   }

   void removeOption(Option *option)
   {
      if (option->m_subs.empty()) {
         removeOption(option, &*sg_topLevelSubCommand);
      } else {
         if (option->isInAllSubCommands()) {
            for (auto subcommand : m_registeredSubCommands) {
               removeOption(option, subcommand);
            }
         } else {
            for (auto subcommand : option->m_subs) {
               removeOption(option, subcommand);
            }
         }
      }
   }

   bool hasOptions(const SubCommand &subcommand) const
   {
      return (!subcommand.m_optionsMap.empty() || !subcommand.m_positionalOpts.empty() ||
              nullptr != subcommand.m_consumeAfterOpt);
   }

   bool hasOptions() const
   {
      for (const auto &subcommand : m_registeredSubCommands) {
         if (hasOptions(*subcommand)) {
            return true;
         }
      }
      return false;
   }

   SubCommand *getActiveSubCommand()
   {
      return m_activeSubCommand;
   }

   void updateArgStr(Option *option, StringRef newName, SubCommand *subcommand)
   {
      SubCommand &sub = *subcommand;
      if (!sub.m_optionsMap.insert(std::make_pair(newName, option)).second) {
         error_stream() << m_programName << ": CommandLine Error: Option '" << option->m_argStr
                        << "' registered more than once!\n";
         report_fatal_error("inconsistency in registered CommandLine options");
      }
      sub.m_optionsMap.erase(option->m_argStr);
   }

   void updateArgStr(Option *option, StringRef newName)
   {
      if (option->m_subs.empty()) {
         updateArgStr(option, newName, &*sg_topLevelSubCommand);
      } else {
         for (auto subcommand : option->m_subs) {
            updateArgStr(option, newName, subcommand);
         }
      }
   }

   void printOptionValues();

   void registerCategory(OptionCategory *category)
   {
      assert(count_if(m_registeredOptionCategories,
                      [category](const OptionCategory *optionCategory) {
         return category->getName() == optionCategory->getName();
      }) == 0 &&
             "Duplicate option categories");

      m_registeredOptionCategories.insert(category);
   }

   void registerSubCommand(SubCommand *subcommand)
   {
      assert(count_if(m_registeredSubCommands,
                      [subcommand](const SubCommand *item) {
         return (!subcommand->getName().empty()) &&
               (item->getName() == subcommand->getName());
      }) == 0 &&
             "Duplicate subcommands");
      m_registeredSubCommands.insert(subcommand);

      // For all options that have been registered for all subcommands, add the
      // option to this subcommand now.
      if (subcommand != &*sg_allSubCommands) {
         for (auto &elem : sg_allSubCommands->m_optionsMap) {
            Option *option = elem.m_second;
            if ((option->isPositional() || option->isSink() || option->isConsumeAfter()) ||
                option->hasArgStr()) {
               addOption(option, subcommand);
            } else {
               addLiteralOption(*option, subcommand, elem.getFirst());
            }
         }
      }
   }

   void unregisterSubCommand(SubCommand *subcommand)
   {
      m_registeredSubCommands.erase(subcommand);
   }

   IteratorRange<typename SmallPtrSet<SubCommand *, 4>::iterator>
   getRegisteredSubCommands()
   {
      return polar::basic::make_range(m_registeredSubCommands.begin(),
                                      m_registeredSubCommands.end());
   }

   void reset()
   {
      m_activeSubCommand = nullptr;
      m_programName.clear();
      m_programOverview = StringRef();

      m_moreHelp.clear();
      m_registeredOptionCategories.clear();

      resetAllOptionOccurrences();
      m_registeredSubCommands.clear();

      sg_topLevelSubCommand->reset();
      sg_allSubCommands->reset();
      registerSubCommand(&*sg_topLevelSubCommand);
      registerSubCommand(&*sg_allSubCommands);
   }

private:
   SubCommand *m_activeSubCommand;

   Option *lookupOption(SubCommand &subcommmand, StringRef &arg, StringRef &value);
   SubCommand *lookupSubCommand(StringRef name);
};

} // namespace

static ManagedStatic<CommandLineParser> sg_globalParser;

void add_literal_option(Option &option, StringRef name)
{
   sg_globalParser->addLiteralOption(option, name);
}

ExtraHelp::ExtraHelp(StringRef help)
   : m_moreHelp(help)
{
   sg_globalParser->m_moreHelp.push_back(help);
}

void Option::addArgument()
{
   sg_globalParser->addOption(this);
   m_fullyInitialized = true;
}

void Option::removeArgument()
{
   sg_globalParser->removeOption(this);
}

void Option::setArgStr(StringRef str)
{
   if (m_fullyInitialized) {
      sg_globalParser->updateArgStr(this, str);
   }
   assert((str.empty() || str[0] != '-') && "Option can't start with '-");
   m_argStr = str;
}

// Initialise the general option category.
OptionCategory sg_generalCategory("General options");

void OptionCategory::getRegisterCategory()
{
   sg_globalParser->registerCategory(this);
}

// A special subcommand representing no subcommand
ManagedStatic<SubCommand> sg_topLevelSubCommand;

// A special subcommand that can be used to put an option into all subcommands.
ManagedStatic<SubCommand> sg_allSubCommands;

void SubCommand::registerSubCommand()
{
   sg_globalParser->registerSubCommand(this);
}

void SubCommand::unregisterSubCommand()
{
   sg_globalParser->unregisterSubCommand(this);
}

void SubCommand::reset()
{
   m_positionalOpts.clear();
   m_sinkOpts.clear();
   m_optionsMap.clear();
   m_consumeAfterOpt = nullptr;
}

SubCommand::operator bool() const
{
   return (sg_globalParser->getActiveSubCommand() == this);
}

//===----------------------------------------------------------------------===//
// Basic, shared command line option processing machinery.
//

/// LookupOption - Lookup the option specified by the specified option on the
/// command line.  If there is a value specified (after an equal sign) return
/// that as well.  This assumes that leading dashes have already been stripped.
Option *CommandLineParser::lookupOption(SubCommand &subcommand, StringRef &arg,
                                        StringRef &value)
{
   // Reject all dashes.
   if (arg.empty()) {
      return nullptr;
   }
   assert(&subcommand != &*sg_allSubCommands);
   size_t equalPos = arg.find('=');
   // If we have an equals sign, remember the value.
   if (equalPos == StringRef::npos) {
      // Look up the option.
      auto inter = subcommand.m_optionsMap.find(arg);
      if (inter == subcommand.m_optionsMap.end()) {
         return nullptr;
      }
      return inter != subcommand.m_optionsMap.end() ? inter->m_second : nullptr;
   }

   // If the argument before the = is a valid option name, we match.  If not,
   // return Arg unmolested.
   auto inter = subcommand.m_optionsMap.find(arg.substr(0, equalPos));
   if (inter == subcommand.m_optionsMap.end()) {
      return nullptr;
   }
   value = arg.substr(equalPos + 1);
   arg = arg.substr(0, equalPos);
   return inter->m_second;
}

SubCommand *CommandLineParser::lookupSubCommand(StringRef name)
{
   if (name.empty()) {
      return &*sg_topLevelSubCommand;
   }
   for (auto subcommand : m_registeredSubCommands) {
      if (subcommand == &*sg_allSubCommands) {
         continue;
      }
      if (subcommand->getName().empty()) {
         continue;
      }
      if (StringRef(subcommand->getName()) == StringRef(name)) {
         return subcommand;
      }
   }
   return &*sg_topLevelSubCommand;
}

namespace {

/// LookupNearestOption - Lookup the closest match to the option specified by
/// the specified option on the command line.  If there is a value specified
/// (after an equal sign) return that as well.  This assumes that leading dashes
/// have already been stripped.
Option *lookup_nearest_option(StringRef arg,
                              const StringMap<Option *> &optionsMap,
                              std::string &nearestString)
{
   // Reject all dashes.
   if (arg.empty()) {
      return nullptr;
   }
   // Split on any equal sign.
   std::pair<StringRef, StringRef> splitArg = arg.split('=');
   StringRef &lhs = splitArg.first; // LHS == Arg when no '=' is present.
   StringRef &rhs = splitArg.second;

   // Find the closest match.
   Option *best = nullptr;
   unsigned bestDistance = 0;
   for (StringMap<Option *>::const_iterator iter = optionsMap.begin(),
        iterEnd = optionsMap.end();
        iter != iterEnd; ++iter)
   {
      Option *option = iter->m_second;
      SmallVector<StringRef, 16> optionNames;
      option->getExtraOptionNames(optionNames);
      if (option->hasArgStr()) {
         optionNames.push_back(option->m_argStr);
      }
      bool permitValue = option->getValueExpectedFlag() != cmd::ValueDisallowed;
      StringRef flag = permitValue ? lhs : arg;
      for (auto name : optionNames) {
         unsigned distance = StringRef(name).editDistance(
                  flag, /*AllowReplacements=*/true, /*MaxEditDistance=*/bestDistance);
         if (!best || distance < bestDistance) {
            best = option;
            bestDistance = distance;
            if (rhs.empty() || !permitValue) {
               nearestString = name;
            } else {
               nearestString = (Twine(name) + "=" + rhs).getStr();
            }
         }
      }
   }

   return best;
}

/// comma_separate_and_add_occurrence - A wrapper around Handler->addOccurrence()
/// that does special handling of cl::CommaSeparated options.
bool comma_separate_and_add_occurrence(Option *handler, unsigned pos,
                                       StringRef argName, StringRef value,
                                       bool multiArg = false)
{
   // Check to see if this option accepts a comma separated list of values.  If
   // it does, we have to split up the value into multiple values.
   if (handler->getMiscFlags() & CommaSeparated) {
      StringRef val(value);
      StringRef::size_type pos = val.find(',');

      while (pos != StringRef::npos) {
         // Process the portion before the comma.
         if (handler->addOccurrence(pos, argName, val.substr(0, pos), multiArg)) {
            return true;
         }
         // Erase the portion before the comma, AND the comma.
         val = val.substr(pos + 1);
         // Check for another comma.
         pos = val.find(',');
      }
      value = val;
   }

   return handler->addOccurrence(pos, argName, value, multiArg);
}

/// provide_option - For Value, this differentiates between an empty value ("")
/// and a null value (StringRef()).  The later is accepted for arguments that
/// don't allow a value (-foo) the former is rejected (-foo=).
inline bool provide_option(Option *handler, StringRef argName,
                           StringRef value, int argc,
                           const char *const *argv, int &i)
{
   // Is this a multi-argument option?
   unsigned numAdditionalVals = handler->getNumAdditionalVals();

   // Enforce value requirements
   switch (handler->getValueExpectedFlag()) {
   case ValueRequired:
      if (!value.getData()) { // No value specified?
         if (i + 1 >= argc) {
            return handler->error("requires a value!");
         }
         // Steal the next argument, like for '-o filename'
         assert(argv && "null check");
         value = StringRef(argv[++i]);
      }
      break;
   case ValueDisallowed:
      if (numAdditionalVals > 0) {
         return handler->error("multi-valued option specified"
                               " with ValueDisallowed modifier!");
      }
      if (value.getData()){
         return handler->error("does not allow a value! '" + Twine(value) +
                               "' specified.");
      }
      break;
   case ValueOptional:
      break;
   }

   // If this isn't a multi-arg option, just run the handler.
   if (numAdditionalVals == 0) {
      return comma_separate_and_add_occurrence(handler, i, argName, value);
   }
   // If it is, run the handle several times.
   bool multiArg = false;

   if (value.getData()) {
      if (comma_separate_and_add_occurrence(handler, i, argName, value, multiArg)) {
         return true;
      }
      --numAdditionalVals;
      multiArg = true;
   }

   while (numAdditionalVals > 0) {
      if (i + 1 >= argc) {
         return handler->error("not enough values!");
      }
      assert(argv && "null check");
      value = StringRef(argv[++i]);

      if (comma_separate_and_add_occurrence(handler, i, argName, value, multiArg)) {
         return true;
      }
      multiArg = true;
      --numAdditionalVals;
   }
   return false;
}

bool provide_positional_option(Option *handler, StringRef arg, int i)
{
   int dummy = i;
   return provide_option(handler, handler->m_argStr, arg, 0, nullptr, dummy);
}

// Option predicates...
inline bool is_grouping(const Option *option)
{
   return option->getFormattingFlag() == cmd::Grouping;
}

inline bool is_prefixed_or_grouping(const Option *option)
{
   return is_grouping(option) || option->getFormattingFlag() == cmd::Prefix;
}

// get_option_pred - Check to see if there are any options that satisfy the
// specified predicate with names that are the prefixes in Name.  This is
// checked by progressively stripping characters off of the name, checking to
// see if there options that satisfy the predicate.  If we find one, return it,
// otherwise return null.
//
Option *get_option_pred(StringRef name, size_t &length,
                        bool (*pred)(const Option *),
                        const StringMap<Option *> &optionsMap)
{

   StringMap<Option *>::const_iterator omi = optionsMap.find(name);

   // Loop while we haven't found an option and Name still has at least two
   // characters in it (so that the next iteration will not be the empty
   // string.
   while (omi == optionsMap.end() && name.getSize() > 1) {
      name = name.substr(0, name.getSize() - 1); // Chop off the last character.
      omi = optionsMap.find(name);
   }

   if (omi != optionsMap.end() && pred(omi->m_second)) {
      length = name.getSize();
      return omi->m_second; // Found one!
   }
   return nullptr; // No option found!
}

/// handle_prefixed_or_grouped_option - The specified argument string (which started
/// with at least one '-') does not fully match an available option.  Check to
/// see if this is a prefix or grouped option.  If so, split arg into output an
/// Arg/Value pair and return the Option to parse it with.
Option *handle_prefixed_or_grouped_option(StringRef &arg, StringRef &value,
                                          bool &errorParsing,
                                          const StringMap<Option *> &optionsMap)
{
   if (arg.getSize() == 1) {
      return nullptr;
   }
   // Do the lookup!
   size_t length = 0;
   Option *pgOpt = get_option_pred(arg, length, is_prefixed_or_grouping, optionsMap);
   if (!pgOpt) {
      return nullptr;
   }
   // If the option is a prefixed option, then the value is simply the
   // rest of the name...  so fall through to later processing, by
   // setting up the argument name flags and value fields.
   if (pgOpt->getFormattingFlag() == cmd::Prefix) {
      value = arg.substr(length);
      arg = arg.substr(0, length);
      assert(optionsMap.count(arg) && optionsMap.find(arg)->m_second == pgOpt);
      return pgOpt;
   }

   // This must be a grouped option... handle them now.  Grouping options can't
   // have values.
   assert(is_grouping(pgOpt) && "Broken get_option_pred!");

   do {
      // Move current arg name out of Arg into OneArgName.
      StringRef oneArgName = arg.substr(0, length);
      arg = arg.substr(length);

      // Because ValueRequired is an invalid flag for grouped arguments,
      // we don't need to pass argc/argv in.
      assert(pgOpt->getValueExpectedFlag() != cmd::ValueRequired &&
            "Option can not be cl::Grouping AND cl::ValueRequired!");
      int dummy = 0;
      errorParsing |=
            provide_option(pgOpt, oneArgName, StringRef(), 0, nullptr, dummy);

      // Get the next grouping option.
      pgOpt = get_option_pred(arg, length, is_grouping, optionsMap);
   } while (pgOpt && length != arg.getSize());

   // Return the last option with Arg cut down to just the last one.
   return pgOpt;
}

bool requires_value(const Option *option)
{
   return option->getNumOccurrencesFlag() == cmd::Required ||
         option->getNumOccurrencesFlag() == cmd::OneOrMore;
}

bool eats_unbounded_number_of_values(const Option *option)
{
   return option->getNumOccurrencesFlag() == cmd::ZeroOrMore ||
         option->getNumOccurrencesFlag() == cmd::OneOrMore;
}

bool is_whitespace(char c)
{
   return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool is_whitespace_or_null(char c)
{
   return is_whitespace(c) || c == '\0';
}

bool is_quote(char c)
{
   return c == '\"' || c == '\'';
}

/// Backslashes are interpreted in a rather complicated way in the Windows-style
/// command line, because backslashes are used both to separate path and to
/// escape double quote. This method consumes runs of backslashes as well as the
/// following double quote if it's escaped.
///
///  * If an even number of backslashes is followed by a double quote, one
///    backslash is output for every pair of backslashes, and the last double
///    quote remains unconsumed. The double quote will later be interpreted as
///    the start or end of a quoted string in the main loop outside of this
///    function.
///
///  * If an odd number of backslashes is followed by a double quote, one
///    backslash is output for every pair of backslashes, and a double quote is
///    output for the last pair of backslash-double quote. The double quote is
///    consumed in this case.
///
///  * Otherwise, backslashes are interpreted literally.
size_t parse_backslash(StringRef src, size_t index, SmallString<128> &token)
{
   size_t end = src.getSize();
   int backslashCount = 0;
   // Skip the backslashes.
   do {
      ++index;
      ++backslashCount;
   } while (index != end && src[index] == '\\');

   bool followedByDoubleQuote = (index != end && src[index] == '"');
   if (followedByDoubleQuote) {
      token.append(backslashCount / 2, '\\');
      if (backslashCount % 2 == 0) {
         return index - 1;
      }
      token.push_back('"');
      return index;
   }
   token.append(backslashCount, '\\');
   return index - 1;
}

// It is called byte order marker but the UTF-8 BOM is actually not affected
// by the host system's endianness.
bool has_utf8_byte_order_mark(ArrayRef<char> str)
{
   return (str.getSize() >= 3 && str[0] == '\xef' && str[1] == '\xbb' && str[2] == '\xbf');
}

bool expand_response_file(StringRef fname, StringSaver &saver,
                          TokenizerCallback tokenizer,
                          SmallVectorImpl<const char *> &newArgv,
                          bool markEOLs, bool relativeNames)
{
   OptionalError<std::unique_ptr<MemoryBuffer>> memBufOrErr =
         MemoryBuffer::getFile(fname);
   if (!memBufOrErr) {
      return false;
   }
   MemoryBuffer &memBuf = *memBufOrErr.get();
   StringRef str(memBuf.getBufferStart(), memBuf.getBufferSize());

   // If we have a UTF-16 byte order mark, convert to UTF-8 for parsing.
   ArrayRef<char> bufRef(memBuf.getBufferStart(), memBuf.getBufferEnd());
   std::string utf8Buf;
   if (has_utf16_byte_order_mark(bufRef)) {
      if (!convert_utf16_to_utf8_string(bufRef, utf8Buf)) {
         return false;
      }
      str = StringRef(utf8Buf);
   }
   // If we see UTF-8 BOM sequence at the beginning of a file, we shall remove
   // these bytes before parsing.
   // Reference: http://en.wikipedia.org/wiki/UTF-8#Byte_order_mark
   else if (has_utf8_byte_order_mark(bufRef)) {
      str = StringRef(bufRef.getData() + 3, bufRef.getSize() - 3);
   }


   // Tokenize the contents into newArgv.
   tokenizer(str, saver, newArgv, markEOLs);

   // If names of nested response files should be resolved relative to including
   // file, replace the included response file names with their full paths
   // obtained by required resolution.
   if (relativeNames) {
      for (unsigned index = 0; index < newArgv.getSize(); ++index) {
         if (newArgv[index]) {
            StringRef arg = newArgv[index];
            if (arg.front() == '@') {
               StringRef fileName = arg.dropFront();
               if (fs::path::is_relative(fileName)) {
                  SmallString<128> responseFile;
                  responseFile.append(1, '@');
                  if (fs::path::is_relative(fname)) {
                     SmallString<128> currDir;
                     fs::current_path(currDir);
                     responseFile.append(currDir.getStr());
                  }
                  fs::path::append(
                           responseFile, fs::path::parent_path(fname), fileName);
                  newArgv[index] = saver.save(responseFile.getCStr()).getData();
               }
            }
         }
      }
   }
   return true;
}

} // anonymous namespace

void tokenize_gnu_command_line(StringRef src, StringSaver &saver,
                               SmallVectorImpl<const char *> &newArgv,
                               bool markEOLs)
{
   SmallString<128> token;
   for (size_t index = 0, end = src.getSize(); index != end; ++index) {
      // Consume runs of whitespace.
      if (token.empty()) {
         while (index != end && is_whitespace(src[index])) {
            // Mark the end of lines in response files
            if (markEOLs && src[index] == '\n') {
               newArgv.push_back(nullptr);
            }

            ++index;
         }
         if (index == end) {
            break;
         }
      }

      char c = src[index];

      // Backslash escapes the next character.
      if (index + 1 < end && c == '\\') {
         ++index; // Skip the escape.
         token.push_back(src[index]);
         continue;
      }

      // Consume a quoted string.
      if (is_quote(c)) {
         ++index;
         while (index != end && src[index] != c) {
            // Backslash escapes the next character.
            if (src[index] == '\\' && index + 1 != end) {
               ++index;
            }
            token.push_back(src[index]);
            ++index;
         }
         if (index == end) {
            break;
         }
         continue;
      }

      // End the token if this is whitespace.
      if (is_whitespace(c)) {
         if (!token.empty()) {
            newArgv.push_back(saver.save(StringRef(token)).getData());
         }
         token.clear();
         continue;
      }
      // This is a normal character.  Append it.
      token.push_back(c);
   }

   // Append the last token after hitting EOF with no whitespace.
   if (!token.empty()) {
      newArgv.push_back(saver.save(StringRef(token)).getData());
   }
   // Mark the end of response files
   if (markEOLs) {
      newArgv.push_back(nullptr);
   }
}

void tokenize_windows_command_line(StringRef src, StringSaver &saver,
                                   SmallVectorImpl<const char *> &newArgv,
                                   bool markEOLs)
{
   SmallString<128> token;

   // This is a small state machine to consume characters until it reaches the
   // end of the source string.
   enum { INIT, UNQUOTED, QUOTED } state = INIT;
   for (size_t index = 0, end = src.getSize(); index != end; ++index) {
      char c = src[index];

      // INIT state indicates that the current input index is at the start of
      // the string or between tokens.
      if (state == INIT) {
         if (is_whitespace_or_null(c)) {
            // Mark the end of lines in response files
            if (markEOLs && c == '\n') {
               newArgv.push_back(nullptr);
            }
            continue;
         }
         if (c == '"') {
            state = QUOTED;
            continue;
         }
         if (c == '\\') {
            index = parse_backslash(src, index, token);
            state = UNQUOTED;
            continue;
         }
         token.push_back(c);
         state = UNQUOTED;
         continue;
      }

      // UNQUOTED state means that it's reading a token not quoted by double
      // quotes.
      if (state == UNQUOTED) {
         // Whitespace means the end of the token.
         if (is_whitespace_or_null(c)) {
            newArgv.push_back(saver.save(StringRef(token)).getData());
            token.clear();
            state = INIT;
            // Mark the end of lines in response files
            if (markEOLs && c == '\n') {
               newArgv.push_back(nullptr);
            }
            continue;
         }
         if (c == '"') {
            state = QUOTED;
            continue;
         }
         if (c == '\\') {
            index = parse_backslash(src, index, token);
            continue;
         }
         token.push_back(c);
         continue;
      }

      // QUOTED state means that it's reading a token quoted by double quotes.
      if (state == QUOTED) {
         if (c == '"') {
            state = UNQUOTED;
            continue;
         }
         if (c == '\\') {
            index = parse_backslash(src, index, token);
            continue;
         }
         token.push_back(c);
      }
   }
   // Append the last token after hitting EOF with no whitespace.
   if (!token.empty()) {
      newArgv.push_back(saver.save(StringRef(token)).getData());
   }
   // Mark the end of response files
   if (markEOLs) {
      newArgv.push_back(nullptr);
   }
}

void tokenize_config_file(StringRef source, StringSaver &saver,
                          SmallVectorImpl<const char *> &newArgv,
                          bool markEOLs)
{
   for (const char *cur = source.begin(); cur != source.end();) {
      SmallString<128> line;
      // Check for comment line.
      if (is_whitespace(*cur)) {
         while (cur != source.end() && is_whitespace(*cur)) {
            ++cur;
         }
         continue;
      }
      if (*cur == '#') {
         while (cur != source.end() && *cur != '\n') {
            ++cur;
         }
         continue;
      }
      // Find end of the current line.
      const char *start = cur;
      for (const char *end = source.end(); cur != end; ++cur) {
         if (*cur == '\\') {
            if (cur + 1 != end) {
               ++cur;
               if (*cur == '\n' ||
                   (*cur == '\r' && (cur + 1 != end) && cur[1] == '\n')) {
                  line.append(start, cur - 1);
                  if (*cur == '\r') {
                     ++cur;
                  }
                  start = cur + 1;
               }
            }
         } else if (*cur == '\n')
            break;
      }
      // Tokenize line.
      line.append(start, cur);
      tokenize_gnu_command_line(line, saver, newArgv, markEOLs);
   }
}

/// Expand response files on a command line recursively using the given
/// StringSaver and tokenization strategy.
bool expand_response_files(StringSaver &saver, TokenizerCallback tokenizer,
                           SmallVectorImpl<const char *> &argv,
                           bool markEOLs, bool relativeNames)
{
   unsigned rspFiles = 0;
   bool allExpanded = true;

   // Don't cache Argv.size() because it can change.
   for (unsigned index = 0; index != argv.getSize();) {
      const char *arg = argv[index];
      // Check if it is an EOL marker
      if (arg == nullptr) {
         ++index;
         continue;
      }
      if (arg[0] != '@') {
         ++index;
         continue;
      }

      // If we have too many response files, leave some unexpanded.  This avoids
      // crashing on self-referential response files.
      if (rspFiles++ > 20) {
         return false;
      }
      // Replace this response file argument with the tokenization of its
      // contents.  Nested response files are expanded in subsequent iterations.
      SmallVector<const char *, 0> expandedArgv;
      if (!expand_response_file(arg + 1, saver, tokenizer, expandedArgv,
                                markEOLs, relativeNames)) {
         // We couldn't read this file, so we leave it in the argument stream and
         // move on.
         allExpanded = false;
         ++index;
         continue;
      }
      argv.erase(argv.begin() + index);
      argv.insert(argv.begin() + index, expandedArgv.begin(), expandedArgv.end());
   }
   return allExpanded;
}

bool read_config_file(StringRef cfgFile, StringSaver &saver,
                      SmallVectorImpl<const char *> &argv)
{
   if (!expand_response_file(cfgFile, saver, tokenize_config_file, argv,
                             /*MarkEOLs*/ false, /*RelativeNames*/ true)) {
      return false;
   }
   return expand_response_files(saver, tokenize_config_file, argv,
                                /*MarkEOLs*/ false, /*RelativeNames*/ true);
}

/// parse_environment_options - An alternative entry point to the
/// CommandLine library, which allows you to read the program's name
/// from the caller (as PROGNAME) and its command-line arguments from
/// an environment variable (whose name is given in ENVVAR).
///
void parse_environment_options(const char *progName, const char *envVar,
                               const char *overview)
{
   // Check args.
   assert(progName && "Program name not specified");
   assert(envVar && "Environment variable name missing");

   // Get the environment variable they want us to parse options out of.
   std::optional<std::string> envValue = sys::Process::getEnv(StringRef(envVar));
   if (!envValue) {
      return;
   }
   // Get program's "name", which we wouldn't know without the caller
   // telling us.
   SmallVector<const char *, 20> newArgv;
   BumpPtrAllocator alloc;
   StringSaver saver(alloc);
   newArgv.push_back(saver.save(progName).getData());

   // Parse the value of the environment variable into a "command line"
   // and hand it off to parseCommandLineOptions().
   tokenize_gnu_command_line(*envValue, saver, newArgv);
   int newArgc = static_cast<int>(newArgv.getSize());
   parse_commandline_options(newArgc, &newArgv[0], StringRef(overview));
}

bool parse_commandline_options(int argc, const char *const *argv,
                               StringRef overview, RawOutStream *errorStream,
                               const char *envVar)
{
   SmallVector<const char *, 20> newArgv;
   BumpPtrAllocator allocator;
   StringSaver saver(allocator);
   newArgv.push_back(argv[0]);

   // Parse options from environment variable.
   if (envVar) {
      if (std::optional<std::string> envValue =
          sys::Process::getEnv(StringRef(envVar))) {
         tokenize_gnu_command_line(*envValue, saver, newArgv);
      }
   }

   // Append options from command line.
   for (int i = 1; i < argc; ++i) {
      newArgv.push_back(argv[i]);
   }
   int newArgc = static_cast<int>(newArgv.size());

   // Parse all options.
   return sg_globalParser->parseCommandLineOptions(newArgc, &newArgv[0], overview,
         errorStream);
}

void CommandLineParser::resetAllOptionOccurrences()
{
   // So that we can parse different command lines multiple times in succession
   // we reset all option values to look like they have never been seen before.
   for (auto subcommand : m_registeredSubCommands) {
      for (auto &option : subcommand->m_optionsMap) {
         option.m_second->reset();
      }
   }
}

bool CommandLineParser::parseCommandLineOptions(int argc,
                                                const char *const *argv,
                                                StringRef overview,
                                                RawOutStream *errorStream)
{
   assert(hasOptions() && "No options specified!");

   // Expand response files.
   SmallVector<const char *, 20> newArgv(argv, argv + argc);
   BumpPtrAllocator alloc;
   StringSaver saver(alloc);
   expand_response_files(saver,
                         Triple(sys::get_process_triple()).isOSWindows() ?
                            tokenize_windows_command_line : tokenize_gnu_command_line,
                         newArgv);
   argv = &newArgv[0];
   argc = static_cast<int>(newArgv.getSize());

   // Copy the program name into ProgName, making sure not to overflow it.
   m_programName = fs::path::filename(StringRef(argv[0]));

   m_programOverview = overview;
   bool ignoreErrors = errorStream;
   if (!errorStream) {
      errorStream = &error_stream();
   }
   bool errorParsing = false;

   // Check out the positional arguments to collect information about them.
   unsigned numPositionalRequired = 0;

   // Determine whether or not there are an unlimited number of positionals
   bool hasUnlimitedPositionals = false;

   int firstArg = 1;
   SubCommand *chosenSubCommand = &*sg_topLevelSubCommand;
   if (argc >= 2 && argv[firstArg][0] != '-') {
      // If the first argument specifies a valid subcommand, start processing
      // options from the second argument.
      chosenSubCommand = lookupSubCommand(StringRef(argv[firstArg]));
      if (chosenSubCommand != &*sg_topLevelSubCommand) {
         firstArg = 2;
      }
   }
   sg_globalParser->m_activeSubCommand = chosenSubCommand;

   assert(chosenSubCommand);
   auto &consumeAfterOpt = chosenSubCommand->m_consumeAfterOpt;
   auto &positionalOpts = chosenSubCommand->m_positionalOpts;
   auto &sinkOpts = chosenSubCommand->m_sinkOpts;
   auto &optionsMap = chosenSubCommand->m_optionsMap;

   if (consumeAfterOpt) {
      assert(positionalOpts.getSize() > 0 &&
             "Cannot specify cl::ConsumeAfter without a positional argument!");
   }
   if (!positionalOpts.empty()) {

      // Calculate how many positional values are _required_.
      bool unboundedFound = false;
      for (size_t i = 0, end = positionalOpts.getSize(); i != end; ++i) {
         Option *option = positionalOpts[i];
         if (requires_value(option)) {
            ++numPositionalRequired;
         } else if (consumeAfterOpt) {
            // ConsumeAfter cannot be combined with "optional" positional options
            // unless there is only one positional argument...
            if (positionalOpts.getSize() > 1) {
               if (!ignoreErrors) {
                  option->error("error - this positional option will never be matched, "
                                "because it does not Require a value, and a "
                                "cl::ConsumeAfter option is active!");
               }
               errorParsing = true;
            }
         } else if (unboundedFound && !option->hasArgStr()) {
            // This option does not "require" a value...  Make sure this option is
            // not specified after an option that eats all extra arguments, or this
            // one will never get any!
            //
            if (!ignoreErrors)
               option->error("error - option can never match, because "
                             "another positional argument will match an "
                             "unbounded number of values, and this option"
                             " does not require a value!");
            *errorStream << m_programName << ": CommandLine Error: Option '" << option->m_argStr
                         << "' is all messed up!\n";
            *errorStream << positionalOpts.getSize();
            errorParsing = true;
         }
         unboundedFound |= eats_unbounded_number_of_values(option);
      }
      hasUnlimitedPositionals = unboundedFound || consumeAfterOpt;
   }

   // PositionalVals - A vector of "positional" arguments we accumulate into
   // the process at the end.
   //
   SmallVector<std::pair<StringRef, unsigned>, 4> positionalVals;

   // If the program has named positional arguments, and the name has been run
   // across, keep track of which positional argument was named.  Otherwise put
   // the positional args into the PositionalVals list...
   Option *activePositionalArg = nullptr;

   // Loop over all of the arguments... processing them.
   bool dashDashFound = false; // Have we read '--'?
   for (int i = firstArg; i < argc; ++i) {
      Option *handler = nullptr;
      Option *nearestHandler = nullptr;
      std::string nearestHandlerString;
      StringRef value;
      StringRef argName = "";

      // Check to see if this is a positional argument.  This argument is
      // considered to be positional if it doesn't start with '-', if it is "-"
      // itself, or if we have seen "--" already.
      //
      if (argv[i][0] != '-' || argv[i][1] == 0 || dashDashFound) {
         // Positional argument!
         if (activePositionalArg) {
            provide_positional_option(activePositionalArg, StringRef(argv[i]), i);
            continue; // We are done!
         }

         if (!positionalOpts.empty()) {
            positionalVals.push_back(std::make_pair(StringRef(argv[i]), i));

            // All of the positional arguments have been fulfulled, give the rest to
            // the consume after option... if it's specified...
            //
            if (positionalVals.getSize() >= numPositionalRequired && consumeAfterOpt) {
               for (++i; i < argc; ++i) {
                  positionalVals.push_back(std::make_pair(StringRef(argv[i]), i));
               }
               break; // Handle outside of the argument processing loop...
            }

            // Delay processing positional arguments until the end...
            continue;
         }
      } else if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == 0 &&
                 !dashDashFound) {
         dashDashFound = true; // This is the mythical "--"?
         continue;             // Don't try to process it as an argument itself.
      } else if (activePositionalArg &&
                 (activePositionalArg->getMiscFlags() & PositionalEatsArgs)) {
         // If there is a positional argument eating options, check to see if this
         // option is another positional argument.  If so, treat it as an argument,
         // otherwise feed it to the eating positional.
         argName = StringRef(argv[i] + 1);
         // Eat leading dashes.
         while (!argName.empty() && argName[0] == '-')
            argName = argName.substr(1);

         handler = lookupOption(*chosenSubCommand, argName, value);
         if (!handler || handler->getFormattingFlag() != cmd::Positional) {
            provide_positional_option(activePositionalArg, StringRef(argv[i]), i);
            continue; // We are done!
         }

      } else { // We start with a '-', must be an argument.
         argName = StringRef(argv[i] + 1);
         // Eat leading dashes.
         while (!argName.empty() && argName[0] == '-') {
            argName = argName.substr(1);
         }
         handler = lookupOption(*chosenSubCommand, argName, value);

         // Check to see if this "option" is really a prefixed or grouped argument.
         if (!handler)
            handler = handle_prefixed_or_grouped_option(argName, value, errorParsing,
                                                        optionsMap);

         // Otherwise, look for the closest available option to report to the user
         // in the upcoming error.
         if (!handler && sinkOpts.empty()) {
            nearestHandler =
                  lookup_nearest_option(argName, optionsMap, nearestHandlerString);
         }
      }

      if (!handler) {
         if (sinkOpts.empty()) {
            *errorStream << m_programName << ": Unknown command line argument '" << argv[i]
                            << "'.  Try: '" << argv[0] << " -help'\n";

            if (nearestHandler) {
               // If we know a near match, report it as well.
               *errorStream << m_programName << ": Did you mean '-" << nearestHandlerString
                            << "'?\n";
            }

            errorParsing = true;
         } else {
            for (SmallVectorImpl<Option *>::iterator iter = sinkOpts.begin(),
                 end = sinkOpts.end();
                 iter != end; ++iter) {
               (*iter)->addOccurrence(i, "", StringRef(argv[i]));
            }
         }
         continue;
      }

      // If this is a named positional argument, just remember that it is the
      // active one...
      if (handler->getFormattingFlag() == cmd::Positional) {
         if ((handler->getMiscFlags() & PositionalEatsArgs) && !value.empty()) {
            handler->error("This argument does not take a value.\n"
                           "\tInstead, it consumes any positional arguments until "
                           "the next recognized option.", *errorStream);
            errorParsing = true;
         }
         activePositionalArg = handler;
      }
      else {
         errorParsing |= provide_option(handler, argName, value, argc, argv, i);
      }
   }

   // Check and handle positional arguments now...
   if (numPositionalRequired > positionalVals.getSize()) {
      *errorStream << m_programName
                   << ": Not enough positional command line arguments specified!\n"
                   << "Must specify at least " << numPositionalRequired
                   << " positional argument" << (numPositionalRequired > 1 ? "s" : "")
                   << ": See: " << argv[0] << " -help\n";

      errorParsing = true;
   } else if (!hasUnlimitedPositionals &&
              positionalVals.getSize() > positionalOpts.getSize()) {
      *errorStream << m_programName << ": Too many positional arguments specified!\n"
                   << "Can specify at most " << positionalOpts.getSize()
                   << " positional arguments: See: " << argv[0] << " -help\n";
      errorParsing = true;

   } else if (!consumeAfterOpt) {
      // Positional args have already been handled if ConsumeAfter is specified.
      unsigned valNo = 0, numVals = static_cast<unsigned>(positionalVals.getSize());
      for (size_t i = 0, e = positionalOpts.getSize(); i != e; ++i) {
         if (requires_value(positionalOpts[i])) {
            provide_positional_option(positionalOpts[i], positionalVals[valNo].first,
                                      positionalVals[valNo].second);
            valNo++;
            --numPositionalRequired; // We fulfilled our duty...
         }

         // If we _can_ give this option more arguments, do so now, as long as we
         // do not give it values that others need.  'Done' controls whether the
         // option even _WANTS_ any more.
         //
         bool done = positionalOpts[i]->getNumOccurrencesFlag() == cmd::Required;
         while (numVals - valNo > numPositionalRequired && !done) {
            switch (positionalOpts[i]->getNumOccurrencesFlag()) {
            case cmd::Optional:
               done = true; // Optional arguments want _at most_ one value
               POLAR_FALLTHROUGH;
            case cmd::ZeroOrMore: // Zero or more will take all they can get...
            case cmd::OneOrMore:  // One or more will take all they can get...
               provide_positional_option(positionalOpts[i],
                                         positionalVals[valNo].first,
                                         positionalVals[valNo].second);
               valNo++;
               break;
            default:
               polar_unreachable("Internal error, unexpected NumOccurrences flag in "
                                 "positional argument processing!");
            }
         }
      }
   } else {
      assert(consumeAfterOpt && numPositionalRequired <= positionalVals.getSize());
      unsigned valNo = 0;
      for (size_t j = 1, e = positionalOpts.getSize(); j != e; ++j)
         if (requires_value(positionalOpts[j])) {
            errorParsing |= provide_positional_option(positionalOpts[j],
                                                      positionalVals[valNo].first,
                                                      positionalVals[valNo].second);
            valNo++;
         }

      // Handle the case where there is just one positional option, and it's
      // optional.  In this case, we want to give JUST THE FIRST option to the
      // positional option and keep the rest for the consume after.  The above
      // loop would have assigned no values to positional options in this case.
      //
      if (positionalOpts.getSize() == 1 && valNo == 0 && !positionalVals.empty()) {
         errorParsing |= provide_positional_option(positionalOpts[0],
               positionalVals[valNo].first,
               positionalVals[valNo].second);
         valNo++;
      }

      // Handle over all of the rest of the arguments to the
      // cl::ConsumeAfter command line option...
      for (; valNo != positionalVals.getSize(); ++valNo) {
         errorParsing |=
               provide_positional_option(consumeAfterOpt, positionalVals[valNo].first,
                                         positionalVals[valNo].second);
      }
   }

   // Loop over args and make sure all required args are specified!
   for (const auto &option : optionsMap) {
      switch (option.m_second->getNumOccurrencesFlag()) {
      case Required:
      case OneOrMore:
         if (option.m_second->getNumOccurrences() == 0) {
            option.m_second->error("must be specified at least once!");
            errorParsing = true;
         }
         POLAR_FALLTHROUGH;
      default:
         break;
      }
   }

   // Now that we know if -debug is specified, we can use it.
   // Note that if ReadResponseFiles == true, this must be done before the
   // memory allocated for the expanded command line is free()d below.
   POLAR_DEBUG(debug_stream() << "Args: ";
         for (int i = 0; i < argc; ++i) debug_stream() << argv[i] << ' ';
   debug_stream() << '\n';);

   // Free all of the memory allocated to the map.  Command line options may only
   // be processed once!
   m_moreHelp.clear();

   // If we had an error processing our arguments, don't let the program execute
   if (errorParsing) {
      if (!ignoreErrors) {
         exit(1);
      }
      return false;
   }
   return true;
}

//===----------------------------------------------------------------------===//
// Option Base class implementation
//

bool Option::error(const Twine &message, StringRef argName, RawOutStream &errorStream)
{
   if (!argName.getData()) {
      argName = m_argStr;
   }
   if (argName.empty()) {
      errorStream << m_helpStr; // Be nice for positional arguments
   } else {
      errorStream << sg_globalParser->m_programName << ": for the -" << argName;
   }
   errorStream << " option: " << message << "\n";
   return true;
}

bool Option::addOccurrence(unsigned pos, StringRef argName, StringRef value,
                           bool multiArg)
{
   if (!multiArg) {
      m_numOccurrences++; // Increment the number of times we have been seen
   }

   switch (getNumOccurrencesFlag()) {
   case Optional:
      if (m_numOccurrences > 1) {
         return error("may only occur zero or one times!", argName);
      }
      break;
   case Required:
      if (m_numOccurrences > 1) {
         return error("must occur exactly one time!", argName);
      }
      POLAR_FALLTHROUGH;
   case OneOrMore:
   case ZeroOrMore:
   case ConsumeAfter:
      break;
   }
   return handleOccurrence(pos, argName, value);
}

// get_value_str - Get the value description string, using "DefaultMsg" if nothing
// has been specified yet.
//
namespace {

StringRef get_value_str(const Option &option, StringRef defaultMsg)
{
   if (option.m_valueStr.empty()) {
      return defaultMsg;
   }
   return option.m_valueStr;
}

} // anonymous namespace

//===----------------------------------------------------------------------===//
// cmd::Alias class implementation
//

// Return the width of the option tag for printing...
size_t Alias::getOptionWidth() const
{
   return m_argStr.getSize() + 6;
}

void Option::printHelpStr(StringRef helpStr, size_t indent,
                          size_t firstLineIndentedBy)
{
   std::pair<StringRef, StringRef> split = helpStr.split('\n');
   out_stream().indent(indent - firstLineIndentedBy) << " - " << split.first << "\n";
   while (!split.second.empty()) {
      split = split.second.split('\n');
      out_stream().indent(indent) << split.first << "\n";
   }
}

// Print out the option for the Alias.
void Alias::printOptionInfo(size_t globalWidth) const
{
   out_stream() << "  -" << m_argStr;
   printHelpStr(m_helpStr, globalWidth, m_argStr.getSize() + 6);
}

//===----------------------------------------------------------------------===//
// Parser Implementation code...
//

// BasicParser implementation
//

// Return the width of the option tag for printing...
size_t BasicParserImpl::getOptionWidth(const Option &option) const
{
   size_t len = option.m_argStr.getSize();
   auto valName = getValueName();
   if (!valName.empty()) {
      size_t formattingLen = 3;
      if (option.getMiscFlags() & PositionalEatsArgs) {
         formattingLen = 6;
      }
      len += get_value_str(option, valName).getSize() + formattingLen;
   }
   return len + 6;
}

// printOptionInfo - Print out information about this option.  The
// to-be-maintained width is specified.
//
void BasicParserImpl::printOptionInfo(const Option &option,
                                      size_t globalWidth) const
{
   out_stream() << "  -" << option.m_argStr;

   auto valName = getValueName();
   if (!valName.empty()) {
      if (option.getMiscFlags() & PositionalEatsArgs) {
         out_stream() << " <" << get_value_str(option, valName) << ">...";
      } else {
         out_stream() << "=<" << get_value_str(option, valName) << '>';
      }
   }
   Option::printHelpStr(option.m_helpStr, globalWidth, getOptionWidth(option));
}

void BasicParserImpl::printOptionName(const Option &option,
                                      size_t globalWidth) const
{
   out_stream() << "  -" << option.m_argStr;
   out_stream().indent(globalWidth - option.m_argStr.getSize());
}

// Parser<bool> implementation
//
bool Parser<bool>::parse(Option &option, StringRef argName, StringRef arg,
                         bool &value)
{
   if (arg == "" || arg == "true" || arg == "TRUE" || arg == "True" ||
       arg == "1") {
      value = true;
      return false;
   }

   if (arg == "false" || arg == "FALSE" || arg == "False" || arg == "0") {
      value = false;
      return false;
   }
   return option.error("'" + arg +
                       "' is invalid value for boolean argument! Try 0 or 1");
}

// Parser<BoolOrDefault> implementation
//
bool Parser<BoolOrDefault>::parse(Option &option, StringRef argName, StringRef arg,
                                  BoolOrDefault &value)
{
   if (arg == "" || arg == "true" || arg == "TRUE" || arg == "True" ||
       arg == "1") {
      value = BoolOrDefault::BOU_TRUE;
      return false;
   }
   if (arg == "false" || arg == "FALSE" || arg == "False" || arg == "0") {
      value = BoolOrDefault::BOU_FALSE;
      return false;
   }

   return option.error("'" + arg +
                       "' is invalid value for boolean argument! Try 0 or 1");
}

// Parser<int> implementation
//
bool Parser<int>::parse(Option &option, StringRef argName, StringRef arg,
                        int &value)
{
   if (arg.getAsInteger(0, value)) {
      return option.error("'" + arg + "' value invalid for integer argument!");
   }
   return false;
}

// Parser<unsigned> implementation
//
bool Parser<unsigned>::parse(Option &option, StringRef argName, StringRef arg,
                             unsigned &value)
{

   if (arg.getAsInteger(0, value)) {
      return option.error("'" + arg + "' value invalid for uint argument!");
   }
   return false;
}

// Parser<unsigned long long> implementation
//
bool Parser<unsigned long long>::parse(Option &option, StringRef argName,
                                       StringRef arg,
                                       unsigned long long &value)
{

   if (arg.getAsInteger(0, value)) {
      return option.error("'" + arg + "' value invalid for uint argument!");
   }
   return false;
}

namespace {

// Parser<double>/Parser<float> implementation
//
bool parse_double(Option &option, StringRef arg, double &value) {
   if (polar::basic::to_float(arg, value)) {
      return false;
   }
   return option.error("'" + arg + "' value invalid for floating point argument!");
}

} // anonymous namespace

bool Parser<double>::parse(Option &option, StringRef argName, StringRef arg,
                           double &value)
{
   return parse_double(option, arg, value);
}

bool Parser<float>::parse(Option &option, StringRef argName, StringRef arg,
                          float &value)
{
   double dVal;
   if (parse_double(option, arg, dVal)) {
      return true;
   }
   value = (float)dVal;
   return false;
}

// GenericParserBase implementation
//

// findOption - Return the option number corresponding to the specified
// argument string.  If the option is not found, getNumOptions() is returned.
//
unsigned GenericParserBase::findOption(StringRef name)
{
   unsigned end = getNumOptions();
   for (unsigned i = 0; i != end; ++i) {
      if (getOption(i) == name) {
         return i;
      }
   }
   return end;
}

// Return the width of the option tag for printing...
size_t GenericParserBase::getOptionWidth(const Option &option) const
{
   if (option.hasArgStr()) {
      size_t size = option.m_argStr.getSize() + 6;
      for (unsigned i = 0, end = getNumOptions(); i != end; ++i) {
         size = std::max(size, getOption(i).getSize() + 8);
      }
      return size;
   } else {
      size_t baseSize = 0;
      for (unsigned i = 0, end = getNumOptions(); i != end; ++i) {
         baseSize = std::max(baseSize, getOption(i).getSize() + 8);
      }
      return baseSize;
   }
}

// printOptionInfo - Print out information about this option.  The
// to-be-maintained width is specified.
//
void GenericParserBase::printOptionInfo(const Option &option,
                                        size_t globalWidth) const
{
   if (option.hasArgStr()) {
      out_stream() << "  -" << option.m_argStr;
      Option::printHelpStr(option.m_helpStr, globalWidth, option.m_argStr.getSize() + 6);
      for (unsigned i = 0, end = getNumOptions(); i != end; ++i) {
         size_t numSpaces = globalWidth - getOption(i).getSize() - 8;
         out_stream() << "    =" << getOption(i);
         out_stream().indent(numSpaces) << " -   " << getDescription(i) << '\n';
      }
   } else {
      if (!option.m_helpStr.empty()) {
         out_stream() << "  " << option.m_helpStr << '\n';
      }

      for (unsigned i = 0, end = getNumOptions(); i != end; ++i) {
         auto item = getOption(i);
         out_stream() << "    -" << item;
         Option::printHelpStr(getDescription(i), globalWidth, item.getSize() + 8);
      }
   }
}

static const size_t sg_maxOptWidth = 8; // arbitrary spacing for printOptionDiff

// printGenericOptionDiff - Print the value of this option and it's default.
//
// "Generic" options have each value mapped to a name.
void GenericParserBase::printGenericOptionDiff(
      const Option &option, const GenericOptionValue &value,
      const GenericOptionValue &defaultValue, size_t globalWidth) const
{
   out_stream() << "  -" << option.m_argStr;
   out_stream().indent(globalWidth - option.m_argStr.getSize());

   unsigned numOpts = getNumOptions();
   for (unsigned i = 0; i != numOpts; ++i) {
      if (value.compare(getOptionValue(i))) {
         continue;
      }
      out_stream() << "= " << getOption(i);
      size_t size = getOption(i).getSize();
      size_t numSpaces = sg_maxOptWidth > size ? sg_maxOptWidth - size : 0;
      out_stream().indent(numSpaces) << " (default: ";
      for (unsigned j = 0; j != numOpts; ++j) {
         if (defaultValue.compare(getOptionValue(j))) {
            continue;
         }
         out_stream() << getOption(j);
         break;
      }
      out_stream() << ")\n";
      return;
   }
   out_stream() << "= *unknown option value*\n";
}

RawOutStream& operator <<(RawOutStream& out, BoolOrDefault value)
{
   out << polar::as_integer(value);
   return out;
}

// printOptionDiff - Specializations for printing basic value types.
//
#define PRINT_OPT_DIFF(T)                                                      \
   void Parser<T>::printOptionDiff(const Option &option, T value, OptionValue<T> defaultValue,      \
   size_t globalWidth) const {                  \
   printOptionName(option, globalWidth);                                           \
   std::string str;                                                           \
{                                                                          \
   RawStringOutStream strStream(str);                                              \
   strStream << value;                                                                 \
}                                                                          \
   out_stream() << "= " << str;                                                     \
   size_t numSpaces =                                                         \
   sg_maxOptWidth > str.size() ? sg_maxOptWidth - str.size() : 0;               \
   out_stream().indent(numSpaces) << " (default: ";                                 \
   if (defaultValue.hasValue()) {                                                        \
   out_stream() << defaultValue.getValue();                                                  \
} else {                                                                      \
   out_stream() << "*no default*";                                                \
   out_stream() << ")\n";                                                           \
}\
}

PRINT_OPT_DIFF(bool)
PRINT_OPT_DIFF(BoolOrDefault)
PRINT_OPT_DIFF(int)
PRINT_OPT_DIFF(unsigned)
PRINT_OPT_DIFF(unsigned long long)
PRINT_OPT_DIFF(double)
PRINT_OPT_DIFF(float)
PRINT_OPT_DIFF(char)

void Parser<std::string>::printOptionDiff(const Option &option, StringRef value,
                                          const OptionValue<std::string> &defaultValue,
                                          size_t globalWidth) const
{
   printOptionName(option, globalWidth);
   out_stream() << "= " << value;
   size_t numSpaces = sg_maxOptWidth > value.getSize() ? sg_maxOptWidth - value.getSize() : 0;
   out_stream().indent(numSpaces) << " (default: ";
   if (defaultValue.hasValue()) {
      out_stream() << defaultValue.getValue();
   } else {
      out_stream() << "*no default*";
   }
   out_stream() << ")\n";
}

// Print a placeholder for options that don't yet support printOptionDiff().
void BasicParserImpl::printOptionNoValue(const Option &option,
                                         size_t globalWidth) const
{
   printOptionName(option, globalWidth);
   out_stream() << "= *cannot print option value*\n";
}

namespace {

//===----------------------------------------------------------------------===//
// -help and -help-hidden option implementation
//

int opt_name_compare(const std::pair<const char *, Option *> *lhs,
                     const std::pair<const char *, Option *> *rhs)
{
   return strcmp(lhs->first, rhs->first);
}

int sub_name_compare(const std::pair<const char *, SubCommand *> *lhs,
                     const std::pair<const char *, SubCommand *> *rhs)
{
   return strcmp(lhs->first, rhs->first);
}

// Copy Options into a vector so we can sort them as we like.
void sort_opts(StringMap<Option *> &optMap,
               SmallVectorImpl<std::pair<const char *, Option *>> &opts,
               bool showHidden)
{
   SmallPtrSet<Option *, 32> optionSet; // Duplicate option detection.

   for (StringMap<Option *>::iterator iter = optMap.begin(), end = optMap.end();
        iter != end; ++iter) {
      // Ignore really-hidden options.
      if (iter->m_second->getOptionHiddenFlag() == ReallyHidden) {
         continue;
      }
      // Unless showhidden is set, ignore hidden flags.
      if (iter->m_second->getOptionHiddenFlag() == Hidden && !showHidden) {
         continue;
      }

      // If we've already seen this option, don't add it to the list again.
      if (!optionSet.insert(iter->m_second).second) {
         continue;
      }
      opts.push_back(
               std::pair<const char *, Option *>(iter->getKey().getData(), iter->m_second));
   }

   // Sort the options list alphabetically.
   array_pod_sort(opts.begin(), opts.end(), opt_name_compare);
}

void sort_sub_commands(const SmallPtrSetImpl<SubCommand *> &subMap,
                       SmallVectorImpl<std::pair<const char *, SubCommand *>> &subs)
{
   for (const auto &sub : subMap) {
      if (sub->getName().empty()) {
         continue;
      }
      subs.push_back(std::make_pair(sub->getName().getData(), sub));
   }
   array_pod_sort(subs.begin(), subs.end(), sub_name_compare);
}

class HelpPrinter
{
protected:
   const bool m_showHidden;
   typedef SmallVector<std::pair<const char *, Option *>, 128>
   StrOptionPairVector;
   typedef SmallVector<std::pair<const char *, SubCommand *>, 128>
   StrSubCommandPairVector;
   // Print the options. Opts is assumed to be alphabetically sorted.
   virtual void sg_printOptions(StrOptionPairVector &options, size_t maxArgLen)
   {
      for (size_t i = 0, end = options.getSize(); i != end; ++i) {
         options[i].second->printOptionInfo(maxArgLen);
      }
   }

   void printSubCommands(StrSubCommandPairVector &subcommands, size_t maxSubLen)
   {
      for (const auto &subcommand : subcommands) {
         out_stream() << "  " << subcommand.first;
         if (!subcommand.second->getDescription().empty()) {
            out_stream().indent(maxSubLen - strlen(subcommand.first));
            out_stream() << " - " << subcommand.second->getDescription();
         }
         out_stream() << "\n";
      }
   }

public:
   explicit HelpPrinter(bool showHidden) : m_showHidden(showHidden)
   {}

   virtual ~HelpPrinter()
   {}

   // Invoke the printer.
   void operator=(bool value)
   {
      if (!value) {
         return;
      }
      printHelp();
      // Halt the program since help information was printed
      exit(0);
   }

   void printHelp()
   {
      SubCommand *subcommand = sg_globalParser->getActiveSubCommand();
      auto &optionsMap = subcommand->m_optionsMap;
      auto &positionalOpts = subcommand->m_positionalOpts;
      auto &consumeAfterOpt = subcommand->m_consumeAfterOpt;

      StrOptionPairVector opts;
      sort_opts(optionsMap, opts, m_showHidden);

      StrSubCommandPairVector subs;
      sort_sub_commands(sg_globalParser->m_registeredSubCommands, subs);

      if (!sg_globalParser->m_programOverview.empty()) {
         out_stream() << "OVERVIEW: " << sg_globalParser->m_programOverview << "\n";
      }
      if (subcommand == &*sg_topLevelSubCommand) {
         out_stream() << "USAGE: " << sg_globalParser->m_programName;
         if (subs.getSize() > 2) {
            out_stream() << " [subcommand]";
         }
         out_stream() << " [options]";
      } else {
         if (!subcommand->getDescription().empty()) {
            out_stream() << "SUBCOMMAND '" << subcommand->getName()
                         << "': " << subcommand->getDescription() << "\n\n";
         }
         out_stream() << "USAGE: " << sg_globalParser->m_programName << " " << subcommand->getName()
                      << " [options]";
      }

      for (auto opt : positionalOpts) {
         if (opt->hasArgStr()) {
            out_stream() << " --" << opt->m_argStr;
         }
         out_stream() << " " << opt->m_helpStr;
      }

      // Print the consume after option info if it exists...
      if (consumeAfterOpt) {
         out_stream() << " " << consumeAfterOpt->m_helpStr;
      }
      if (subcommand == &*sg_topLevelSubCommand && !subs.empty()) {
         // Compute the maximum subcommand length...
         size_t maxSubLen = 0;
         for (size_t i = 0, end = subs.getSize(); i != end; ++i) {
            maxSubLen = std::max(maxSubLen, strlen(subs[i].first));
         }
         out_stream() << "\n\n";
         out_stream() << "SUBCOMMANDS:\n\n";
         printSubCommands(subs, maxSubLen);
         out_stream() << "\n";
         out_stream() << "  Type \"" << sg_globalParser->m_programName
                      << " <subcommand> -help\" to get more help on a specific "
                         "subcommand";
      }

      out_stream() << "\n\n";

      // Compute the maximum argument length...
      size_t maxArgLen = 0;
      for (size_t i = 0, end = opts.getSize(); i != end; ++i) {
         maxArgLen = std::max(maxArgLen, opts[i].second->getOptionWidth());
      }
      out_stream() << "OPTIONS:\n";
      sg_printOptions(opts, maxArgLen);

      // Print any extra help the user has declared.
      for (auto iter : sg_globalParser->m_moreHelp) {
         out_stream() << iter;
      }
      sg_globalParser->m_moreHelp.clear();
   }
};

class CategorizedHelpPrinter : public HelpPrinter
{
public:
   explicit CategorizedHelpPrinter(bool showHidden)
      : HelpPrinter(showHidden)
   {}

   // Helper function for sg_printOptions().
   // It shall return a negative value if A's name should be lexicographically
   // ordered before B's name. It returns a value greater than zero if B's name
   // should be ordered before A's name, and it returns 0 otherwise.
   static int optionCategoryCompare(OptionCategory *const *lhs,
                                    OptionCategory *const *rhs)
   {
      return (*lhs)->getName().compare((*rhs)->getName());
   }

   // Make sure we inherit our base class's operator=()
   using HelpPrinter::operator=;

protected:
   void sg_printOptions(StrOptionPairVector &opts, size_t maxArgLen) override
   {
      std::vector<OptionCategory *> sortedCategories;
      std::map<OptionCategory *, std::vector<Option *>> categorizedOptions;

      // Collect registered option categories into vector in preparation for
      // sorting.
      for (auto iter = sg_globalParser->m_registeredOptionCategories.begin(),
           end = sg_globalParser->m_registeredOptionCategories.end();
           iter != end; ++iter) {
         sortedCategories.push_back(*iter);
      }

      // Sort the different option categories alphabetically.
      assert(sortedCategories.size() > 0 && "No option categories registered!");
      array_pod_sort(sortedCategories.begin(), sortedCategories.end(),
                     optionCategoryCompare);

      // Create map to empty vectors.
      for (std::vector<OptionCategory *>::const_iterator
           iter = sortedCategories.begin(),
           end = sortedCategories.end();
           iter != end; ++iter) {
         categorizedOptions[*iter] = std::vector<Option *>();
      }

      // Walk through pre-sorted options and assign into categories.
      // Because the options are already alphabetically sorted the
      // options within categories will also be alphabetically sorted.
      for (size_t index = 0, end = opts.getSize(); index != end; ++index) {
         Option *option = opts[index].second;
         assert(categorizedOptions.count(option->m_category) > 0 &&
                "Option has an unregistered category");
         categorizedOptions[option->m_category].push_back(option);
      }

      // Now do printing.
      for (std::vector<OptionCategory *>::const_iterator
           iter = sortedCategories.begin(),
           end = sortedCategories.end();
           iter != end; ++iter) {
         // Hide empty categories for -help, but show for -help-hidden.
         const auto &categoryOptions = categorizedOptions[*iter];
         bool isEmptyCategory = categoryOptions.empty();
         if (!m_showHidden && isEmptyCategory) {
            continue;
         }
         // Print category information.
         out_stream() << "\n";
         out_stream() << (*iter)->getName() << ":\n";

         // Check if description is set.
         if (!(*iter)->getDescription().empty()) {
            out_stream() << (*iter)->getDescription() << "\n\n";
         } else {
            out_stream() << "\n";
         }
         // When using -help-hidden explicitly state if the category has no
         // options associated with it.
         if (isEmptyCategory) {
            out_stream() << "  This option category has no options.\n";
            continue;
         }
         // Loop over the options in the category and print.
         for (const Option *option : categoryOptions) {
            option->printOptionInfo(maxArgLen);
         }
      }
   }
};

// This wraps the Uncategorizing and Categorizing printers and decides
// at run time which should be invoked.
class HelpPrinterWrapper
{
private:
   HelpPrinter &m_uncategorizedPrinter;
   CategorizedHelpPrinter &m_categorizedPrinter;

public:
   explicit HelpPrinterWrapper(HelpPrinter &uncategorizedPrinter,
                               CategorizedHelpPrinter &categorizedPrinter)
      : m_uncategorizedPrinter(uncategorizedPrinter),
        m_categorizedPrinter(categorizedPrinter)
   {}

   // Invoke the printer.
   void operator=(bool value);
};

} // End anonymous namespace

// Define a category for generic options that all tools should have.
static cmd::OptionCategory sg_genericCategory("Generic Options");

// Declare the four HelpPrinter instances that are used to print out help, or
// help-hidden as an uncategorized list or in categories.
static HelpPrinter sg_uncategorizedNormalPrinter(false);
static HelpPrinter sg_uncategorizedHiddenPrinter(true);
static CategorizedHelpPrinter sg_categorizedNormalPrinter(false);
static CategorizedHelpPrinter sg_categorizedHiddenPrinter(true);

// Declare HelpPrinter wrappers that will decide whether or not to invoke
// a categorizing help printer
static HelpPrinterWrapper sg_wrappedNormalPrinter(sg_uncategorizedNormalPrinter,
                                                  sg_categorizedNormalPrinter);
static HelpPrinterWrapper sg_wrappedHiddenPrinter(sg_uncategorizedHiddenPrinter,
                                                  sg_categorizedHiddenPrinter);

static VersionPrinterType sg_overrideVersionPrinter = nullptr;

static std::vector<VersionPrinterType> *sg_extraVersionPrinters = nullptr;

namespace {
class VersionPrinter
{
public:
   void print() {
      RawOutStream &outstream = out_stream();
      outstream << "polarphp (https://polarphp.org/):\n  ";
      outstream << POLARPHP_PACKAGE_NAME << " version " << POLARPHP_VERSION;
      outstream << "\n  ";
#ifndef __OPTIMIZE__
      outstream << "DEBUG build";
#else
      outstream << "Optimized build";
#endif
#ifndef NDEBUG
      outstream << " with assertions";
#endif
#if POLAR_VERSION_PRINTER_SHOW_HOST_TARGET_INFO
      std::string CPU = sys::get_host_cpu_name();
      if (CPU == "generic") {
         CPU = "(unknown)";
      }
      outstream << ".\n"
                << "  Default target: " << sys::get_default_target_triple() << '\n'
                << "  Host CPU: " << CPU;
#endif
      outstream << '\n';
   }
   void operator=(bool optionWasSpecified)
   {
      if (!optionWasSpecified) {
         return;
      }
      if (sg_overrideVersionPrinter != nullptr) {
         sg_overrideVersionPrinter(out_stream());
         exit(0);
      }
      print();

      // Iterate over any registered extra printers and call them to add further
      // information.
      if (sg_extraVersionPrinters != nullptr) {
         out_stream() << '\n';
         for (auto iter : *sg_extraVersionPrinters) {
            iter(out_stream());
         }
      }
      exit(0);
   }
};
} // End anonymous namespace

// Define the --version option that prints out the LLVM version for the tool
static VersionPrinter sg_versionPrinterInstance;

static cmd::Opt<VersionPrinter, true, Parser<bool>>
sg_versOp("version", cmd::Desc("Display the version of this program"),
          cmd::location(sg_versionPrinterInstance), cmd::ValueDisallowed,
          cmd::Category(sg_genericCategory));

// Define uncategorized help printers.
// -help-list is hidden by default because if Option categories are being used
// then -help behaves the same as -help-list.
static cmd::Opt<HelpPrinter, true, Parser<bool>> sg_hLOp(
      "help-list",
      cmd::Desc("Display list of available options (-help-list-hidden for more)"),
      cmd::location(sg_uncategorizedNormalPrinter), cmd::Hidden, cmd::ValueDisallowed,
      cmd::Category(sg_genericCategory), cmd::Sub(*sg_allSubCommands));

static cmd::Opt<HelpPrinter, true, Parser<bool>>
sg_hLhOp("help-list-hidden", cmd::Desc("Display list of all available options"),
         cmd::location(sg_uncategorizedHiddenPrinter), cmd::Hidden,
         cmd::ValueDisallowed, cmd::Category(sg_genericCategory),
         cmd::Sub(*sg_allSubCommands));

// Define uncategorized/categorized help printers. These printers change their
// behaviour at runtime depending on whether one or more Option categories have
// been declared.
static cmd::Opt<HelpPrinterWrapper, true, Parser<bool>>
sg_hOp("help", cmd::Desc("Display available options (-help-hidden for more)"),
       cmd::location(sg_wrappedNormalPrinter), cmd::ValueDisallowed,
       cmd::Category(sg_genericCategory), cmd::Sub(*sg_allSubCommands));

static cmd::Opt<HelpPrinterWrapper, true, Parser<bool>>
sg_hHOp("help-hidden", cmd::Desc("Display all available options"),
        cmd::location(sg_wrappedHiddenPrinter), cmd::Hidden, cmd::ValueDisallowed,
        cmd::Category(sg_genericCategory), cmd::Sub(*sg_allSubCommands));

static cmd::Opt<bool> sg_printOptions(
      "print-options",
      cmd::Desc("Print non-default options after command line parsing"),
      cmd::Hidden, cmd::init(false), cmd::Category(sg_genericCategory),
      cmd::Sub(*sg_allSubCommands));

static cmd::Opt<bool> sg_printAllOptions(
      "print-all-options",
      cmd::Desc("Print all option values after command line parsing"), cmd::Hidden,
      cmd::init(false), cmd::Category(sg_genericCategory), cmd::Sub(*sg_allSubCommands));

void HelpPrinterWrapper::operator=(bool value)
{
   if (!value) {
      return;
   }
   // Decide which printer to invoke. If more than one option category is
   // registered then it is useful to show the categorized help instead of
   // uncategorized help.
   if (sg_globalParser->m_registeredOptionCategories.getSize() > 1) {
      // unhide -help-list option so user can have uncategorized output if they
      // want it.
      sg_hLOp.setHiddenFlag(NotHidden);

      m_categorizedPrinter = true; // Invoke categorized printer
   } else {
      m_uncategorizedPrinter = true; // Invoke uncategorized printer
   }
}

// Print the value of each option.
void print_option_values()
{
   sg_globalParser->printOptionValues();
}

void CommandLineParser::printOptionValues()
{
   if (!sg_printOptions && !sg_printAllOptions) {
      return;
   }
   SmallVector<std::pair<const char *, Option *>, 128> opts;
   sort_opts(m_activeSubCommand->m_optionsMap, opts, /*ShowHidden*/ true);

   // Compute the maximum argument length...
   size_t maxArgLen = 0;
   for (size_t i = 0, e = opts.getSize(); i != e; ++i) {
      maxArgLen = std::max(maxArgLen, opts[i].second->getOptionWidth());
   }

   for (size_t i = 0, e = opts.getSize(); i != e; ++i) {
      opts[i].second->printOptionValue(maxArgLen, sg_printAllOptions);
   }
}

// Utility function for printing the help message.
void print_help_message(bool hidden, bool categorized)
{
   if (!Hidden && !categorized) {
      sg_categorizedNormalPrinter.printHelp();
   } else if (!Hidden && categorized) {
      sg_categorizedNormalPrinter.printHelp();
   } else if (Hidden && !categorized) {
      sg_categorizedHiddenPrinter.printHelp();
   } else {
      sg_categorizedHiddenPrinter.printHelp();
   }
}

/// Utility function for printing version number.
void print_version_message()
{
   sg_versionPrinterInstance.print();
}

void set_version_printer(VersionPrinterType func)
{
   sg_overrideVersionPrinter = func;
}

void add_extra_version_printer(VersionPrinterType func)
{
   if (!sg_extraVersionPrinters) {
      sg_extraVersionPrinters = new std::vector<VersionPrinterType>;
   }
   sg_extraVersionPrinters->push_back(func);
}

StringMap<Option *> &get_registered_options(SubCommand &sub)
{
   auto &subs = sg_globalParser->m_registeredSubCommands;
   (void)subs;
   assert(polar::basic::is_contained(subs, &sub));
   return sub.m_optionsMap;
}

IteratorRange<typename SmallPtrSet<SubCommand *, 4>::iterator>
get_registered_subcommands()
{
   return sg_globalParser->getRegisteredSubCommands();
}

void hide_unrelated_options(cmd::OptionCategory &category, SubCommand &sub)
{
   for (auto &iter : sub.m_optionsMap) {
      if (iter.m_second->m_category != &category &&
          iter.m_second->m_category != &sg_genericCategory) {
         iter.m_second->setHiddenFlag(cmd::ReallyHidden);
      }
   }
}

void hide_unrelated_options(ArrayRef<const cmd::OptionCategory *> categories,
                            SubCommand &sub)
{
   auto categoriesBegin = categories.begin();
   auto categoriesEnd = categories.end();
   for (auto &iter : sub.m_optionsMap) {
      if (std::find(categoriesBegin, categoriesEnd, iter.m_second->m_category) ==
          categoriesEnd &&
          iter.m_second->m_category != &sg_genericCategory) {
         iter.m_second->setHiddenFlag(cmd::ReallyHidden);
      }
   }
}

void reset_command_line_parser()
{
   sg_globalParser->reset();
}

void reset_all_option_occurrences()
{
   sg_globalParser->resetAllOptionOccurrences();
}

void polarphp_parse_commandline_options(int argc, const char *const *argv,
                                        const char *overview)
{
   cmd::parse_commandline_options(argc, argv, StringRef(overview),
                                  &polar::utils::null_stream());
}

} // cmd
} // polar
