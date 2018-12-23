// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/05.

//===----------------------------------------------------------------------===//
//
// This is a utility class used to parse user-provided text files with
// "special case lists" for code sanitizers. Such files are used to
// define an "ABI list" for DataFlowSanitizer and blacklists for sanitizers
// like AddressSanitizer or UndefinedBehaviorSanitizer.
//
// Empty lines and lines starting with "#" are ignored. m_sections are defined
// using a '[section_name]' header and can be used to specify sanitizers the
// entries below it apply to. section names are regular expressions, and
// entries without a section header match all sections (e.g. an '[*]' header
// is assumed.)
// The remaining lines should have the form:
//   prefix:wildcard_expression[=category]
// If category is not specified, it is assumed to be empty string.
// Definitions of "prefix" and "category" are sanitizer-specific. For example,
// sanitizer blacklists support prefixes "src", "fun" and "global".
// Wildcard expressions define, respectively, source files, functions or
// globals which shouldn't be instrumented.
// Examples of categories:
//   "functional": used in DFSan to list functions with pure functional
//                 semantics.
//   "init": used in ASan blacklist to disable initialization-order bugs
//           detection for certain globals or source files.
// Full special case list file example:
// ---
// [address]
// # Blacklisted items:
// fun:*_ZN4base6subtle*
// global:*global_with_bad_access_or_initialization*
// global:*global_with_initialization_issues*=init
// type:*Namespace::ClassName*=init
// src:file_with_tricky_code.cc
// src:ignore-global-initializers-issues.cc=init
//
// [dataflow]
// # Functions with pure functional semantics:
// fun:cos=functional
// fun:sin=functional
// ---
// Note that the wild card is in fact an llvm::Regex, but * is automatically
// replaced with .*
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_SPECIAL_CASE_LIST_H
#define POLARPHP_UTILS_SPECIAL_CASE_LIST_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringSet.h"
#include "polarphp/utils/TrigramIndex.h"
#include <string>
#include <vector>
#include <regex>

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
} // basic

namespace utils {

class MemoryBuffer;
using polar::basic::StringRef;
using polar::basic::StringMap;
using polar::basic::StringSet;

class SpecialCaseList
{
public:
   /// Parses the special case list entries from files. On failure, returns
   /// 0 and writes an error message to string.
   static std::unique_ptr<SpecialCaseList>
   create(const std::vector<std::string> &paths, std::string &error);
   /// Parses the special case list from a memory buffer. On failure, returns
   /// 0 and writes an error message to string.
   static std::unique_ptr<SpecialCaseList> create(const MemoryBuffer *memoryBuffer,
                                                  std::string &error);
   /// Parses the special case list entries from files. On failure, reports a
   /// fatal error.
   static std::unique_ptr<SpecialCaseList>
   createOrDie(const std::vector<std::string> &paths);

   ~SpecialCaseList();

   /// Returns true, if special case list contains a line
   /// \code
   ///   @prefix:<E>=@category
   /// \endcode
   /// where @query satisfies wildcard expression <E> in a given @section.
   bool inSection(StringRef section, StringRef prefix, StringRef query,
                  StringRef category = StringRef()) const;

   /// Returns the line number corresponding to the special case list entry if
   /// the special case list contains a line
   /// \code
   ///   @prefix:<E>=@category
   /// \endcode
   /// where @query satisfies wildcard expression <E> in a given @section.
   /// Returns zero if there is no blacklist entry corresponding to this
   /// expression.
   unsigned inSectionBlame(StringRef section, StringRef prefix, StringRef query,
                           StringRef category = StringRef()) const;

protected:
   // Implementations of the create*() functions that can also be used by derived
   // classes.
   bool createInternal(const std::vector<std::string> &paths,
                       std::string &error);
   bool createInternal(const MemoryBuffer *memoryBuffer, std::string &error);

   SpecialCaseList() = default;
   SpecialCaseList(SpecialCaseList const &) = delete;
   SpecialCaseList &operator=(SpecialCaseList const &) = delete;

   /// Represents a set of regular expressions.  Regular expressions which are
   /// "literal" (i.e. no regex metacharacters) are stored in m_strings.  The
   /// reason for doing so is efficiency; StringMap is much faster at matching
   /// literal strings than Regex.
   class Matcher
   {
   public:
      bool insert(std::string regexp, unsigned lineNumber, std::string &reError);
      // Returns the line number in the source file that this query matches to.
      // Returns zero if no match is found.
      unsigned match(StringRef query) const;

   private:
      StringMap<unsigned> m_strings;
      TrigramIndex m_trigrams;
      std::vector<std::pair<std::unique_ptr<std::regex>, unsigned>> m_regExes;
   };

   using SectionEntries = StringMap<StringMap<Matcher>>;

   struct Section {
      Section(std::unique_ptr<Matcher> matcher) : m_sectionMatcher(std::move(matcher))
      {}

      std::unique_ptr<Matcher> m_sectionMatcher;
      SectionEntries m_entries;
   };

   std::vector<Section> m_sections;

   /// Parses just-constructed SpecialCaseList entries from a memory buffer.
   bool parse(const MemoryBuffer *memoryBuffer, StringMap<size_t> &m_sectionsMap,
              std::string &error);

   // Helper method for derived classes to search by prefix, query, and category
   // once they have already resolved a section entry.
   unsigned inSectionBlame(const SectionEntries &entries, StringRef prefix,
                           StringRef query, StringRef category) const;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_SPECIAL_CASE_LIST_H
