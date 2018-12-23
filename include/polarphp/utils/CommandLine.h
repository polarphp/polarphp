// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

//===----------------------------------------------------------------------===//
//
// This class implements a command line argument processor that is useful when
// creating a tool.  It provides a simple, minimalistic interface that is easily
// extensible and supports nonlocal (library) command line options.
//
// Note that rather than trying to figure out what this code does, you should
// read the library documentation located in docs/CommandLine.html or looks at
// the many example usages in tools/*/*.cpp
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_COMMAND_LINE_H
#define POLARPHP_UTILS_COMMAND_LINE_H

#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/RawOutStream.h"

#include <cassert>
#include <climits>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <vector>

namespace polar {

// forward declare class with namespace
namespace utils {
class StringSaver;
} // utils

/// cmd Namespace - This namespace contains all of the command line option
/// processing machinery.  It is intentionally a short name to make qualified
/// usage concise.
namespace cmd {

using polar::utils::RawOutStream;
using polar::basic::StringRef;
using polar::basic::StringMap;
using polar::basic::SmallVector;
using polar::basic::Twine;
using polar::basic::SmallPtrSet;
using polar::basic::SmallVectorImpl;
using polar::basic::ArrayRef;
using polar::basic::IteratorRange;
using polar::utils::StringSaver;
using polar::utils::ManagedStatic;

//===----------------------------------------------------------------------===//
// parse_commandline_options - Command line option processing entry point.
//
// Returns true on success. Otherwise, this will print the error message to
// stderr and exit if \p errStream is not set (nullptr by default), or print the
// error message to \p errStream and return false if \p errStream is provided.
//
// If EnvVar is not nullptr, command-line options are also parsed from the
// environment variable named by EnvVar.  Precedence is given to occurrences
// from argv.  This precedence is currently implemented by parsing argv after
// the environment variable, so it is only implemented correctly for options
// that give precedence to later occurrences.  If your program supports options
// that give precedence to earlier occurrences, you will need to extend this
bool parse_commandline_options(int argc, const char *const *argv,
                               StringRef overview = "",
                               RawOutStream *errStream = nullptr,
                               const char *envVar = nullptr);

//===----------------------------------------------------------------------===//
// parse_environment_options - Environment variable option processing alternate
//                           entry point.
//
void parse_environment_options(const char *progName, const char *envvar,
                               const char *overview = "");

// Function pointer type for printing version information.
using VersionPrinterType = std::function<void(RawOutStream &)>;

///===---------------------------------------------------------------------===//
/// set_version_printer - Override the default (LLVM specific) version printer
///                     used to print out the version when --version is given
///                     on the command line. This allows other systems using the
///                     CommandLine utilities to print their own version string.
void set_version_printer(VersionPrinterType func);

///===---------------------------------------------------------------------===//
/// add_extra_version_printer - Add an extra printer to use in addition to the
///                          default one. This can be called multiple times,
///                          and each time it adds a new function to the list
///                          which will be called after the basic LLVM version
///                          printing is complete. Each can then add additional
///                          information specific to the tool.
void add_extra_version_printer(VersionPrinterType func);

// print_option_values - Print option values.
// With -print-options print the difference between option values and defaults.
// With -print-all-options print all option values.
// (Currently not perfect, but best-effort.)
void print_option_values();

// Forward declaration - addLiteralOption needs to be up here to make gcc happy.
class Option;

/// Adds a new option for parsing and provides the option it refers to.
///
/// \param O pointer to the option
/// \param Name the string name for the option to handle during parsing
///
/// Literal options are used by some Parsers to register special option values.
/// This is how the PassNameParser registers pass names for opt.
void add_literal_option(Option &option, StringRef name);

//===----------------------------------------------------------------------===//
// Flags permitted to be passed to command line arguments
//

enum NumOccurrencesFlag
{ // Flags for the number of occurrences allowed
   Optional = 0x00,        // Zero or One occurrence
   ZeroOrMore = 0x01,      // Zero or more occurrences allowed
   Required = 0x02,        // One occurrence required
   OneOrMore = 0x03,       // One or more occurrences required

   // ConsumeAfter - Indicates that this option is fed anything that follows the
   // last positional argument required by the application (it is an error if
   // there are zero positional arguments, and a ConsumeAfter option is used).
   // Thus, for example, all arguments to LLI are processed until a filename is
   // found.  Once a filename is found, all of the succeeding arguments are
   // passed, unprocessed, to the ConsumeAfter option.
   //
   ConsumeAfter = 0x04
};

enum ValueExpected
{ // Is a value required for the option?
   // zero reserved for the unspecified value
   ValueOptional = 0x01,  // The value can appear... or not
   ValueRequired = 0x02,  // The value is required to appear!
   ValueDisallowed = 0x03 // A value may not be specified (for flags)
};

enum OptionHidden
{   // Control whether -help shows this option
   NotHidden = 0x00,   // Option included in -help & -help-hidden
   Hidden = 0x01,      // -help doesn't, but -help-hidden does
   ReallyHidden = 0x02 // Neither -help nor -help-hidden show this arg
};

// Formatting flags - This controls special features that the option might have
// that cause it to be parsed differently...
//
// Prefix - This option allows arguments that are otherwise unrecognized to be
// matched by options that are a prefix of the actual value.  This is useful for
// cases like a linker, where options are typically of the form '-lfoo' or
// '-L../../include' where -l or -L are the actual flags.  When prefix is
// enabled, and used, the value for the flag comes from the suffix of the
// argument.
//
// Grouping - With this option enabled, multiple letter options are allowed to
// bunch together with only a single hyphen for the whole group.  This allows
// emulation of the behavior that ls uses for example: ls -la === ls -l -a
//

enum FormattingFlags
{
   NormalFormatting = 0x00, // Nothing special
   Positional = 0x01,       // Is a positional argument, no '-' required
   Prefix = 0x02,           // Can this option directly prefix its value?
   Grouping = 0x03          // Can this option group with other options?
};

enum MiscFlags
{             // Miscellaneous flags to adjust argument
   CommaSeparated = 0x01,     // Should this cl::list split between commas?
   PositionalEatsArgs = 0x02, // Should this positional cl::list eat -args?
   Sink = 0x04                // Should this cl::list eat all unknown options?
};

//===----------------------------------------------------------------------===//
// Option Category class
//
class OptionCategory
{
private:
   StringRef const m_name;
   StringRef const m_description;

   void getRegisterCategory();

public:
   OptionCategory(StringRef const name,
                  StringRef const Description = "")
      : m_name(name), m_description(Description)
   {
      getRegisterCategory();
   }

   StringRef getName() const
   {
      return m_name;
   }

   StringRef getDescription() const
   {
      return m_description;
   }
};

// The general Option Category (used as default category).
extern OptionCategory sg_generalCategory;

//===----------------------------------------------------------------------===//
// SubCommand class
//
class SubCommand
{
private:
   StringRef m_name;
   StringRef m_description;

protected:
   void registerSubCommand();
   void unregisterSubCommand();

public:
   SubCommand(StringRef name, StringRef Description = "")
      : m_name(name), m_description(Description)
   {
      registerSubCommand();
   }
   SubCommand() = default;

   void reset();

   explicit operator bool() const;

   StringRef getName() const
   {
      return m_name;
   }

   StringRef getDescription() const
   {
      return m_description;
   }

   SmallVector<Option *, 4> m_positionalOpts;
   SmallVector<Option *, 4> m_sinkOpts;
   StringMap<Option *> m_optionsMap;

   Option *m_consumeAfterOpt = nullptr; // The ConsumeAfter option if it exists.
};

// A special subcommand representing no subcommand
extern ManagedStatic<SubCommand> sg_topLevelSubCommand;

// A special subcommand that can be used to put an option into all subcommands.
extern ManagedStatic<SubCommand> sg_allSubCommands;

//===----------------------------------------------------------------------===//
// Option Base class
//
class Option
{
   friend class Alias;

   // handleOccurrences - Overriden by subclasses to handle the value passed into
   // an argument.  Should return true if there was an error processing the
   // argument and the program should exit.
   //
   virtual bool handleOccurrence(unsigned pos, StringRef argName,
                                 StringRef arg) = 0;

   virtual enum ValueExpected getValueExpectedFlagDefault() const
   {
      return ValueOptional;
   }

   // Out of line virtual function to provide home for the class.
   virtual void anchor();

   int m_numOccurrences = 0; // The number of times specified
   // Occurrences, HiddenFlag, and Formatting are all enum types but to avoid
   // problems with signed enums in bitfields.
   unsigned m_occurrences : 3; // enum NumOccurrencesFlag
   // not using the enum type for 'Value' because zero is an implementation
   // detail representing the non-value
   unsigned m_value : 2;
   unsigned m_hiddenFlag : 2; // enum OptionHidden
   unsigned m_formatting : 2; // enum FormattingFlags
   unsigned m_misc : 3;
   unsigned m_position = 0;       // Position of last occurrence of the option
   unsigned m_additionalVals = 0; // Greater than 0 for multi-valued option.

public:
   StringRef m_argStr;   // The argument string itself (ex: "help", "o")
   StringRef m_helpStr;  // The Descriptive text message for -help
   StringRef m_valueStr; // String Describing what the value of this option is
   OptionCategory *m_category; // The Category this option belongs to
   SmallPtrSet<SubCommand *, 4> m_subs; // The subcommands this option belongs to.
   bool m_fullyInitialized = false; // Has addArgument been called?

   inline enum NumOccurrencesFlag getNumOccurrencesFlag() const
   {
      return (enum NumOccurrencesFlag)m_occurrences;
   }

   inline enum ValueExpected getValueExpectedFlag() const
   {
      return m_value ? ((enum ValueExpected)m_value) : getValueExpectedFlagDefault();
   }

   inline enum OptionHidden getOptionHiddenFlag() const
   {
      return (enum OptionHidden)m_hiddenFlag;
   }

   inline enum FormattingFlags getFormattingFlag() const
   {
      return (enum FormattingFlags)m_formatting;
   }

   inline unsigned getMiscFlags() const
   {
      return m_misc;
   }

   inline unsigned getPosition() const
   {
      return m_position;
   }

   inline unsigned getNumAdditionalVals() const
   {
      return m_additionalVals;
   }

   // hasArgStr - Return true if the argstr != ""
   bool hasArgStr() const
   {
      return !m_argStr.empty();
   }

   bool isPositional() const
   {
      return getFormattingFlag() == cmd::Positional;
   }

   bool isSink() const
   {
      return getMiscFlags() & cmd::Sink;
   }

   bool isConsumeAfter() const
   {
      return getNumOccurrencesFlag() == cmd::ConsumeAfter;
   }

   bool isInAllSubCommands() const
   {
      return polar::basic::any_of(m_subs, [](const SubCommand *subcmd) {
         return subcmd == &*sg_allSubCommands;
      });
   }

   //-------------------------------------------------------------------------===
   // Accessor functions set by OptionModifiers
   //
   void setArgStr(StringRef str);
   void setDescription(StringRef str)
   {
      m_helpStr = str;
   }
   void setValueStr(StringRef str)
   {
      m_valueStr = str;
   }
   void setNumOccurrencesFlag(enum NumOccurrencesFlag value)
   {
      m_occurrences = value;
   }

   void setValueExpectedFlag(enum ValueExpected value)
   {
      m_value = value;
   }

   void setHiddenFlag(enum OptionHidden value)
   {
      m_hiddenFlag = value;
   }

   void setFormattingFlag(enum FormattingFlags value)
   {
      m_formatting = value;
   }

   void setMiscFlag(enum MiscFlags flag)
   {
      m_misc |= flag;
   }

   void setPosition(unsigned pos)
   {
      m_position = pos;
   }

   void setCategory(OptionCategory &category)
   {
      m_category = &category;
   }

   void addSubCommand(SubCommand &cmd)
   {
      m_subs.insert(&cmd);
   }

protected:
   explicit Option(enum NumOccurrencesFlag occurrencesFlag,
                   enum OptionHidden hidden)
      : m_occurrences(occurrencesFlag), m_value(0), m_hiddenFlag(hidden),
        m_formatting(NormalFormatting), m_misc(0), m_category(&sg_generalCategory)
   {}

   inline void setNumAdditionalVals(unsigned n)
   {
      m_additionalVals = n;
   }

public:
   virtual ~Option() = default;

   // addArgument - Register this argument with the commandline system.
   //
   void addArgument();

   /// Unregisters this option from the CommandLine system.
   ///
   /// This option must have been the last option registered.
   /// For testing purposes only.
   void removeArgument();

   // Return the width of the option tag for printing...
   virtual size_t getOptionWidth() const = 0;

   // printOptionInfo - Print out information about this option.  The
   // to-be-maintained width is specified.
   //
   virtual void printOptionInfo(size_t globalWidth) const = 0;

   virtual void printOptionValue(size_t globalWidth, bool force) const = 0;

   virtual void setDefault() = 0;

   static void printHelpStr(StringRef helpStr, size_t indent,
                            size_t firstLineIndentedBy);

   virtual void getExtraOptionNames(SmallVectorImpl<StringRef> &)
   {}

   // addOccurrence - Wrapper around handleOccurrence that enforces Flags.
   //
   virtual bool addOccurrence(unsigned pos, StringRef argName, StringRef value,
                              bool multiArg = false);

   // Prints option name followed by message.  Always returns true.
   bool error(const Twine &message, StringRef argName = StringRef(), RawOutStream &errorStream = polar::utils::error_stream());
   bool error(const Twine &message, RawOutStream &errorStream)
   {
      return error(message, StringRef(), errorStream);
   }

   inline int getNumOccurrences() const
   {
      return m_numOccurrences;
   }

   inline void reset()
   {
      m_numOccurrences = 0;
   }
};

//===----------------------------------------------------------------------===//
// Command line option modifiers that can be used to modify the behavior of
// command line option Parsers...
//

// Desc - Modifier to set the Description shown in the -help output...
struct Desc
{
   StringRef m_desc;

   Desc(StringRef str) : m_desc(str) {}

   void apply(Option &option) const
   {
      option.setDescription(m_desc);
   }
};

// ValueDesc - Modifier to set the value Description shown in the -help
// output...
struct ValueDesc
{
   StringRef m_desc;

   ValueDesc(StringRef str) : m_desc(str) {}

   void apply(Option &option) const
   {
      option.setValueStr(m_desc);
   }
};

// init - Specify a default (initial) value for the command line argument, if
// the default constructor for the argument type does not give you what you
// want.  This is only valid on "opt" arguments, not on "list" arguments.
//
template <typename Type>
struct Initializer
{
   const Type &m_init;
   Initializer(const Type &value) : m_init(value)
   {}

   template <typename Opt> void apply(Opt &option) const
   {
      option.setInitialValue(m_init);
   }
};

template <typename Type>
Initializer<Type> init(const Type &value)
{
   return Initializer<Type>(value);
}

// location - Allow the user to specify which external variable they want to
// store the results of the command line argument processing into, if they don't
// want to store it in the option itself.
//
template <typename Type>
struct LocationClass
{
   Type &m_loc;

   LocationClass(Type &loc) : m_loc(loc)
   {}

   template <typename Opt>
   void apply(Opt &option) const
   {
      option.setLocation(option, m_loc);
   }
};

template <typename Type>
LocationClass<Type> location(Type &loc)
{
   return LocationClass<Type>(loc);
}

// cat - Specifiy the Option category for the command line argument to belong
// to.
struct Category
{
   OptionCategory &m_category;

   Category(OptionCategory &category) : m_category(category)
   {}

   template <typename Opt>
   void apply(Opt &opt) const
   {
      opt.setCategory(m_category);
   }
};

// sub - Specify the subcommand that this option belongs to.
struct Sub
{
   SubCommand &m_sub;

   Sub(SubCommand &sub) : m_sub(sub)
   {}

   template <typename Opt>
   void apply(Opt &opt) const
   {
      opt.addSubCommand(m_sub);
   }
};

//===----------------------------------------------------------------------===//
// OptionValue class

// Support value comparison outside the template.
struct GenericOptionValue
{
   virtual bool compare(const GenericOptionValue &other) const = 0;

protected:
   GenericOptionValue() = default;
   GenericOptionValue(const GenericOptionValue&) = default;
   GenericOptionValue &operator=(const GenericOptionValue &) = default;
   ~GenericOptionValue() = default;

private:
   virtual void anchor();
};

template <typename DataType>
struct OptionValue;

// The default value safely does nothing. Option value printing is only
// best-effort.
template <typename DataType, bool isClass>
struct OptionValueBase : public GenericOptionValue
{
   // Temporary storage for argument passing.
   using WrapperType = OptionValue<DataType>;

   bool hasValue() const
   {
      return false;
   }

   const DataType &getValue() const
   {
      polar_unreachable("no default value");
   }

   // Some options may take their value from a different data type.
   template <typename OtherDataType>
   void setValue(const OtherDataType & /*V*/)
   {}

   bool compare(const DataType & /*V*/) const
   {
      return false;
   }

   bool compare(const GenericOptionValue & /*V*/) const override
   {
      return false;
   }

protected:
   ~OptionValueBase() = default;
};

// Simple copy of the option value.
template <typename DataType>
class OptionValueCopy : public GenericOptionValue
{
   DataType m_value;
   bool m_valid = false;

protected:
   OptionValueCopy(const OptionValueCopy&) = default;
   OptionValueCopy &operator=(const OptionValueCopy &) = default;
   ~OptionValueCopy() = default;

public:
   OptionValueCopy() = default;

   bool hasValue() const
   {
      return m_valid;
   }

   const DataType &getValue() const
   {
      assert(m_valid && "invalid option value");
      return m_value;
   }

   void setValue(const DataType &value)
   {
      m_valid = true;
      m_value = value;
   }

   bool compare(const DataType &value) const
   {
      return m_valid && (m_value != value);
   }

   bool compare(const GenericOptionValue &value) const override
   {
      const OptionValueCopy<DataType> &valueCopy =
            static_cast<const OptionValueCopy<DataType> &>(value);
      if (!valueCopy.hasValue())
         return false;
      return compare(valueCopy.getValue());
   }
};

// Non-class option values.
template <typename DataType>
struct OptionValueBase<DataType, false> : OptionValueCopy<DataType>
{
   using WrapperType = DataType;

protected:
   OptionValueBase() = default;
   OptionValueBase(const OptionValueBase&) = default;
   OptionValueBase &operator=(const OptionValueBase &) = default;
   ~OptionValueBase() = default;
};

// Top-level option class.
template <typename DataType>
struct OptionValue final
      : OptionValueBase<DataType, std::is_class<DataType>::value>
{
   OptionValue() = default;

   OptionValue(const DataType &value) { this->setValue(value); }

   // Some options may take their value from a different data type.
   template <typename OtherDataType>
   OptionValue<DataType> &operator=(const OtherDataType &value)
   {
      this->setValue(value);
      return *this;
   }
};

// Other safe-to-copy-by-value common option types.
enum class BoolOrDefault
{
   BOU_UNSET,
   BOU_TRUE,
   BOU_FALSE
};

RawOutStream& operator <<(RawOutStream& out, BoolOrDefault);

template <>
struct OptionValue<cmd::BoolOrDefault> : OptionValueCopy<cmd::BoolOrDefault>
{
   using WrapperType = cmd::BoolOrDefault;
   OptionValue() = default;
   OptionValue(const cmd::BoolOrDefault &value)
   {
      this->setValue(value);
   }

   OptionValue<cmd::BoolOrDefault> &operator=(const cmd::BoolOrDefault &value)
   {
      setValue(value);
      return *this;
   }

private:
   void anchor() override;
};

template <>
struct OptionValue<std::string>  : OptionValueCopy<std::string>
{
   using WrapperType = StringRef;

   OptionValue() = default;

   OptionValue(const std::string &value)
   {
      this->setValue(value);
   }

   OptionValue<std::string> &operator=(const std::string &value)
   {
      setValue(value);
      return *this;
   }

private:
   void anchor() override;
};

//===----------------------------------------------------------------------===//
// Enum valued command line option
//

// This represents a single enum value, using "int" as the underlying type.
struct OptionEnumValue
{
   StringRef m_name;
   int m_value;
   StringRef m_description;
};

#define clEnumVal(ENUMVAL, Desc)                                               \
   polar::cmd::OptionEnumValue { #ENUMVAL, int(ENUMVAL), Desc }
#define clEnumValN(ENUMVAL, FLAGNAME, Desc)                                    \
   polar::cmd::OptionEnumValue { FLAGNAME, int(ENUMVAL), Desc }

// values - For custom data types, allow specifying a group of values together
// as the values that go into the mapping that the option handler uses.
//
class ValuesClass
{
   // Use a vector instead of a map, because the lists should be short,
   // the overhead is less, and most importantly, it keeps them in the order
   // inserted so we can print our option out nicely.
   SmallVector<OptionEnumValue, 4> m_values;

public:
   ValuesClass(std::initializer_list<OptionEnumValue> options)
      : m_values(options)
   {}

   template <class Opt>
   void apply(Opt &opt) const
   {
      for (auto value : m_values) {
         opt.getParser().addLiteralOption(value.m_name, value.m_value,
                                          value.m_description);
      }
   }
};

/// Helper to build a ValuesClass by forwarding a variable number of arguments
/// as an Initializer list to the ValuesClass constructor.
template <typename... OptsType>
ValuesClass values(OptsType... options)
{
   return ValuesClass({options...});
}

//===----------------------------------------------------------------------===//
// Parser class - Parameterizable Parser for different data types.  By default,
// known data types (string, int, bool) have specialized Parsers, that do what
// you would expect.  The default Parser, used for data types that are not
// built-in, uses a mapping table to map specific options to values, which is
// used, among other things, to handle enum types.

//--------------------------------------------------
// GenericParserBase - This class holds all the non-generic code that we do
// not need replicated for every instance of the generic Parser.  This also
// allows us to put stuff into CommandLine.cpp
//
class GenericParserBase
{
protected:
   class GenericOptionInfo
   {
   public:
      GenericOptionInfo(StringRef name, StringRef helpStr)
         : m_name(name), m_helpStr(helpStr)
      {}
      StringRef m_name;
      StringRef m_helpStr;
   };

public:
   GenericParserBase(Option &opt) : m_owner(opt)
   {}

   virtual ~GenericParserBase() = default;
   // Base class should have virtual-destructor

   // getNumOptions - Virtual function implemented by generic subclass to
   // indicate how many entries are in Values.
   //
   virtual unsigned getNumOptions() const = 0;

   // getOption - Return option name N.
   virtual StringRef getOption(unsigned num) const = 0;

   // getDescription - Return Description N
   virtual StringRef getDescription(unsigned num) const = 0;

   // Return the width of the option tag for printing...
   virtual size_t getOptionWidth(const Option &option) const;

   virtual const GenericOptionValue &getOptionValue(unsigned opt) const = 0;

   // printOptionInfo - Print out information about this option.  The
   // to-be-maintained width is specified.
   //
   virtual void printOptionInfo(const Option &opt, size_t globalWidth) const;

   void printGenericOptionDiff(const Option &option, const GenericOptionValue &value,
                               const GenericOptionValue &defaultValue,
                               size_t globalWidth) const;

   // printOptionDiff - print the value of an option and it's default.
   //
   // Template definition ensures that the option and default have the same
   // DataType (via the same AnyOptionValue).
   template <class AnyOptionValue>
   void printOptionDiff(const Option &opt, const AnyOptionValue &value,
                        const AnyOptionValue &defaultValue,
                        size_t globalWidth) const
   {
      printGenericOptionDiff(opt, value, defaultValue, globalWidth);
   }

   void initialize()
   {}

   void getExtraOptionNames(SmallVectorImpl<StringRef> &optionNames)
   {
      // If there has been no argstr specified, that means that we need to add an
      // argument for every possible option.  This ensures that our options are
      // vectored to us.
      if (!m_owner.hasArgStr()) {
         for (unsigned i = 0, e = getNumOptions(); i != e; ++i) {
            optionNames.push_back(getOption(i));
         }
      }
   }

   enum ValueExpected getValueExpectedFlagDefault() const
   {
      // If there is an ArgStr specified, then we are of the form:
      //
      //    -opt=O2   or   -opt O2  or  -optO2
      //
      // In which case, the value is required.  Otherwise if an arg str has not
      // been specified, we are of the form:
      //
      //    -O2 or O2 or -la (where -l and -a are separate options)
      //
      // If this is the case, we cannot allow a value.
      //
      if (m_owner.hasArgStr()) {
         return ValueRequired;
      } else {
         return ValueDisallowed;
      }
   }

   // findOption - Return the option number corresponding to the specified
   // argument string.  If the option is not found, getNumOptions() is returned.
   //
   unsigned findOption(StringRef name);

protected:
   Option &m_owner;
};

// Default Parser implementation - This implementation depends on having a
// mapping of recognized options to values of some sort.  In addition to this,
// each entry in the mapping also tracks a help message that is printed with the
// command line option for -help.  Because this is a simple mapping Parser, the
// data type can be any unsupported type.
//
template <typename DataType>
class Parser : public GenericParserBase
{
protected:
   class OptionInfo : public GenericOptionInfo
   {
   public:
      OptionInfo(StringRef name, DataType value, StringRef helpStr)
         : GenericOptionInfo(name, helpStr), m_value(value)
      {}

      OptionValue<DataType> m_value;
   };
   SmallVector<OptionInfo, 8> m_values;

public:
   Parser(Option &option) : GenericParserBase(option)
   {}

   using ParserDataType = DataType;

   // Implement virtual functions needed by GenericParserBase
   unsigned getNumOptions() const override
   {
      return unsigned(m_values.getSize());
   }

   StringRef getOption(unsigned index) const override
   {
      return m_values[index].m_name;
   }

   StringRef getDescription(unsigned index) const override
   {
      return m_values[index].m_helpStr;
   }

   // getOptionValue - Return the value of option name N.
   const GenericOptionValue &getOptionValue(unsigned index) const override
   {
      return m_values[index].m_value;
   }

   // parse - Return true on error.
   bool parse(Option &opt, StringRef argName, StringRef arg, DataType &value) {
      StringRef argVal;
      if (m_owner.hasArgStr()) {
         argVal = arg;
      } else {
         argVal = argName;
      }
      for (size_t i = 0, e = m_values.getSize(); i != e; ++i)
         if (m_values[i].m_names == argVal) {
            value = m_values[i].m_value.getValue();
            return false;
         }

      return opt.error("Cannot find option named '" + argVal + "'!");
   }

   /// addLiteralOption - Add an entry to the mapping table.
   ///
   template <typename OtherDataType>
   void addLiteralOption(StringRef name, const DataType &value, StringRef helpStr)
   {
      assert(findOption(name) == m_values.getSize() && "Option already exists!");
      OptionInfo x(name, static_cast<DataType>(value), helpStr);
      m_values.push_back(x);
      addLiteralOption(m_owner, name);
   }

   /// removeLiteralOption - Remove the specified option.
   ///
   void removeLiteralOption(StringRef name)
   {
      unsigned index = findOption(name);
      assert(index != m_values.getSize() && "Option not found!");
      m_values.erase(m_values.begin() + index);
   }
};

//--------------------------------------------------
// BasicParser - Super class of Parsers to provide boilerplate code
//
class BasicParserImpl
{ // non-template implementation of BasicParser<t>
public:
   BasicParserImpl(Option &)
   {}

   enum ValueExpected getValueExpectedFlagDefault() const
   {
      return ValueRequired;
   }

   void getExtraOptionNames(SmallVectorImpl<StringRef> &)
   {}

   void initialize()
   {}

   // Return the width of the option tag for printing...
   size_t getOptionWidth(const Option &option) const;

   // printOptionInfo - Print out information about this option.  The
   // to-be-maintained width is specified.
   //
   void printOptionInfo(const Option &option, size_t globalWidth) const;

   // printOptionNoValue - Print a placeholder for options that don't yet support
   // printOptionDiff().
   void printOptionNoValue(const Option &option, size_t globalWidth) const;

   // getValueName - Overload in subclass to provide a better default value.
   virtual StringRef getValueName() const
   {
      return "value";
   }

   // An out-of-line virtual method to provide a 'home' for this class.
   virtual void anchor();

protected:
   ~BasicParserImpl() = default;

   // A helper for BasicParser::printOptionDiff.
   void printOptionName(const Option &option, size_t globalWidth) const;
};

// BasicParser - The real basic Parser is just a template wrapper that provides
// a typedef for the provided data type.
//
template <typename DataType>
class BasicParser : public BasicParserImpl
{
public:
   using ParserDataType = DataType;
   using OptVal = OptionValue<DataType>;

   BasicParser(Option &option) : BasicParserImpl(option)
   {}

protected:
   ~BasicParser() = default;
};

//--------------------------------------------------
// Parser<bool>
//
template <>
class Parser<bool> : public BasicParser<bool>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, bool &value);

   void initialize()
   {}

   enum ValueExpected getValueExpectedFlagDefault() const
   {
      return ValueOptional;
   }

   // getValueName - Do not print =<value> at all.
   StringRef getValueName() const override
   {
      return StringRef();
   }

   void printOptionDiff(const Option &option, bool value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<bool>;

//--------------------------------------------------
// Parser<BoolOrDefault>
template <>
class Parser<BoolOrDefault> : public BasicParser<BoolOrDefault>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, BoolOrDefault &value);

   enum ValueExpected getValueExpectedFlagDefault() const
   {
      return ValueOptional;
   }

   // getValueName - Do not print =<value> at all.
   StringRef getValueName() const override
   {
      return StringRef();
   }

   void printOptionDiff(const Option &option, BoolOrDefault value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<BoolOrDefault>;

//--------------------------------------------------
// Parser<int>
//
template <>
class Parser<int> : public BasicParser<int>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, int &value);

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "int";
   }

   void printOptionDiff(const Option &option, int value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<int>;

//--------------------------------------------------
// Parser<unsigned>
//
template <> class Parser<unsigned> : public BasicParser<unsigned>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, unsigned &value);

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "uint";
   }

   void printOptionDiff(const Option &option, unsigned value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<unsigned>;

//--------------------------------------------------
// Parser<unsigned long long>
//
template <>
class Parser<unsigned long long>
      : public BasicParser<unsigned long long>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg,
              unsigned long long &value);

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "uint";
   }

   void printOptionDiff(const Option &option, unsigned long long value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<unsigned long long>;

//--------------------------------------------------
// Parser<double>
//
template <>
class Parser<double> : public BasicParser<double>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, double &value);

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "number";
   }

   void printOptionDiff(const Option &option, double value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<double>;

//--------------------------------------------------
// Parser<float>
//
template <>
class Parser<float> : public BasicParser<float>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &option, StringRef argName, StringRef arg, float &value);

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "number";
   }

   void printOptionDiff(const Option &option, float value, OptVal defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<float>;

//--------------------------------------------------
// Parser<std::string>
//
template <>
class Parser<std::string> : public BasicParser<std::string>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &, StringRef, StringRef arg, std::string &value)
   {
      value = arg.getStr();
      return false;
   }

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "string";
   }

   void printOptionDiff(const Option &option, StringRef value, const OptVal &defaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<std::string>;

//--------------------------------------------------
// Parser<char>
//
template <>
class Parser<char> : public BasicParser<char>
{
public:
   Parser(Option &option) : BasicParser(option)
   {}

   // parse - Return true on error.
   bool parse(Option &, StringRef, StringRef arg, char &value) {
      value = arg[0];
      return false;
   }

   // getValueName - Overload in subclass to provide a better default value.
   StringRef getValueName() const override
   {
      return "char";
   }

   void printOptionDiff(const Option &option, char value, OptVal DefaultValue,
                        size_t globalWidth) const;

   // An out-of-line virtual method to provide a 'home' for this class.
   void anchor() override;
};

extern template class BasicParser<char>;

//--------------------------------------------------
// PrintOptionDiff
//
// This collection of wrappers is the intermediary between class opt and class
// Parser to handle all the template nastiness.

// This overloaded function is selected by the generic Parser.
template <typename ParserClass, typename DataType>
void print_option_diff(const Option &option, const GenericParserBase &parser, const DataType &value,
                       const OptionValue<DataType> &defaultValue, size_t globalWidth)
{
   OptionValue<DataType> ovalue = value;
   parser.printOptionDiff(option, ovalue, defaultValue, globalWidth);
}

// This is instantiated for basic Parsers when the parsed value has a different
// type than the option value. e.g. HelpPrinter.
template <typename ParserDataType, typename ValueDataType>
struct OptionDiffPrinter
{
   void print(const Option &option, const Parser<ParserDataType> &parser, const ValueDataType & /*V*/,
              const OptionValue<ValueDataType> & /*Default*/, size_t globalWidth)
   {
      parser.printOptionNoValue(option, globalWidth);
   }
};

// This is instantiated for basic Parsers when the parsed value has the same
// type as the option value.
template <typename DataType>
struct OptionDiffPrinter<DataType, DataType> {
   void print(const Option &option, const Parser<DataType> &parser, const DataType &value,
              const OptionValue<DataType> &defaultValue, size_t globalWidth)
   {
      parser.printOptionDiff(option, value, defaultValue, globalWidth);
   }
};

// This overloaded function is selected by the basic Parser, which may parse a
// different type than the option type.
template <typename ParserClass, typename ValueDataType>
void print_option_diff(
      const Option &option,
      const BasicParser<typename ParserClass::ParserDataType> &parser,
      const ValueDataType &value, const OptionValue<ValueDataType> &defaultValue, size_t globalWidth)
{

   OptionDiffPrinter<typename ParserClass::ParserDataType, ValueDataType> printer;
   printer.print(option, static_cast<const ParserClass &>(parser), value, defaultValue,
                 globalWidth);
}

//===----------------------------------------------------------------------===//
// Applicator class - This class is used because we must use partial
// specialization to handle literal string arguments specially (const char* does
// not correctly respond to the apply method).  Because the syntax to use this
// is a pain, we have the 'apply' method below to handle the nastiness...
//
template <typename Mode>
struct Applicator
{
   template <typename Opt>
   static void opt(const Mode &mode, Opt &option)
   {
      mode.apply(option);
   }
};

// Handle const char* as a special case...
template <unsigned n>
struct Applicator<char[n]>
{
   template <typename Opt> static void opt(StringRef str, Opt &opt)
   {
      opt.setArgStr(str);
   }
};

template <unsigned n>
struct Applicator<const char[n]>
{
   template <typename Opt> static void opt(StringRef str, Opt &opt)
   {
      opt.setArgStr(str);
   }
};

template <>
struct Applicator<StringRef>
{
   template <typename Opt> static void opt(StringRef str, Opt &opt)
   {
      opt.setArgStr(str);
   }
};

template <>
struct Applicator<NumOccurrencesFlag>
{
   static void opt(NumOccurrencesFlag flag, Option &opt)
   {
      opt.setNumOccurrencesFlag(flag);
   }
};

template <>
struct Applicator<ValueExpected>
{
   static void opt(ValueExpected value, Option &opt)
   {
      opt.setValueExpectedFlag(value);
   }
};

template <>
struct Applicator<OptionHidden>
{
   static void opt(OptionHidden optionHide, Option &option)
   {
      option.setHiddenFlag(optionHide);
   }
};

template <>
struct Applicator<FormattingFlags>
{
   static void opt(FormattingFlags flag, Option &option)
   {
      option.setFormattingFlag(flag);
   }
};

template <>
struct Applicator<MiscFlags>
{
   static void opt(MiscFlags mflag, Option &option)
   {
      option.setMiscFlag(mflag);
   }
};

// apply method - Apply modifiers to an option in a type safe way.
template <typename Opt, typename Mode, typename... Modes>
void apply(Opt *option, const Mode &mode, const Modes &... modes)
{
   Applicator<Mode>::opt(mode, *option);
   apply(option, modes...);
}

template <typename Opt, typename Mode>
void apply(Opt *option, const Mode &mode)
{
   Applicator<Mode>::opt(mode, *option);
}

//===----------------------------------------------------------------------===//
// OptStorage class

// Default storage class definition: external storage.  This implementation
// assumes the user will specify a variable to store the data into with the
// cl::location(x) modifier.
//
template <typename DataType, bool ExternalStorage, bool isClass>
class OptStorage
{
   DataType *m_location = nullptr; // Where to store the object...
   OptionValue<DataType> m_defaultValue;

   void checkLocation() const
   {
      assert(m_location && "cmd::location(...) not specified for a command "
                           "line option with external storage, "
                           "or cmd::init specified before cmd::location()!!");
   }

public:
   OptStorage() = default;

   bool setLocation(Option &option, DataType &location)
   {
      if (m_location) {
         return option.error("cmd::location(x) specified more than once!");
      }
      m_location = &location;
      m_defaultValue = location;
      return false;
   }

   template <typename T>
   void setValue(const T &value, bool initial = false)
   {
      checkLocation();
      *m_location = value;
      if (initial) {
         m_defaultValue = value;
      }
   }

   DataType &getValue()
   {
      checkLocation();
      return *m_location;
   }

   const DataType &getValue() const
   {
      checkLocation();
      return *m_location;
   }

   operator DataType() const
   {
      return this->getValue();
   }

   const OptionValue<DataType> &getDefault() const
   {
      return m_defaultValue;
   }
};

// Define how to hold a class type object, such as a string.  Since we can
// inherit from a class, we do so.  This makes us exactly compatible with the
// object in all cases that it is used.
//
template <typename DataType>
class OptStorage<DataType, false, true> : public DataType
{
public:
   OptionValue<DataType> m_defaultValue;

   template <typename T> void setValue(const T &value, bool initial = false)
   {
      DataType::operator=(value);
      if (initial) {
         m_defaultValue = value;
      }
   }

   DataType &getValue()
   {
      return *this;
   }

   const DataType &getValue() const
   {
      return *this;
   }

   const OptionValue<DataType> &getDefault() const
   {
      return m_defaultValue;
   }
};

// Define a partial specialization to handle things we cannot inherit from.  In
// this case, we store an instance through containment, and overload operators
// to get at the value.
//
template <typename DataType>
class OptStorage<DataType, false, false>
{
public:
   DataType m_value;
   OptionValue<DataType> m_defaultValue;

   // Make sure we initialize the value with the default constructor for the
   // type.
   OptStorage() : m_value(DataType()), m_defaultValue(DataType())
   {}

   template <typename T> void setValue(const T &value, bool initial = false)
   {
      m_value = value;
      if (initial) {
         m_defaultValue = value;
      }
   }

   DataType &getValue()
   {
      return m_value;
   }

   DataType getValue() const
   {
      return m_value;
   }

   const OptionValue<DataType> &getDefault() const
   {
      return m_defaultValue;
   }

   operator DataType() const
   {
      return getValue();
   }

   // If the datatype is a pointer, support -> on it.
   DataType operator->() const
   {
      return m_value;
   }
};

//===----------------------------------------------------------------------===//
// opt - A scalar command line option.
//
template <typename DataType, bool ExternalStorage = false,
          typename ParserClass = Parser<DataType>>
class Opt : public Option,
      public OptStorage<DataType, ExternalStorage,
      std::is_class<DataType>::value>
{
   ParserClass m_parser;

   bool handleOccurrence(unsigned pos, StringRef argName,
                         StringRef arg) override
   {
      typename ParserClass::ParserDataType value =
            typename ParserClass::ParserDataType();
      if (m_parser.parse(*this, argName, arg, value)) {
         return true; // Parse error!
      }
      this->setValue(value);
      this->setPosition(pos);
      return false;
   }

   enum ValueExpected getValueExpectedFlagDefault() const override
   {
      return m_parser.getValueExpectedFlagDefault();
   }

   void getExtraOptionNames(SmallVectorImpl<StringRef> &optionNames) override
   {
      return m_parser.getExtraOptionNames(optionNames);
   }

   // Forward printing stuff to the Parser...
   size_t getOptionWidth() const override
   {
      return m_parser.getOptionWidth(*this);
   }

   void printOptionInfo(size_t globalWidth) const override
   {
      m_parser.printOptionInfo(*this, globalWidth);
   }

   void printOptionValue(size_t globalWidth, bool force) const override
   {
      if (force || this->getDefault().compare(this->getValue())) {
         cmd::print_option_diff<ParserClass>(*this, m_parser, this->getValue(),
                                             this->getDefault(), globalWidth);
      }
   }

   template <class T, class = typename std::enable_if<
                std::is_assignable<T&, T>::value>::type>
   void setDefaultImpl()
   {
      const OptionValue<DataType> &value = this->getDefault();
      if (value.hasValue()) {
         this->setValue(value.getValue());
      }
   }

   template <class T, class = typename std::enable_if<
                !std::is_assignable<T&, T>::value>::type>
   void setDefaultImpl(...)
   {}

   void setDefault() override
   {
      setDefaultImpl<DataType>();
   }

   void done()
   {
      addArgument();
      m_parser.initialize();
   }

public:
   // Command line options should not be copyable
   Opt(const Opt &) = delete;
   Opt &operator=(const Opt &) = delete;

   // setInitialValue - Used by the cl::init modifier...
   void setInitialValue(const DataType &value)
   {
      this->setValue(value, true);
   }

   ParserClass &getParser()
   {
      return m_parser;
   }

   template <typename T>
   DataType &operator=(const T &value)
   {
      this->setValue(value);
      return this->getValue();
   }

   template <typename... Modes>
   explicit Opt(const Modes &... modes)
      : Option(Optional, NotHidden), m_parser(*this)
   {
      apply(this, modes...);
      done();
   }
};

extern template class Opt<unsigned>;
extern template class Opt<int>;
extern template class Opt<std::string>;
extern template class Opt<char>;
extern template class Opt<bool>;

//===----------------------------------------------------------------------===//
// ListStorage class

// Default storage class definition: external storage.  This implementation
// assumes the user will specify a variable to store the data into with the
// cl::location(x) modifier.
//
template <typename DataType, typename StorageClass>
class ListStorage
{
   StorageClass *m_storage = nullptr; // Where to store the object...

public:
   ListStorage() = default;

   bool setLocation(Option &option, StorageClass &storage)
   {
      if (m_storage) {
         return option.error("cmd::location(x) specified more than once!");
      }
      m_storage = &storage;
      return false;
   }

   template <class T>
   void addValue(const T &value)
   {
      assert(m_storage != nullptr && "cmd::location(...) not specified for a command "
                                     "line option with external storage!");
      m_storage->pushBack(value);
   }
};

// Define how to hold a class type object, such as a string.
// Originally this code inherited from std::vector. In transitioning to a new
// API for command line options we should change this. The new implementation
// of this ListStorage specialization implements the minimum subset of the
// std::vector API required for all the current clients.
//
// FIXME: Reduce this API to a more narrow subset of std::vector
//
template <typename DataType>
class ListStorage<DataType, bool>
{
   std::vector<DataType> m_storage;

public:
   using iterator = typename std::vector<DataType>::iterator;

   iterator begin()
   {
      return m_storage.begin();
   }

   iterator end()
   {
      return m_storage.end();
   }

   using const_iterator = typename std::vector<DataType>::const_iterator;

   const_iterator begin() const
   {
      return m_storage.begin();
   }

   const_iterator end() const
   {
      return m_storage.end();
   }

   using size_type = typename std::vector<DataType>::size_type;

   size_type getSize() const
   {
      return m_storage.size();
   }

   bool empty() const
   {
      return m_storage.empty();
   }

   void push_back(const DataType &value)
   {
      m_storage.push_back(value);
   }

   void push_back(DataType &&value)
   {
      m_storage.push_back(value);
   }

   using reference = typename std::vector<DataType>::reference;
   using const_reference = typename std::vector<DataType>::const_reference;

   reference operator[](size_type pos)
   {
      return m_storage[pos];
   }

   const_reference operator[](size_type pos) const
   {
      return m_storage[pos];
   }

   iterator erase(const_iterator pos)
   {
      return m_storage.erase(pos);
   }

   iterator erase(const_iterator first, const_iterator last)
   {
      return m_storage.erase(first, last);
   }

   iterator erase(iterator pos)
   {
      return m_storage.erase(pos);
   }

   iterator erase(iterator first, iterator last)
   {
      return m_storage.erase(first, last);
   }

   iterator insert(const_iterator pos, const DataType &value)
   {
      return m_storage.insert(pos, value);
   }

   iterator insert(const_iterator pos, DataType &&value)
   {
      return m_storage.insert(pos, value);
   }

   iterator insert(iterator pos, const DataType &value)
   {
      return m_storage.insert(pos, value);
   }

   iterator insert(iterator pos, DataType &&value)
   {
      return m_storage.insert(pos, value);
   }

   reference front()
   {
      return m_storage.front();
   }

   const_reference front() const
   {
      return m_storage.front();
   }

   operator std::vector<DataType>&()
   {
      return m_storage;
   }

   operator ArrayRef<DataType>()
   {
      return m_storage;
   }
   std::vector<DataType> *operator&()
   {
      return &m_storage;
   }

   const std::vector<DataType> *operator&() const
   {
      return &m_storage;
   }

   template <typename T> void addValue(const T &value)
   {
      m_storage.push_back(value);
   }
};

//===----------------------------------------------------------------------===//
// list - A list of command line options.
//
template <typename DataType, class StorageClass = bool,
          class ParserClass = Parser<DataType>>
class List : public Option, public ListStorage<DataType, StorageClass>
{
   std::vector<unsigned> m_positions;
   ParserClass m_parser;

   enum ValueExpected getValueExpectedFlagDefault() const override
   {
      return m_parser.getValueExpectedFlagDefault();
   }

   void getExtraOptionNames(SmallVectorImpl<StringRef> &optionNames) override
   {
      return m_parser.getExtraOptionNames(optionNames);
   }

   bool handleOccurrence(unsigned pos, StringRef argName,
                         StringRef arg) override
   {
      typename ParserClass::ParserDataType value =
            typename ParserClass::ParserDataType();
      if (m_parser.parse(*this, argName, arg, value)) {
         return true; // Parse Error!
      }
      ListStorage<DataType, StorageClass>::addValue(value);
      setPosition(pos);
      m_positions.push_back(pos);
      return false;
   }

   // Forward printing stuff to the Parser...
   size_t getOptionWidth() const override
   {
      return m_parser.getOptionWidth(*this);
   }

   void printOptionInfo(size_t globalWidth) const override
   {
      m_parser.printOptionInfo(*this, globalWidth);
   }

   // Unimplemented: list options don't currently store their default value.
   void printOptionValue(size_t /*GlobalWidth*/, bool /*Force*/) const override
   {
   }

   void setDefault() override
   {}

   void done()
   {
      addArgument();
      m_parser.initialize();
   }

public:
   // Command line options should not be copyable
   List(const List &) = delete;
   List &operator=(const List &) = delete;

   ParserClass &getParser()
   {
      return m_parser;
   }

   unsigned getPosition(unsigned optnum) const
   {
      assert(optnum < this->size() && "Invalid option index");
      return m_positions[optnum];
   }

   void setNumAdditionalVals(unsigned n)
   {
      Option::setNumAdditionalVals(n);
   }

   template <typename... Modes>
   explicit List(const Modes &... modes)
      : Option(ZeroOrMore, NotHidden), m_parser(*this)
   {
      apply(this, modes...);
      done();
   }
};

// MultiValue - Modifier to set the number of additional values.
struct MultiValue
{
   unsigned m_additionalVals;
   explicit MultiValue(unsigned value) : m_additionalVals(value)
   {}

   template <typename DataType, typename StorageType, typename ParserType>
   void apply(List<DataType, StorageType, ParserType> &list) const
   {
      list.setNumAdditionalVals(m_additionalVals);
   }
};

//===----------------------------------------------------------------------===//
// BitsStorage class

// Default storage class definition: external storage.  This implementation
// assumes the user will specify a variable to store the data into with the
// cl::location(x) modifier.
//
template <typename DataType, typename StorageClass>
class BitsStorage
{
   unsigned *m_storage = nullptr; // Where to store the bits...

   template <typename T>
   static unsigned Bit(const T &value)
   {
      unsigned bitPos = reinterpret_cast<unsigned>(value);
      assert(bitPos < sizeof(unsigned) * CHAR_BIT &&
             "enum exceeds width of bit vector!");
      return 1 << bitPos;
   }

public:
   BitsStorage() = default;

   bool setLocation(Option &option, unsigned &storage)
   {
      if (storage) {
         return option.error("cl::location(x) specified more than once!");
      }
      m_storage = &storage;
      return false;
   }

   template <typename T> void addValue(const T &value)
   {
      assert(m_storage != 0 && "cl::location(...) not specified for a command "
                               "line option with external storage!");
      *m_storage |= Bit(value);
   }

   unsigned getBits()
   {
      return *m_storage;
   }

   template <typename T> bool isSet(const T &value)
   {
      return (*m_storage & Bit(value)) != 0;
   }
};

// Define how to hold bits.  Since we can inherit from a class, we do so.
// This makes us exactly compatible with the bits in all cases that it is used.
//
template <typename DataType>
class BitsStorage<DataType, bool>
{
   unsigned m_storage; // Where to store the bits...

   template <typename T>
   static unsigned Bit(const T &value)
   {
      unsigned bitPos = (unsigned)value;
      assert(bitPos < sizeof(unsigned) * CHAR_BIT &&
             "enum exceeds width of bit vector!");
      return 1 << value;
   }

public:
   template <typename T>
   void addValue(const T &value)
   {
      m_storage |= Bit(value);
   }

   unsigned getBits()
   {
      return m_storage;
   }

   template <typename T>
   bool isSet(const T &value)
   {
      return (m_storage & Bit(value)) != 0;
   }
};

//===----------------------------------------------------------------------===//
// bits - A bit vector of command options.
//
template <typename DataType, typename Storage = bool,
          typename ParserClass = Parser<DataType>>
class Bits : public Option, public BitsStorage<DataType, Storage>
{
   std::vector<unsigned> m_positions;
   ParserClass m_parser;

   enum ValueExpected getValueExpectedFlagDefault() const override
   {
      return m_parser.getValueExpectedFlagDefault();
   }

   void getExtraOptionNames(SmallVectorImpl<StringRef> &optionNames) override
   {
      return m_parser.getExtraOptionNames(optionNames);
   }

   bool handleOccurrence(unsigned pos, StringRef argName,
                         StringRef arg) override
   {
      typename ParserClass::ParserDataType value =
            typename ParserClass::ParserDataType();
      if (m_parser.parse(*this, argName, arg, value)) {
         return true; // Parse Error!
      }
      this->addValue(value);
      setPosition(pos);
      m_positions.push_back(pos);
      return false;
   }

   // Forward printing stuff to the Parser...
   size_t getOptionWidth() const override
   {
      return m_parser.getOptionWidth(*this);
   }

   void printOptionInfo(size_t globalWidth) const override
   {
      m_parser.printOptionInfo(*this, globalWidth);
   }

   // Unimplemented: bits options don't currently store their default values.
   void printOptionValue(size_t /*GlobalWidth*/, bool /*Force*/) const override
   {
   }

   void setDefault() override
   {}

   void done()
   {
      addArgument();
      m_parser.initialize();
   }

public:
   // Command line options should not be copyable
   Bits(const Bits &) = delete;
   Bits &operator=(const Bits &) = delete;

   ParserClass &getParser()
   {
      return m_parser;
   }

   unsigned getPosition(unsigned optnum) const
   {
      assert(optnum < this->size() && "Invalid option index");
      return m_positions[optnum];
   }

   template <typename... Modes>
   explicit Bits(const Modes &... modes)
      : Option(ZeroOrMore, NotHidden), m_parser(*this)
   {
      apply(this, modes...);
      done();
   }
};

//===----------------------------------------------------------------------===//
// Aliased command line option (alias this name to a preexisting name)
//

class Alias : public Option
{
   Option *m_aliasFor;

   bool handleOccurrence(unsigned pos, StringRef /*ArgName*/,
                         StringRef arg) override
   {
      return m_aliasFor->handleOccurrence(pos, m_aliasFor->m_argStr, arg);
   }

   bool addOccurrence(unsigned pos, StringRef /*ArgName*/, StringRef value,
                      bool multiArg = false) override
   {
      return m_aliasFor->addOccurrence(pos, m_aliasFor->m_argStr, value, multiArg);
   }

   // Handle printing stuff...
   size_t getOptionWidth() const override;
   void printOptionInfo(size_t globalWidth) const override;

   // Aliases do not need to print their values.
   void printOptionValue(size_t /*GlobalWidth*/, bool /*Force*/) const override
   {
   }

   void setDefault() override
   {
      m_aliasFor->setDefault();
   }

   ValueExpected getValueExpectedFlagDefault() const override
   {
      return m_aliasFor->getValueExpectedFlag();
   }

   void done()
   {
      if (!hasArgStr()) {
         error("cmd::Alias must have argument name specified!");
      }
      if (!m_aliasFor) {
         error("cmd::Alias must have an cmd::AliasOpt(option) specified!");
      }
      m_subs = m_aliasFor->m_subs;
      addArgument();
   }

public:
   // Command line options should not be copyable
   Alias(const Alias &) = delete;
   Alias &operator=(const Alias &) = delete;

   void setAliasFor(Option &option)
   {
      if (m_aliasFor) {
         error("cmd::Alias must only have one cmd::AliasOpt(...) specified!");
      }
      m_aliasFor = &option;
   }

   template <typename... Modes>
   explicit Alias(const Modes &... modes)
      : Option(Optional, Hidden), m_aliasFor(nullptr)
   {
      apply(this, modes...);
      done();
   }
};

// aliasfor - Modifier to set the option an alias aliases.
struct AliasOpt
{
   Option &m_option;

   explicit AliasOpt(Option &option) : m_option(option)
   {}

   void apply(Alias &alias) const
   {
      alias.setAliasFor(m_option);
   }
};

// ExtraHelp - provide additional help at the end of the normal help
// output. All occurrences of cl::ExtraHelp will be accumulated and
// printed to stderr at the end of the regular help, just before
// exit is called.
struct ExtraHelp
{
   StringRef m_moreHelp;

   explicit ExtraHelp(StringRef help);
};

void print_version_message();

/// This function just prints the help message, exactly the same way as if the
/// -help or -help-hidden option had been given on the command line.
///
/// \param Hidden if true will print hidden options
/// \param Categorized if true print options in categories
void print_help_message(bool hidden = false, bool categorized = false);

//===----------------------------------------------------------------------===//
// Public interface for accessing registered options.
//

/// Use this to get a StringMap to all registered named options
/// (e.g. -help). Note \p Map Should be an empty StringMap.
///
/// \return A reference to the StringMap used by the cl APIs to parse options.
///
/// Access to unnamed arguments (i.e. positional) are not provided because
/// it is expected that the client already has access to these.
///
/// Typical usage:
/// \code
/// main(int argc,char* argv[]) {
/// StringMap<llvm::cl::Option*> &opts = llvm::cl::getRegisteredOptions();
/// assert(opts.count("help") == 1)
/// opts["help"]->setDescription("Show alphabetical help information")
/// // More code
/// llvm::cl::parse_commandline_options(argc,argv);
/// //More code
/// }
/// \endcode
///
/// This interface is useful for modifying options in libraries that are out of
/// the control of the client. The options should be modified before calling
/// llvm::cl::parse_commandline_options().
///
/// Hopefully this API can be deprecated soon. Any situation where options need
/// to be modified by tools or libraries should be handled by sane APIs rather
/// than just handing around a global list.
StringMap<Option *> &get_registered_options(SubCommand &sub = *sg_topLevelSubCommand);

/// Use this to get all registered SubCommands from the provided Parser.
///
/// \return A range of all SubCommand pointers registered with the Parser.
///
/// Typical usage:
/// \code
/// main(int argc, char* argv[]) {
///   llvm::cl::parse_commandline_options(argc, argv);
///   for (auto* S : llvm::cl::getRegisteredSubcommands()) {
///     if (*S) {
///       std::cout << "Executing subcommand: " << S->getName() << std::endl;
///       // Execute some function based on the name...
///     }
///   }
/// }
/// \endcode
///
/// This interface is useful for defining subcommands in libraries and
/// the dispatch from a single point (like in the main function).
IteratorRange<typename SmallPtrSet<SubCommand *, 4>::iterator>
get_registered_subcommands();

//===----------------------------------------------------------------------===//
// Standalone command line processing utilities.
//

/// Tokenizes a command line that can contain escapes and quotes.
//
/// The quoting rules match those used by GCC and other tools that use
/// libiberty's buildargv() or expandargv() utilities, and do not match bash.
/// They differ from buildargv() on treatment of backslashes that do not escape
/// a special character to make it possible to accept most Windows file paths.
///
/// \param [in] Source The string to be split on whitespace with quotes.
/// \param [in] Saver Delegates back to the caller for saving parsed strings.
/// \param [in] MarkEOLs true if tokenizing a response file and you want end of
/// lines and end of the response file to be marked with a nullptr string.
/// \param [out] NewArgv All parsed strings are appended to NewArgv.
void tokenize_gnu_command_line(StringRef source, StringSaver &saver,
                               SmallVectorImpl<const char *> &newArgv,
                               bool markEOLs = false);

/// Tokenizes a Windows command line which may contain quotes and escaped
/// quotes.
///
/// See MSDN docs for CommandLineToArgvW for information on the quoting rules.
/// http://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx
///
/// \param [in] Source The string to be split on whitespace with quotes.
/// \param [in] Saver Delegates back to the caller for saving parsed strings.
/// \param [in] MarkEOLs true if tokenizing a response file and you want end of
/// lines and end of the response file to be marked with a nullptr string.
/// \param [out] NewArgv All parsed strings are appended to NewArgv.
void tokenize_windows_command_line(StringRef source, StringSaver &saver,
                                   SmallVectorImpl<const char *> &newArgv,
                                   bool markEOLs = false);

/// String tokenization function type.  Should be compatible with either
/// Windows or Unix command line tokenizers.
using TokenizerCallback = void (*)(StringRef source, StringSaver &saver,
SmallVectorImpl<const char *> &newArgv,
bool markEOLs);

/// Tokenizes content of configuration file.
///
/// \param [in] Source The string representing content of config file.
/// \param [in] Saver Delegates back to the caller for saving parsed strings.
/// \param [out] NewArgv All parsed strings are appended to NewArgv.
/// \param [in] MarkEOLs Added for compatibility with TokenizerCallback.
///
/// It works like TokenizeGNUCommandLine with ability to skip comment lines.
///
void tokenize_config_file(StringRef source, StringSaver &saver,
                          SmallVectorImpl<const char *> &newArgv,
                          bool markEOLs = false);

/// Reads command line options from the given configuration file.
///
/// \param [in] CfgFileName Path to configuration file.
/// \param [in] Saver  Objects that saves allocated strings.
/// \param [out] Argv Array to which the read options are added.
/// \return true if the file was successfully read.
///
/// It reads content of the specified file, tokenizes it and expands "@file"
/// commands resolving file names in them relative to the directory where
/// CfgFilename resides.
///
bool read_config_file(StringRef cfgFileName, StringSaver &saver,
                      SmallVectorImpl<const char *> &argv);

/// Expand response files on a command line recursively using the given
/// StringSaver and tokenization strategy.  Argv should contain the command line
/// before expansion and will be modified in place. If requested, Argv will
/// also be populated with nullptrs indicating where each response file line
/// ends, which is useful for the "/link" argument that needs to consume all
/// remaining arguments only until the next end of line, when in a response
/// file.
///
/// \param [in] Saver Delegates back to the caller for saving parsed strings.
/// \param [in] Tokenizer Tokenization strategy. Typically Unix or Windows.
/// \param [in,out] Argv Command line into which to expand response files.
/// \param [in] MarkEOLs Mark end of lines and the end of the response file
/// with nullptrs in the Argv vector.
/// \param [in] RelativeNames true if names of nested response files must be
/// resolved relative to including file.
/// \return true if all @files were expanded successfully or there were none.
bool expand_response_files(StringSaver &saver, TokenizerCallback tokenizer,
                           SmallVectorImpl<const char *> &argv,
                           bool markEOLs = false, bool relativeNames = false);

/// Mark all options not part of this category as cl::ReallyHidden.
///
/// \param Category the category of options to keep displaying
///
/// Some tools (like clang-format) like to be able to hide all options that are
/// not specific to the tool. This function allows a tool to specify a single
/// option category to display in the -help output.
void hide_unrelated_options(OptionCategory &category,
                            SubCommand &sub = *sg_topLevelSubCommand);

/// Mark all options not part of the categories as cl::ReallyHidden.
///
/// \param Categories the categories of options to keep displaying.
///
/// Some tools (like clang-format) like to be able to hide all options that are
/// not specific to the tool. This function allows a tool to specify a single
/// option category to display in the -help output.
void hide_unrelated_options(ArrayRef<const OptionCategory *> categories,
                            SubCommand &sub = *sg_topLevelSubCommand);

/// Reset all command line options to a state that looks as if they have
/// never appeared on the command line.  This is useful for being able to parse
/// a command line multiple times (especially useful for writing tests).
void reset_all_option_occurrences();

/// Reset the command line Parser back to its initial state.  This
/// removes
/// all options, categories, and subcommands and returns the Parser to a state
/// where no options are supported.
void reset_command_line_parser();

} // cmd
} // polar

#endif // POLARPHP_UTILS_COMMAND_LINE_H
